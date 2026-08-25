CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2
SRC = src/gf_arena.c src/gf_outline.c src/gf_bitmap.c src/gf_raster.c src/gf_svg.c src/gf_gff.c src/gf_shape.c src/gf_hb_compat.c src/gf_freetype_compat.c src/gf_hintvm.c src/gf_fontmake.c src/gf_transform.c src/gf_layout.c src/gf_svg_path.c

all: demo_glyphwife

demo_glyphwife: $(SRC) demo/main.c
	$(CC) $(CFLAGS) -o demo_glyphwife $(SRC) demo/main.c

run: demo_glyphwife
	./demo_glyphwife

clean:
	rm -f demo_glyphwife *.pbm *.svg *.gff
