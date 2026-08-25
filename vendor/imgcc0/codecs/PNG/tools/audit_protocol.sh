#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
bad=0

check() {
  label=$1
  pattern=$2
  if grep -RInE "$pattern" --include='*.c' --include='*.h' .; then
    echo "FAIL: $label"
    bad=1
  else
    echo "OK: $label"
  fi
}

check 'heap allocation calls' '\b(malloc|calloc|realloc|free)[[:space:]]*\('
check 'floating types' '\b(double|float)\b'
check 'explicit 64-bit integer types' '\b(long[[:space:]]+long|int64_t|uint64_t|int_least64_t|uint_least64_t|int_fast64_t|uint_fast64_t)\b'
check 'LP64-sensitive explicit declarations' '\b(size_t|long)\b'
check 'long integer literal suffixes' '([0-9]|0x[0-9A-Fa-f]+)([uU]?[lL]{1,2}|[lL]{1,2}[uU]?)\b'
check 'floating numeric literals' '(^|[^A-Za-z0-9_"])(([0-9]+\.[0-9]+)|([0-9]+[eE][+-]?[0-9]+))'

exit "$bad"
