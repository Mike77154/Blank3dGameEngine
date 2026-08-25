CC=gcc
CFLAGS=-std=c89 -O2 -Wall -Wextra -Iinclude
CORE=src/glyphwifettf.c
SVG=src/glyphwifettf_svg.c

all: ttf_dump ttf_svg_dump ttf_svg_dump_plus ttf_pgm_raster ttf_layout_demo ttf_atlas_ascii ttf_measure_demo ttf_text_bitmap

ttf_dump: $(CORE) examples/ttf_dump.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_dump.c -o ttf_dump

ttf_svg_dump: $(CORE) $(SVG) examples/ttf_svg_dump.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) $(SVG) examples/ttf_svg_dump.c -o ttf_svg_dump

ttf_svg_dump_plus: $(CORE) $(SVG) examples/ttf_svg_dump_plus.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) $(SVG) examples/ttf_svg_dump_plus.c -o ttf_svg_dump_plus

ttf_pgm_raster: $(CORE) examples/ttf_pgm_raster.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_pgm_raster.c -o ttf_pgm_raster

ttf_layout_demo: $(CORE) examples/ttf_layout_demo.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_layout_demo.c -o ttf_layout_demo

ttf_atlas_ascii: $(CORE) examples/ttf_atlas_ascii.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_atlas_ascii.c -o ttf_atlas_ascii

ttf_measure_demo: $(CORE) examples/ttf_measure_demo.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_measure_demo.c -o ttf_measure_demo

ttf_text_bitmap: $(CORE) examples/ttf_text_bitmap.c include/glyphwifettf.h
	$(CC) $(CFLAGS) $(CORE) examples/ttf_text_bitmap.c -o ttf_text_bitmap

strict_core:
	$(CC) -std=c89 -pedantic -DGWT_USE_LONG_LONG=0 -Wall -Wextra -Iinclude -c $(CORE) -o glyphwifettf_core_strict.o

clean:
	rm -f ttf_dump ttf_svg_dump ttf_svg_dump_plus ttf_pgm_raster ttf_layout_demo ttf_atlas_ascii ttf_measure_demo ttf_text_bitmap *.o *.pgm *.svg
