#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUTDIR="${OUTDIR:-$ROOT/fuzz/out/standalone}"
CC_BIN="${CC:-cc}"
CFLAGS_BIN="${CFLAGS:--O1}"
C89_FLAGS="-std=c89 -pedantic -Wall -Wextra -Werror -Wno-unused-function"
TARGETS="png_fuzz_decode_full png_fuzz_decode_incremental png_fuzz_apng_decode png_fuzz_encode_png png_fuzz_encode_apng png_fuzz_unknown_chunks png_fuzz_convert_formats png_fuzz_adam7 png_fuzz_metadata"

mkdir -p "$OUTDIR"

for target in $TARGETS; do
  "$CC_BIN" $CFLAGS_BIN $C89_FLAGS -DPNG_DEC_USE_ZLIB -I"$ROOT" -I"$ROOT/fuzz" \
    "$ROOT/fuzz/$target.c" "$ROOT/fuzz/standalone_driver.c" "$ROOT"/png_*.c -lz \
    -o "$OUTDIR/$target"

  for seed in "$ROOT/fuzz/corpus/$target"/*; do
    [ -f "$seed" ] || continue
    "$OUTDIR/$target" < "$seed" >/dev/null
  done
  echo "PASS $target"
done
