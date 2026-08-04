CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude

.PHONY: all demos preview radial_preview test clean

all: demos

demos: ascii_demo v02_feature_demo super_preview_ppm

ascii_demo: src/gbar89.c include/gbar89.h examples/ascii_demo.c
	$(CC) $(CFLAGS) src/gbar89.c examples/ascii_demo.c -o ascii_demo

v02_feature_demo: src/gbar89.c include/gbar89.h examples/v02_feature_demo.c
	$(CC) $(CFLAGS) src/gbar89.c examples/v02_feature_demo.c -o v02_feature_demo

super_preview_ppm: src/gbar89.c include/gbar89.h examples/super_preview_ppm.c
	$(CC) $(CFLAGS) -O2 src/gbar89.c examples/super_preview_ppm.c -o super_preview_ppm

preview: super_preview_ppm
	mkdir -p preview_frames
	./super_preview_ppm

smoke_v02: src/gbar89.c include/gbar89.h tests/smoke_v02.c
	$(CC) $(CFLAGS) src/gbar89.c tests/smoke_v02.c -o smoke_v02

smoke_v03: src/gbar89.c include/gbar89.h tests/smoke_v03.c
	$(CC) $(CFLAGS) src/gbar89.c tests/smoke_v03.c -o smoke_v03

smoke_v04: src/gbar89.c include/gbar89.h tests/smoke_v04.c
	$(CC) $(CFLAGS) src/gbar89.c tests/smoke_v04.c -o smoke_v04

test: smoke_v02 smoke_v03 smoke_v04
	./smoke_v02
	./smoke_v03
	./smoke_v04

clean:
	rm -f ascii_demo v02_feature_demo super_preview_ppm radial_vertical_preview_ppm smoke_v02 smoke_v03 smoke_v04
	rm -f preview_raw.ppm preview_frames/frame_*.ppm

radial_vertical_preview_ppm: src/gbar89.c include/gbar89.h examples/radial_vertical_preview_ppm.c
	$(CC) $(CFLAGS) -O2 src/gbar89.c examples/radial_vertical_preview_ppm.c -o radial_vertical_preview_ppm

radial_preview: radial_vertical_preview_ppm
	mkdir -p radial_vertical_frames
	./radial_vertical_preview_ppm
