#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

if grep -RniE '\b(malloc|calloc|realloc|free)[[:space:]]*\(' include src examples tests; then
  echo "forbidden heap call found" >&2
  exit 1
fi
if grep -RniE '(^|[;,(])[[:space:]]*(float|double)[[:space:]*]' include src examples tests; then
  echo "forbidden floating type found" >&2
  exit 1
fi

make clean
make CFLAGS='-O2 -Wall -Wextra -Werror -pedantic -std=c89 -Iinclude'
cc -O2 -Wall -Wextra -Werror -pedantic -std=c89 -Iinclude \
  tests/test_katana89.c src/katana89.o -o tests/test_katana89
./tests/test_katana89
./custom_provider
./generate_all >/dev/null
printf '%s\n' 'protocol check: PASS'
