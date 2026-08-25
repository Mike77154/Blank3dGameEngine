#!/bin/sh
set -eu

HARNESS=${1:-./tests/fuzz_pcx_stdin}
SRC=${2:-tests/fuzz_corpus_min_afl}
DST=${3:-tests/fuzz_corpus_min_afl_tmin}

if ! command -v afl-tmin >/dev/null 2>&1; then
  echo "error: afl-tmin not found in PATH" >&2
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
for f in "$SRC"/*; do
  [ -f "$f" ] || continue
  out="$DST/$(basename "$f")"
  afl-tmin -i "$f" -o "$out" -- "$HARNESS" @@ >/dev/null 2>&1
  printf 'minimized %s -> %s\n' "$f" "$out"
done
