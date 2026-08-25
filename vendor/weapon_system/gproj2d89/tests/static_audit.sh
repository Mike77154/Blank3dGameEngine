#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
FILES="$ROOT/include/gproj2d89.h $ROOT/src/gproj2d89.c"

if grep -En '\b(malloc|calloc|realloc|free)[[:space:]]*\(' $FILES; then
    echo "forbidden allocator call found" >&2
    exit 1
fi
if grep -En '\b(float|double|long[[:space:]]+double)\b' $FILES; then
    echo "non-integer numeric type found" >&2
    exit 1
fi
if grep -En '//|/\*[^*]*(malloc|calloc|realloc|free|float|double)' $FILES; then
    echo "audit keyword found in source comments" >&2
    exit 1
fi

echo "STATIC AUDIT PASSED"
