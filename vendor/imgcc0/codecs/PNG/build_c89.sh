#!/bin/sh
set -eu
CC=${CC:-gcc}
CFLAGS=${CFLAGS:--O2 -Wall -Wextra -Werror}
$CC -std=c89 -pedantic $CFLAGS -DPNG_DEC_USE_ZLIB -I. -c \
  png_mem89.c png_fixed89.c png_crc.c png_filters.c png_chunks.c \
  png_parser.c png_render.c png_stubs.c png_zlib.c png_decoder.c \
  png_apng.c png_apng_progressive.c png_encoder.c
ar rcs libtiny_png_c89.a \
  png_mem89.o png_fixed89.o png_crc.o png_filters.o png_chunks.o \
  png_parser.o png_render.o png_stubs.o png_zlib.o png_decoder.o \
  png_apng.o png_apng_progressive.o png_encoder.o
printf '%s\n' 'built libtiny_png_c89.a (link consumers with -lz)'
