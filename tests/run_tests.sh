#!/bin/bash
GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
DIR="$(cd "$(dirname "$0")" && pwd)/build"
fail=0
for t in "$DIR"/test_*; do
    [ -x "$t" ] || continue
    if "$t" >/dev/null 2>&1; then
        printf "  ${GREEN}PASS${NC}  %s\n" "$(basename "$t")"
    else
        printf "  ${RED}FAIL${NC}  %s\n" "$(basename "$t")"; "$t"; fail=1
    fi
done
exit $fail