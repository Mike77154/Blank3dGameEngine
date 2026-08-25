#!/bin/sh
set -eu
CC=${CC:-gcc}
SRC='png_mem89.c png_fixed89.c png_crc.c png_filters.c png_chunks.c png_parser.c png_render.c png_stubs.c png_zlib.c png_decoder.c png_apng.c png_apng_progressive.c png_encoder.c'
$CC -std=c89 -pedantic -Wall -Wextra -Werror -O2 -DPNG_DEC_USE_ZLIB -I. $SRC tests/roundtrip_smoke.c -lz -o tests/roundtrip_smoke
./tests/roundtrip_smoke
$CC -std=c89 -pedantic -Wall -Wextra -Werror -O2 -DPNG_DEC_USE_ZLIB -I. $SRC tests/repeat_smoke.c -lz -o tests/repeat_smoke
./tests/repeat_smoke tests/corpus/png/rgba16_gama_adam7.png
