CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2

INCLUDES = \
  -Iinclude \
  -Ivendor/sfnt_decoder_c89/include \
  -Ivendor/glyphwifettf_v0_4_3/include \
  -Ivendor/monika_otf_decoder_c89_interp/src \
  -Ivendor/monika_woff1_decoder_c89/src \
  -Ivendor/monika_woff2_c89_decoder/include \
  -Ivendor/gmyy_spritefont_decoder_v3 \
  -Ivendor/mugen_fnt_codec/include \
  -Ivendor/giffy_hb_c89_v0_20/include

CORE_SRC = \
  src/monika_fontcore.c \
  vendor/sfnt_decoder_c89/src/sfnt_decoder.c \
  vendor/glyphwifettf_v0_4_3/src/glyphwifettf.c \
  vendor/glyphwifettf_v0_4_3/src/glyphwifettf_svg.c \
  vendor/monika_otf_decoder_c89_interp/src/otf_c89.c \
  vendor/monika_woff1_decoder_c89/src/woff1_decoder.c \
  vendor/monika_woff1_decoder_c89/src/woff1_inflate.c \
  vendor/monika_woff2_c89_decoder/src/w2f_core.c \
  vendor/monika_woff2_c89_decoder/src/w2f_brotli_stub.c \
  vendor/monika_woff2_c89_decoder/src/w2f_brotli_google_static.c \
  vendor/gmyy_spritefont_decoder_v3/gmyy_spritefont.c \
  vendor/mugen_fnt_codec/src/mfont_core.c \
  vendor/mugen_fnt_codec/src/mfont_text.c \
  vendor/mugen_fnt_codec/src/mfont_util.c \
  vendor/mugen_fnt_codec/src/zlfont.c \
  vendor/mugen_fnt_codec/src/pcxfnt.c \
  vendor/mugen_fnt_codec/src/fntbump.c \
  vendor/mugen_fnt_codec/src/fntpng.c \
  vendor/mugen_fnt_codec/third_party/puff/puff.c \
  vendor/giffy_hb_c89_v0_20/src/giffy_hb.c

.PHONY: all clean proof proof-brotli proof-mugen

all: build/proof_png_demo

build:
	mkdir -p build

build/proof_png_demo: build $(CORE_SRC) examples/proof_png_demo.c include/monika_fontcore.h
	$(CC) $(CFLAGS) -DGWT_USE_LONG_LONG=0 $(INCLUDES) -o $@ examples/proof_png_demo.c $(CORE_SRC)

proof: build/proof_png_demo
	./build/proof_png_demo vendor/monika_woff2_c89_decoder/tests/tiny.ttf proof_fontcore.png > proof_output.txt

clean:
	rm -rf build proof_fontcore.png proof_output.txt

proof-brotli: build
	$(CC) $(CFLAGS) -DGWT_USE_LONG_LONG=0 -DMFC_USE_GOOGLE_BROTLI -DW2F_USE_GOOGLE_BROTLI $(INCLUDES) -o build/proof_png_demo_brotli examples/proof_png_demo.c $(CORE_SRC) -lbrotlidec -lbrotlicommon
	./build/proof_png_demo_brotli vendor/monika_woff2_c89_decoder/tests/tiny.ttf proof_fontcore.png > proof_output.txt


build/proof_mugen_fnt_png: build $(CORE_SRC) examples/proof_mugen_fnt_png.c include/monika_fontcore.h
	$(CC) $(CFLAGS) -DGWT_USE_LONG_LONG=0 $(INCLUDES) -o $@ examples/proof_mugen_fnt_png.c $(CORE_SRC)

proof-mugen: build/proof_mugen_fnt_png
	./build/proof_mugen_fnt_png tests/mugen/f-6x9.fnt proof_mugen_fnt.png > proof_mugen_fnt_output.txt

proof-rush-box: build/proof_mugen_fnt_png
	./build/proof_mugen_fnt_png tests/mugen/rush_box.fnt proof_rush_box.png > proof_rush_box_output.txt

proof-rush-fnt: build/proof_mugen_fnt_png
	./build/proof_mugen_fnt_png tests/mugen/rush_fnt.fnt proof_rush_fnt.png > proof_rush_fnt_output.txt

proof-rush: proof-rush-box proof-rush-fnt
