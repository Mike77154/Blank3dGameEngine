CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
INCLUDES = -Igweaponbody89/include -Igmuzzlegas89/include -Igballisticcrack89/include -Iglatetail89/include -Igcinemathump89/include -Idemo
OBJS = gweaponbody89/build/gweaponbody89.o gmuzzlegas89/build/gmuzzlegas89.o gballisticcrack89/build/gballisticcrack89.o glatetail89/build/glatetail89.o gcinemathump89/build/gcinemathump89.o

all: libs build/render_demo

libs:
	$(MAKE) -C gweaponbody89
	$(MAKE) -C gmuzzlegas89
	$(MAKE) -C gballisticcrack89
	$(MAKE) -C glatetail89
	$(MAKE) -C gcinemathump89

build:
	mkdir -p build

build/render_demo.o: demo/render_demo.c demo/demo_wav.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c demo/render_demo.c -o $@

build/render_demo: build/render_demo.o libs
	$(CC) $(CFLAGS) build/render_demo.o $(OBJS) -o $@

samples: build/render_demo
	mkdir -p audio
	./build/render_demo

test: libs
	$(MAKE) -C gweaponbody89 test
	$(MAKE) -C gmuzzlegas89 test
	$(MAKE) -C gballisticcrack89 test
	$(MAKE) -C glatetail89 test
	$(MAKE) -C gcinemathump89 test

clean:
	rm -rf build audio/*.wav
	$(MAKE) -C gweaponbody89 clean
	$(MAKE) -C gmuzzlegas89 clean
	$(MAKE) -C gballisticcrack89 clean
	$(MAKE) -C glatetail89 clean
	$(MAKE) -C gcinemathump89 clean

.PHONY: all libs samples test clean

build/augment_sequence.o: demo/augment_sequence.c demo/demo_wav.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c demo/augment_sequence.c -o $@

build/augment_sequence: build/augment_sequence.o libs
	$(CC) $(CFLAGS) build/augment_sequence.o $(OBJS) -o $@
