#!/bin/sh
set -eu
make clean
make
./demo_glyphwife
test -f out_text_AVfiS.pbm
test -f out_S_from_svg_path.svg
test -f demo_font.gff
echo smoke ok
