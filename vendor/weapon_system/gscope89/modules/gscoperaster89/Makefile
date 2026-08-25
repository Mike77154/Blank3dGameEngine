CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ideps

all: libgscoperaster89.a

libgscoperaster89.a: gscoperaster89.o
	$(AR) rcs $@ $^

gscoperaster89.o: src/gscoperaster89.c include/gscoperaster89.h deps/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscoperaster89.c -o $@

demo_raster: src/gscoperaster89.c examples/demo_raster.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c src/gscoperaster89.c examples/demo_raster.c -o $@

demo: demo_raster

clean:
	rm -f *.o *.a demo_raster demo_raster.exe

.PHONY: all demo clean
