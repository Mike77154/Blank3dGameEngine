CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ideps

all: libgscopevector89.a

libgscopevector89.a: gscopevector89.o
	$(AR) rcs $@ $^

gscopevector89.o: src/gscopevector89.c include/gscopevector89.h deps/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscopevector89.c -o $@

demo_custom_reticle: src/gscopevector89.c examples/demo_custom_reticle.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c src/gscopevector89.c examples/demo_custom_reticle.c -o $@

demo: demo_custom_reticle

clean:
	rm -f *.o *.a demo_custom_reticle demo_custom_reticle.exe

.PHONY: all demo clean
