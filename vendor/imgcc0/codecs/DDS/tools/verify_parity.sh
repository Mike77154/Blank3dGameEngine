#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
CC=${CC:-cc}
tmp="${TMPDIR:-/tmp}/gdds_parity_$$"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/out"

"$CC" -std=c89 -pedantic-errors -Iinclude \
  src/gdds_common.c src/gdds_parse.c src/gdds_mips.c src/gdds_mipgen.c \
  src/gdds_srgb.c src/gdds_bc.c src/gdds_decode.c src/gdds_encode.c src/gdds_normal.c \
  tools/parity_harness.c -o "$tmp/parity_harness"

"$tmp/parity_harness" "$tmp/out"
count=0
for expected in corpus/parity_expected/*; do
    name=$(basename "$expected")
    cmp "$expected" "$tmp/out/$name"
    count=$((count + 1))
done

actual=$(find "$tmp/out" -type f | wc -l | tr -d ' ')
if [ "$actual" -ne "$count" ]; then
    echo "parity output count mismatch: expected $count, got $actual" >&2
    exit 1
fi

echo "byte-parity corpus: $count/$count artifacts identical"
