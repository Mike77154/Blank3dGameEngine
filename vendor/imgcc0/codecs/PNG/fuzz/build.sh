#!/bin/sh
set -eu

ROOT="${SRC:-$(pwd)}"
OUTDIR="${OUT:-$ROOT/fuzz/out/oss-fuzz}"
CC_BIN="${CC:-clang}"
CFLAGS_BIN="${CFLAGS:--O1 -g0 -fno-omit-frame-pointer}"
LIBFUZZ_BIN="${LIB_FUZZING_ENGINE:--fsanitize=fuzzer}"
C89_FLAGS="-std=c89 -pedantic -Wall -Wextra -Wno-unused-function"
TARGETS="png_fuzz_decode_full png_fuzz_decode_incremental png_fuzz_apng_decode png_fuzz_encode_png png_fuzz_encode_apng png_fuzz_unknown_chunks png_fuzz_convert_formats png_fuzz_adam7 png_fuzz_metadata"

mkdir -p "$OUTDIR"

for target in $TARGETS; do
  "$CC_BIN" $CFLAGS_BIN $C89_FLAGS -DPNG_DEC_USE_ZLIB -I"$ROOT" -I"$ROOT/fuzz" \
    "$ROOT/fuzz/$target.c" "$ROOT"/png_*.c $LIBFUZZ_BIN -lz -o "$OUTDIR/$target"
  if [ -f "$ROOT/fuzz/png.dict" ]; then
    cp "$ROOT/fuzz/png.dict" "$OUTDIR/$target.dict"
  fi
done

python3 - "$ROOT" "$OUTDIR" <<'PY2'
import sys, os, zipfile
root, outdir = sys.argv[1], sys.argv[2]
corpus_root = os.path.join(root, 'fuzz', 'corpus')
if not os.path.isdir(corpus_root):
    raise SystemExit(0)
for target in os.listdir(corpus_root):
    tdir = os.path.join(corpus_root, target)
    if not os.path.isdir(tdir):
        continue
    zpath = os.path.join(outdir, '%s_seed_corpus.zip' % target)
    with zipfile.ZipFile(zpath, 'w', compression=zipfile.ZIP_DEFLATED) as zf:
        for name in sorted(os.listdir(tdir)):
            path = os.path.join(tdir, name)
            if os.path.isfile(path):
                zf.write(path, arcname=name)
PY2
