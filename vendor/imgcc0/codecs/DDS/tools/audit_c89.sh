#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

bad='malloc|realloc|calloc|free|float|double|long[[:space:]]+long|uint64_t|int64_t|uintptr_t|size_t'
if grep -RInE "\\b($bad)\\b" --include='*.c' --include='*.h' .; then
    echo "forbidden C/H token found" >&2
    exit 1
fi
if grep -RInE '#include <stdint.h>|static[[:space:]]+inline|//|for[[:space:]]*\([^;]*\b(int|unsigned|gdds_)\b' --include='*.c' --include='*.h' .; then
    echo "C99-only construct found" >&2
    exit 1
fi

echo "C89/static/fixed-point source audit: ok"
