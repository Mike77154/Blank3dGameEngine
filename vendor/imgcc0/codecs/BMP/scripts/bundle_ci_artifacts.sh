#!/bin/sh
set -eu

OUT_DIR=${1:?output directory required}
PROJECT=${2:?project name required}
TARGET=${3:?target name required}
HARNESS=${4:-}
CORPUS_DIR=${5:-}
MIN_CORPUS_DIR=${6:-}
shift 6 || true

PYTHON=${PYTHON:-python3}
RENDER_SCRIPT=${RENDER_SCRIPT:-./scripts/render_ci_bundle.py}

mkdir -p "$OUT_DIR"
mkdir -p "$OUT_DIR/logs"

copy_path() {
  if [ $# -eq 0 ]; then
    return 0
  fi
  SRC=$1
  if [ ! -e "$SRC" ]; then
    return 0
  fi
  BASE=$(basename "$SRC")
  if [ -d "$SRC" ]; then
    DEST="$OUT_DIR/extras/$BASE"
    mkdir -p "$OUT_DIR/extras"
    rm -rf "$DEST"
    cp -R "$SRC" "$DEST"
  else
    DEST="$OUT_DIR/logs/$BASE"
    cp "$SRC" "$DEST"
  fi
}

count_files() {
  if [ -d "$1" ]; then
    find "$1" -type f ! -name 'README*' ! -name '.*' | wc -l | awk '{print $1}'
  else
    printf '0\n'
  fi
}

if [ -n "$MIN_CORPUS_DIR" ] && [ -d "$MIN_CORPUS_DIR" ]; then
  mkdir -p "$OUT_DIR/minimized_corpus"
  cp -R "$MIN_CORPUS_DIR"/. "$OUT_DIR/minimized_corpus/"
fi

if [ -n "$CORPUS_DIR" ] && [ -d "$CORPUS_DIR" ]; then
  mkdir -p "$OUT_DIR/source_corpus"
  cp -R "$CORPUS_DIR"/. "$OUT_DIR/source_corpus/"
fi

if [ -n "$HARNESS" ] && [ -f "$HARNESS" ]; then
  mkdir -p "$OUT_DIR/bin"
  cp "$HARNESS" "$OUT_DIR/bin/"
fi

for path in "$@"; do
  copy_path "$path"
done

{
  printf 'project: %s\n' "$PROJECT"
  printf 'target: %s\n' "$TARGET"
  if command -v date >/dev/null 2>&1; then
    printf 'generated_utc: %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  fi
  printf 'harness: %s\n' "$HARNESS"
  printf 'source_corpus: %s\n' "$CORPUS_DIR"
  printf 'source_corpus_files: %s\n' "$(count_files "$CORPUS_DIR")"
  printf 'minimized_corpus: %s\n' "$MIN_CORPUS_DIR"
  printf 'minimized_corpus_files: %s\n' "$(count_files "$MIN_CORPUS_DIR")"
} > "$OUT_DIR/manifest.txt"

if [ -f "$RENDER_SCRIPT" ]; then
  "$PYTHON" "$RENDER_SCRIPT" --out-dir "$OUT_DIR" --project "$PROJECT" --target "$TARGET"
fi
