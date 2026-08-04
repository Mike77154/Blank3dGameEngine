#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
STATUS=0

for DIR in include src examples tests; do
    if grep -R -n -E '\b(malloc|calloc|realloc|free)\b|\b(float|double)\b' "$ROOT/$DIR"; then
        STATUS=1
    fi
done

if [ "$STATUS" -ne 0 ]; then
    echo "audit failed"
    exit 1
fi

echo "audit OK"
