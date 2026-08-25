#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUTDIR="${OUTDIR:-$ROOT/differential/out}"
CC_BIN="${CC:-cc}"
CFLAGS_BIN="${CFLAGS:--std=c89 -Wall -Wextra -Werror -pedantic -O2}"
PNG_CFLAGS="$(pkg-config --cflags libpng 2>/dev/null || true)"
PNG_LIBS="$(pkg-config --libs libpng 2>/dev/null || echo '-lpng -lz')"

mkdir -p "$OUTDIR"

# shellcheck disable=SC2086
$CC_BIN $CFLAGS_BIN -DPNG_DEC_USE_ZLIB -I"$ROOT" $PNG_CFLAGS \
  "$ROOT/test_differential_libpng.c" "$ROOT"/png_*.c $PNG_LIBS -lz \
  -o "$OUTDIR/test_differential_libpng"

echo "built $OUTDIR/test_differential_libpng"
