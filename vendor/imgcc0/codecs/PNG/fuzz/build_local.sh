#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUTDIR="${OUTDIR:-$ROOT/fuzz/out/local}"
MODE="${MODE:-libfuzzer}"
SANITIZER="${SANITIZER:-address}"
CC_BIN="${CC:-clang}"
CFLAGS_BIN="${CFLAGS:--O0 -g0 -fno-omit-frame-pointer}"
C89_FLAGS="-std=c89 -pedantic -Wall -Wextra -Wno-unused-function"
TARGETS="png_fuzz_decode_full png_fuzz_decode_incremental png_fuzz_apng_decode png_fuzz_encode_png png_fuzz_encode_apng png_fuzz_unknown_chunks png_fuzz_convert_formats png_fuzz_adam7 png_fuzz_metadata"

mkdir -p "$OUTDIR"

if [ "$MODE" = "libfuzzer" ]; then
  if [ "$SANITIZER" = "memory" ]; then
    FUZZ_FLAGS="-fsanitize=fuzzer,memory -fsanitize-memory-track-origins"
  else
    FUZZ_FLAGS="-fsanitize=fuzzer,$SANITIZER"
  fi
  for target in $TARGETS; do
    "$CC_BIN" $CFLAGS_BIN $C89_FLAGS -DPNG_DEC_USE_ZLIB -I"$ROOT" -I"$ROOT/fuzz" \
      "$ROOT/fuzz/$target.c" "$ROOT"/png_*.c $FUZZ_FLAGS -lz -o "$OUTDIR/$target"
  done
elif [ "$MODE" = "afl" ]; then
  CC_BIN="${CC:-afl-clang-fast}"
  AFL_SAN=""
  if [ "$SANITIZER" = "address" ]; then
    AFL_SAN="-fsanitize=address"
  elif [ "$SANITIZER" = "undefined" ]; then
    AFL_SAN="-fsanitize=undefined"
  fi
  for target in $TARGETS; do
    "$CC_BIN" $CFLAGS_BIN $C89_FLAGS $AFL_SAN -DPNG_DEC_USE_ZLIB -I"$ROOT" -I"$ROOT/fuzz" \
      "$ROOT/fuzz/$target.c" "$ROOT/fuzz/standalone_driver.c" "$ROOT"/png_*.c -lz -o "$OUTDIR/${target}_afl"
  done
else
  echo "MODE must be libfuzzer or afl" >&2
  exit 1
fi

echo "built fuzzers in $OUTDIR"
