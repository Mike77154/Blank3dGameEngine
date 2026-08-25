#!/bin/sh
set -eu

HARNESS=${1:-./tests/fuzz_bmp_libfuzzer}
SRC=${2:-tests/fuzz_corpus}
DST=${3:-tests/fuzz_corpus_min}
CONTROL_FILE=${4:-$DST/.merge-control}

if [ ! -x "$HARNESS" ]; then
  echo "error: harness not executable: $HARNESS" >&2
  exit 1
fi
if [ ! -d "$SRC" ]; then
  echo "error: source corpus directory not found: $SRC" >&2
  exit 1
fi

mkdir -p "$DST"
rm -f "$CONTROL_FILE"
"$HARNESS" -merge=1 -merge_control_file="$CONTROL_FILE" "$DST" "$SRC"
rm -f "$CONTROL_FILE"
printf 'minimized corpus written to %s\n' "$DST"
