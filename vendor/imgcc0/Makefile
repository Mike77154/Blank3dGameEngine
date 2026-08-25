CC ?= cc
AR ?= ar
PYTHON ?= python3
CFLAGS ?= -std=c89 -pedantic-errors -O2 -w
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

# imgcc0 is built and consumed as a static archive inside Blank3D.
# BMP/PCX public headers otherwise select __declspec(dllimport) on MinGW,
# producing _imp__bmp_* / _imp__pcx_* references that cannot resolve
# against the objects embedded in libimgcc0.a. Keep the static ABI explicit.
IMGCC0_STATIC_DEFS = -DBMP_STATIC_DEFINE -DPCX_STATIC_DEFINE

INCLUDES = \
	-Icompat \
	-Iinclude \
	-Icodecs/JPEG/include -Icodecs/JPEG/src \
	-Icodecs/TGA \
	-Icodecs/GIF/include -Icodecs/GIF/src \
	-Icodecs/QOI/include -Icodecs/QOI/src \
	-Icodecs/WEBP/src -Icodecs/WEBP \
	-Icodecs/TIFF/include -Icodecs/TIFF/src \
	-Icodecs/PNG \
	-Icodecs/DDS/include \
	-Icodecs/BMP -Icodecs/BMP/include \
	-Icodecs/PCX -Icodecs/PCX/include \
	-Icodecs/ZRAGF -Icodecs/ZRAGF/zragf_deflate -Icodecs/ZRAGF/zragf_inflate \
	-Icodecs/PSD/include -Icodecs/PSD/src

