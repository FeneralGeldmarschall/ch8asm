#!/usr/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
BOLD='\033[1m'

exec=$1
cases=$(ls test/asm | cut -d'.' -f 1)
num_total=$(ls test/asm | wc -l)

passed=true
num_passed=0

echo -e "${BOLD}TEST RUN:${NC}"
for i in $cases; do
  echo -e "\tTesting instruction ${i}..."
  eval "./$exec" "test/asm/${i}.asm" "2>&1" "|" "sed 's/^/\t/'"
  if [[ $(diff -q "test/bin/${i}.bin" out.ch8) ]]; then
    # diff "bin/${i}.bin" out.ch8
    echo -e "\t${RED}TEST FAILED${NC}"
  else
    echo -e "\t${GREEN}TEST PASSED${NC}"
    ((num_passed=num_passed+1))
  fi
  echo ""
done

rm out.ch8

echo -e "${BOLD}TEST SUMMARY:${NC}"
echo -e "\t${num_passed}/${num_total} tests passed"