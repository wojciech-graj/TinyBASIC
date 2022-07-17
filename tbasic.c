/*
 * Copyright (c) 2022 Wojciech Graj
 *
 * Licensed under the MIT license: https://opensource.org/licenses/MIT
 * Permission is granted to use, copy, modify, and redistribute the work.
 * Full license information available in the project LICENSE file.
 *
 * DESCRIPTION:
 *   A TinyBASIC Interpreter
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define KEYWORD_CNT 12
#define VTAB_INIT_SIZE 16
#define CALLSTACK_SIZE 32
#define XOFF "\023"
#define XON "\021"

#ifdef PRINT_ERROR
void errexit(int status)
{
	char *m;
	switch (status) {
	case 8:
		m = "Cannot load source";
		break;
	case 9:
		m = "Line number 0 not allowed";
		break;
	case 18:
		m = "LET is missing a variable name";
		break;
	case 20:
		m = "LET is missing an =";
		break;
	case 37:
		m = "No line to GO TO";
		break;
	case 46:
		m = "GOSUB subroutine does not exist";
		break;
	case 104:
		m = "INPUT syntax bad - expects variable name";
		break;
	case 123:
		m = "INPUT syntax bad - expects comma";
		break;
	case 133:
		m = "RETURN has no matching GOSUB";
		break;
	case 154:
		m = "Can't LIST line number 0";
		break;
	case 188:
		m = "Memory overflow: too many GOSUB's";
		break;
	case 259:
		m = "RND (0) not allowed";
		break;
	case 303:
		m = "USR not supported";
		break;
	case 330:
		m = "IF syntax error - expects relation operator";
		break;
	}
	fprintf(stderr, "%d: %s\n", status, m);
	exit(status);
}
#else
#define errexit exit
#endif

static const char ZERO = '\0';
short vars[26];
char *s /* Source code */, *c /* Current location in source */;

struct {
	unsigned i;
	char *stack[CALLSTACK_SIZE];
} callstack = { 0 };

void skpspc(void)
{
	for (; *c == ' '; c++);
}

void skpto(char target)
{
	for (; *c != target; c++);
}

/*
 * Compare strings ignoring spaces in s1 and checking that s0 is shorter than s1
 * Returns length of s1 that matches (disregarding spaces) s2 if fully match, -1 otherwise
 **/
int strspccmp(char *s0, char *s1)
{
	int i0, i1;
	for (i0 = i1 = 0; s0[i0]; i0++) {
		for (; s1[i1] == ' '; i1++);
		if (s0[i0] != s1[i1])
			return -1;
		i1++;
	}
	return i1;
}

short expr(void);

short factor(void)
{
	skpspc();
	short val;
	if (*c == '(') {
		c++;
		val = expr();
		skpto(')');
		c++;
	} else if (isupper(*c)) {
		if (!strncmp("RND", c, 3)) {
			skpto('(');
			c++;
			short max = expr();
			if (!max)
				errexit(259);
			val = rand() % max;
			skpto(')');
		} else if (!strncmp("USR", c, 3)) {
			errexit(303);
		} else {
			val = vars[*c - 'A'];
		}
		c++;
	} else if (isdigit(*c)) {
		val = atoi(c);
		for (; isdigit(*c); c++);
	}
	return val;
}

short term(void)
{
	skpspc();
	short f = factor();
	while (1) {
		skpspc();
		switch (*c) {
		case '*':
			c++;
			f *= factor();
			break;
		case '/':
			c++;
			f /= factor();
			break;
		default:
			return f;
		}
	}
}

short unsigned_expr(void)
{
	skpspc();
	short t = term();
	while (1) {
		skpspc();
		switch (*c) {
		case '+':
			c++;
			t += term();
			break;
		case '-':
			c++;
			t -= term();
			break;
		default:
			return t;
		}
	}
}

short expr(void)
{
	skpspc();
	short sgn = 1;
	switch (*c) {
	case '+':
		c++;
		break;
	case '-':
		sgn = -1;
		c++;
		break;
	}
	return sgn * unsigned_expr();
}

void gotoln(short ln)
{
	if (!ln)
		errexit(9);
	c = s;
	for (; atoi(c) < ln; skpto('\n'), c++);
}

void stmt(void);

void f_pr(void)
{
	skpspc();
	static int col = 0;
	while (1) {
		ENDLP:
		if (*c == '\n')
			break;
		if (*c == '"') {
			c++;
			char *start = c;
			skpto('"');
			int len = c - start;
			col += len;
			printf("%.*s", len, start);
		} else {
			short val = expr();
			col += printf("%hd", val);
		}
		for (;; c++) {
			switch (*c) {
			case ',': {
				int diff = 8 - col % 8;
				col += diff;
				printf("%*s", diff, "");
			}
				c++;
				skpspc();
				if (*c == '\n')
					return;
				goto ENDLP;
			case ';':
				c++;
				skpspc();
				if (*c == '\n')
					return;
				goto ENDLP;
			case ':':
				putchar(XOFF[0]);
				goto END;
			case '\n':
				goto END;
			}
		}
	}
	END:
	fputs("\r\n", stdout);
	col = 0;
}

