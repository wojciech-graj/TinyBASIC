# TinyBASIC Interpreter

A TinyBASIC Interpreter.

The interpreter can run programs following the specifications described in the [TINY BASIC User Manual by Tom Pittman](http://www.ittybittycomputers.com/IttyBitty/TinyBasic/TBuserMan.htm), with the following limitations:
- Numeric literals cannot contain spaces
- `CLEAR`, `RUN`, and `USR` keywords are not implemented
- Not all errors are checked for

## Usage

The source file has to be compiled into a binary, e.g.
```
gcc tbasic.c -o tbasic
```
To get the smallest binary (approx. 11KB):
```
clang tbasic.c -o tbasic -Os -s -z norelro -z noseparate-code -flto
```
Then simply execute, passing the BASIC source file as a command-line argument

The following macros can be defined to enable optional features:
- `PRINT_ERROR`: Print error messages
- `TRACE`: Print each source line prior to its execution

## License

```
Copyright (c) 2022 Wojciech Graj
Licensed under the MIT license: https://opensource.org/licenses/MIT
Permission is granted to use, copy, modify, and redistribute the work.
Full license information available in the project LICENSE file.
```
