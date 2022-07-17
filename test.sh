#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
PASS=0
FAIL=0
for f in tests/test_*.BAS
do
    ./tbasic $f > temp.txt
    nm=${f#"tests/test_"}
    nm=${nm%".BAS"}
    ou="tests/out_${nm}.txt"
    cmp -s $ou temp.txt
    if [ $? = 0 ]
    then
        echo -e "${GREEN}${nm}"
	PASS=$((PASS+1))
    else
        echo -e "${RED}${nm}${NC}"
        echo "Expected:"
        cat $ou
        echo "Actual:"
        cat temp.txt
	FAIL=$((FAIL+1))
    fi
done
echo -e "${GREEN}${PASS} passed ${RED}${FAIL} failed${NC}"
rm temp.txt
