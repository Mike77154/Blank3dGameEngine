#!/bin/sh
set -eu
DEST=${1:-./.cache/libwebp}
REF=${LIBWEBP_REF:-HEAD}
URL="https://chromium.googlesource.com/webm/libwebp/+archive/${REF}.tar.gz"
mkdir -p "$DEST/src"
if command -v curl >/dev/null 2>&1; then
  curl -L "$URL" | tar xz -C "$DEST/src"
elif command -v wget >/dev/null 2>&1; then
  wget -O- "$URL" | tar xz -C "$DEST/src"
else
  echo "necesitas curl o wget" >&2
  exit 1
fi
if [ -f "$DEST/src/makefile.unix" ]; then
  (cd "$DEST/src" && make -f makefile.unix examples/dwebp)
  printf '%s
' "dwebp disponible en $DEST/src/examples/dwebp"
else
  echo "no encontré makefile.unix en la fuente descargada" >&2
  exit 1
fi
