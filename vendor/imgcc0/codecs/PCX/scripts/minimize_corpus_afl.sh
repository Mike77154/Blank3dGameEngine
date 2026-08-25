#!/bin/sh
set -eu

HARNESS=${1:-./tests/fuzz_pcx_stdin}
SRC=${2:-tests/fuzz_corpus}
DST=${3:-tests/fuzz_corpus_min_afl}

if ! command -v afl-cmin >/dev/null 2>&1; then
  echo "error: afl-cmin not found in PATH" >&2
  exit 127
fi
if [ ! -x "$HARNESS" ]; then
  echo "error: harness not executable: $HARNESS" >&2
  exit 1
fi
if [ ! -d "$SRC" ]; then
  echo "error: source corpus directory not found: $SRC" >&2
  exit 1
fi

rm -rf "$DST"
mkdir -p "$DST"
afl-cmin -i "$SRC" -o "$DST" -- "$HARNESS" @@
printf 'afl-cmin corpus written to %s\n' "$DST"