/* Get new input line */
void inpln(char *buf)
{
	fputs("?" XON, stdout);
	fflush(stdout);
	fgets(buf, 255, stdin);
}

void f_in(void)
{
	skpspc();
	static char buf[255];
	static char *loc = &ZERO;
	while (*c != '\n') {
		if (!isupper(*c))
			errexit(104);
		char *id = c;
		if (*loc == '\0' || *loc == '\n') {
			inpln(buf);
			loc = buf;
		}
		for (c = loc; *c == ' '; c++)
			if (*c == '\n' || !*c) {
				c = buf;
				inpln(buf);
			}
		short val;
		if (isupper(*c)) {
			val = *c - '@';
			c++;
		} else {
			val = expr();
		}
		vars[*id - 'A'] = val;
		for (; *c == ' ' || *c == ','; c++);
		loc = c;
		c = id + 1;
		skpspc();
		if (*c != ',')
			errexit(123);
		c++;
		skpspc();
	}
}

void f_let(void) {
	skpspc();
	char id = *c;
	if (!isupper(*c))
		errexit(18);
	c++;
	skpspc();
	if (*c != '=')
		errexit(20);
	c++;
	skpspc();
	short val = expr();
	vars[id - 'A'] = val;
}

void f_goto(void)
{
	short ln = expr();
	gotoln(ln);
	if (atoi(c) != ln)
		errexit(37);
}

void f_gosub(void)
{
	if (callstack.i == CALLSTACK_SIZE - 1)
		errexit(188);
	short ln = expr();
	callstack.stack[callstack.i++] = c;
	gotoln(ln);
	if (atoi(c) != ln)
		errexit(46);
}

void f_ret(void)
{
	if (!callstack.i)
		errexit(133);
	c = callstack.stack[--callstack.i];
}

void f_if(void)
{
	skpspc();
	short e0 = expr();
	for (; *c < 60 || *c > 62; c++);
	char *op = c;
	c++;
	if (*c > 59 && *c < 63)
		c++;
	short e1 = expr();
	skpspc();
	int eq = strspccmp("THEN", c);
	if (eq >= 0)
		c += eq;
	skpspc();
	switch (*op) {
	case '=':
		if (e0 == e1) {
			stmt();
			return;
		}
		break;
	case '<':
		switch (*(op + 1)) {
		case '=':
			if (e0 <= e1) {
				stmt();
				return;
			}
			break;
		case '>':
			if (e0 != e1) {
				stmt();
				return;
			}
			break;
		default:
			if (e0 < e1) {
				stmt();
				return;
			}
		}
		break;
	case '>':
		switch (*(op + 1)) {
		case '=':
			if (e0 >= e1) {
				stmt();
				return;
			}
			break;
		case '<':
			if (e0 != e1) {
				stmt();
				return;
			}
			break;
		default:
			if (e0 > e1) {
				stmt();
				return;
			}
		}
		break;
	default:
		errexit(330);
	}
	skpto('\n');
}

void f_rem(void)
{
	skpto('\n');
}

void f_list(void)
{
	skpspc();
	if (*c == '\n') {
		fputs(s, stdout);
		return;
	}
	short ln0 = expr();
	if (ln0 == 0)
		errexit(154);
	skpspc();
	if (*c != ',') {
		char *ret = c;
		gotoln(ln0);
		char *lnstart = c;
		skpto('\n');
		int len = c - lnstart;
		printf("%.*s\n", len, lnstart);
		c = ret;
		return;
	}
	c++;
	short ln1 = expr();
	char *ret = c;
	gotoln(ln0);
	char *start = c;
	while(1) {
		if (atoi(c) >= ln1) {
			skpto('\n');
			break;
		}
		skpto('\n');
		c++;
	}
	int len = c - start;
	printf("%.*s\n", len, start);
	c = ret;
}

void f_end(void)
{
	free(s);
	exit(0);
}

struct {
	const char *s;
	void (*f)(void);
} KEYWORDS[] = {
	{"PRINT", f_pr},
	{"PR", f_pr},
	{"INPUT", f_in},
	{"LET", f_let},
	{"GOTO", f_goto},
	{"GOSUB", f_gosub},
	{"RETURN", f_ret},
	{"IF", f_if},
	{"REM", f_rem},
	{"LIST", f_list},
	{"END", f_end},
	{"", f_let},
};

void stmt(void)
{
	unsigned i;
	for(i = 0; i < KEYWORD_CNT; i++) {
		int len = strspccmp(KEYWORDS[i].s, c);
		if (len >= 0) {
#ifdef TRACE
			char *start = c;
			for (; *c != '\n' && *c; c++);
			printf("TRACE: %.*s\n", c - start, start);
			c = start;
#endif
			c += len;
			KEYWORDS[i].f();
			return;
		}
	}
}

void read_prog(char *path)
{
	FILE *f = fopen(path,"r");
	if (!f)
		errexit(8);
	fseek(f, 0, SEEK_END);
	long l = ftell(f);
	fseek(f, 0, SEEK_SET);
	s = malloc(l + 1);
	s[l] = '\0';
	fread(s, l, 1, f);
	fclose(f);
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	read_prog(argv[1]);
	c = s;
	while (1) {
		for (; !isupper(*c); c++);
		stmt();
	}
}
