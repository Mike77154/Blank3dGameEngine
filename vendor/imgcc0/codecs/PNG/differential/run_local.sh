#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUTDIR="${OUTDIR:-$ROOT/differential/out}"

"$ROOT/differential/build_local.sh"

set --
while IFS= read -r rel || [ -n "$rel" ]; do
  case "$rel" in
    ''|'#'*)
      continue
      ;;
  esac
  set -- "$@" "$ROOT/$rel"
done < "$ROOT/differential/corpus_static_pngs.txt"

"$OUTDIR/test_differential_libpng" "$@"
