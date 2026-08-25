#!/bin/sh
set -eu
DEST=${1:-./.cache/libwebp-test-data}
SHA=06ddd96e276c2c638a72d39d3c0f340afd61978c
URL="https://chromium.googlesource.com/webm/libwebp-test-data/+archive/${SHA}.tar.gz"
mkdir -p "$DEST"
if command -v curl >/dev/null 2>&1; then
  curl -L "$URL" | tar xz -C "$DEST"
elif command -v wget >/dev/null 2>&1; then
  wget -O- "$URL" | tar xz -C "$DEST"
else
  echo "necesitas curl o wget" >&2
  exit 1
fi
printf '%s
' "Corpus oficial descargado en $DEST"
