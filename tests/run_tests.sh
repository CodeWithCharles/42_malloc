#!/bin/bash
GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'; NC='\033[0m'
ROOT="$(cd "$(dirname "$0")" && pwd)"
DIR="$ROOT/build"
fail=0
shopt -s nullglob

for src in "$ROOT"/test_*.c; do
    name="$(basename "$src" .c)"
    bin="$DIR/$name"
    if [ ! -x "$bin" ]; then
        printf "  ${YELLOW}MISS${NC}  %s (source sans binaire)\n" "$name"
        fail=1
        continue
    fi
    if "$bin" >/dev/null 2>&1; then
        printf "  ${GREEN}PASS${NC}  %s\n" "$name"
    else
        printf "  ${RED}FAIL${NC}  %s\n" "$name"
        "$bin"
        fail=1
    fi
done

for bin in "$DIR"/test_*; do
    [ -x "$bin" ] || continue
    name="$(basename "$bin")"
    if [ ! -f "$ROOT/$name.c" ]; then
        printf "  ${YELLOW}ORPH${NC}  %s (binaire sans source)\n" "$name"
        fail=1
    fi
done

exit $fail