CORE_SOURCES = src/imgcc0.c src/imgcc0_zragf_bridge.c
JPEG_SOURCES = codecs/JPEG/src/c89jpeg.c
TGA_SOURCES = codecs/TGA/gafnyf_tga.c
GIF_SOURCES = $(wildcard codecs/GIF/src/*.c)
QOI_SOURCES = $(wildcard codecs/QOI/src/*.c)
WEBP_SOURCES = $(wildcard codecs/WEBP/src/dec/*.c) $(wildcard codecs/WEBP/src/demux/*.c) $(wildcard codecs/WEBP/src/utils/*.c)
TIFF_SOURCES = $(wildcard codecs/TIFF/src/*.c)
PNG_SOURCES = \
	codecs/PNG/png_mem89.c codecs/PNG/png_fixed89.c codecs/PNG/png_crc.c \
	codecs/PNG/png_filters.c codecs/PNG/png_chunks.c codecs/PNG/png_parser.c \
	codecs/PNG/png_render.c codecs/PNG/png_stubs.c \
	codecs/PNG/png_decoder.c codecs/PNG/png_apng.c codecs/PNG/png_apng_progressive.c \
	codecs/PNG/png_encoder.c
DDS_SOURCES = $(wildcard codecs/DDS/src/*.c)
BMP_SOURCES = $(wildcard codecs/BMP/bmp/*.c)
PCX_SOURCES = \
	codecs/PCX/pcx_chunk.c codecs/PCX/pcx_parser.c codecs/PCX/pcx_render.c \
	codecs/PCX/pcx_decoder.c codecs/PCX/pcx_encoder.c codecs/PCX/pcx_report.c

PSD_SOURCES = \
	codecs/PSD/src/psd89_core.c \
	codecs/PSD/src/psd89_util.c \
	codecs/PSD/src/psd89_stdio.c \
	codecs/PSD/src/psd89_zip.c \
	codecs/PSD/src/psd89_read.c \
	codecs/PSD/src/psd89_write.c \
	codecs/PSD/src/psd89_compose.c \
	codecs/PSD/src/psd89_vector_bezier.c \
	codecs/PSD/src/psd89_mask_global_tags.c \
	codecs/PSD/src/psd89_zip_diag.c \
	codecs/PSD/src/psd89_advanced.c

ZRAGF_SOURCES = \
	codecs/ZRAGF/zragflib_internal.c \
	codecs/ZRAGF/zragf_bitio.c \
	codecs/ZRAGF/zragf_compress.c \
	codecs/ZRAGF/zragf_decompress.c \
	codecs/ZRAGF/zragf_deflate_rfc1951.c \
	codecs/ZRAGF/zragf_frame.c \
	codecs/ZRAGF/zragf_huff.c \
	codecs/ZRAGF/zragf_lz.c \
	codecs/ZRAGF/zragf_stream.c \
	codecs/ZRAGF/zragf_api.c \
	codecs/ZRAGF/zragf_workspace.c \
	codecs/ZRAGF/zragf_protocol89_mem.c \
	codecs/ZRAGF/zragf_deflate/deflate_blocks.c \
	codecs/ZRAGF/zragf_deflate/deflate_core.c \
	codecs/ZRAGF/zragf_deflate/deflate_cost.c \
	codecs/ZRAGF/zragf_deflate/deflate_emit.c \
	codecs/ZRAGF/zragf_deflate/deflate_fixed.c \
	codecs/ZRAGF/zragf_deflate/deflate_hash.c \
	codecs/ZRAGF/zragf_deflate/deflate_huffman.c \
	codecs/ZRAGF/zragf_deflate/deflate_huffman_shared.c \
	codecs/ZRAGF/zragf_deflate/deflate_lazy.c \
	codecs/ZRAGF/zragf_deflate/deflate_matcher.c \
	codecs/ZRAGF/zragf_deflate/deflate_plan.c \
	codecs/ZRAGF/zragf_deflate/deflate_rle.c \
	codecs/ZRAGF/zragf_deflate/deflate_stream.c \
	codecs/ZRAGF/zragf_deflate/deflate_tokens.c \
	codecs/ZRAGF/zragf_inflate/inflate_core.c \
	codecs/ZRAGF/zragf_inflate/inflate_huffman.c \
	codecs/ZRAGF/zragf_inflate/inflate_shared.c \
	codecs/ZRAGF/zragf_inflate/inflate_stream.c

LIB_SOURCES = $(CORE_SOURCES) $(JPEG_SOURCES) $(TGA_SOURCES) $(GIF_SOURCES) $(QOI_SOURCES) $(WEBP_SOURCES) $(TIFF_SOURCES) $(PNG_SOURCES) $(DDS_SOURCES) $(BMP_SOURCES) $(PCX_SOURCES) $(PSD_SOURCES) $(ZRAGF_SOURCES)
LIB_OBJECTS = $(addprefix build/obj/,$(LIB_SOURCES:.c=.o))
DEMO_OBJECTS = build/obj/demos/demo_cli.o

all: protocol-audit build/libimgcc0.a build/demo_cli

build/libimgcc0.a: $(LIB_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(LIB_OBJECTS)

build/demo_cli: $(DEMO_OBJECTS) build/libimgcc0.a
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(DEMO_OBJECTS) build/libimgcc0.a $(LDLIBS)

build/obj/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(IMGCC0_STATIC_DEFS) $(CFLAGS) $(INCLUDES) -c $< -o $@

protocol-audit:
	$(PYTHON) scripts/audit_imgcc0_protocol.py

regression: all
	$(PYTHON) scripts/test_strict_samples.py

adapter-regression: all
	$(PYTHON) scripts/test_vendor_adapters.py
	$(PYTHON) scripts/test_psd_adapter.py

vendor-verify:
	$(PYTHON) scripts/verify_vendor_swap.py

zragf-audit:
	$(PYTHON) codecs/ZRAGF/tools/protocol89_audit.py

ZLIB_COMPAT_TEST = build/test_zragf_zlib_compat_img
TIFF_ZRAGF_TEST = build/test_tifx_zragf

$(ZLIB_COMPAT_TEST): tests/test_zragf_zlib_compat_img.c build/libimgcc0.a
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Icodecs/ZRAGF $< build/libimgcc0.a -o $@

$(TIFF_ZRAGF_TEST): codecs/TIFF/tests/test_tifx.c build/libimgcc0.a
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $< build/libimgcc0.a -o $@

zragf-image-compat: all $(ZLIB_COMPAT_TEST) $(TIFF_ZRAGF_TEST)
	./$(ZLIB_COMPAT_TEST)
	cd codecs/TIFF && ../../$(TIFF_ZRAGF_TEST)

clean:
	rm -rf build demo_assets/dumped demo_assets/report

.PHONY: all clean protocol-audit regression adapter-regression vendor-verify zragf-audit zragf-image-compat
