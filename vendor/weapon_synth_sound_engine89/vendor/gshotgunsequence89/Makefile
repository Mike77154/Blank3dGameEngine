CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2

INCLUDES = -Iinclude \
 -Ivendor/gpaah89/include -Ivendor/gpump89/include \
 -Ivendor/chuecka89/include -Ivendor/shotpumpkin89/include -Ivendor/gweaponfoley89/include \
 -Ivendor/gklek89/include \
 -Ivendor/gweaponbody89/include -Ivendor/gmuzzlegas89/include \
 -Ivendor/gballisticcrack89/include -Ivendor/glatetail89/include \
 -Ivendor/gcinemathump89/include

OBJS = build/gshotgunsequence89.o \
 build/gpaah89.o build/gpump89.o build/chuecka89.o build/shotpumpkin89.o build/gklek89.o build/gweaponfoley89.o \
 build/gweaponbody89.o build/gmuzzlegas89.o build/gballisticcrack89.o \
 build/glatetail89.o build/gcinemathump89.o

all: build/libgshotgunsequence89.a build/render_shotgun_sequence build/test_smoke

build:
	mkdir -p build audio

build/gshotgunsequence89.o: src/gshotgunsequence89.c include/gshotgunsequence89.h vendor/gklek89/include/gklek89.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/gpaah89.o: vendor/gpaah89/src/gpaah89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gpump89.o: vendor/gpump89/src/gpump89.c vendor/gpump89/src/gpump89_grains.inc | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/chuecka89.o: vendor/chuecka89/src/chuecka89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/shotpumpkin89.o: vendor/shotpumpkin89/src/shotpumpkin89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gklek89.o: vendor/gklek89/src/gklek89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gweaponfoley89.o: vendor/gweaponfoley89/src/gweaponfoley89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gweaponbody89.o: vendor/gweaponbody89/src/gweaponbody89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gmuzzlegas89.o: vendor/gmuzzlegas89/src/gmuzzlegas89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gballisticcrack89.o: vendor/gballisticcrack89/src/gballisticcrack89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/glatetail89.o: vendor/glatetail89/src/glatetail89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
build/gcinemathump89.o: vendor/gcinemathump89/src/gcinemathump89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/libgshotgunsequence89.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

build/render_shotgun_sequence.o: demo/render_shotgun_sequence.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/render_shotgun_sequence: build/render_shotgun_sequence.o build/libgshotgunsequence89.a
	$(CC) $(CFLAGS) $^ -o $@

build/test_smoke.o: tests/smoke.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/test_smoke: build/test_smoke.o build/libgshotgunsequence89.a
	$(CC) $(CFLAGS) $^ -o $@

samples: all
	./build/render_shotgun_sequence

test: all
	./build/test_smoke
	./build/render_shotgun_sequence
	python3 tools/validate_wav.py audio/*.wav

clean:
	rm -rf build audio/*.wav
