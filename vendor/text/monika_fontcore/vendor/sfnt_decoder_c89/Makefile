CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude
AR ?= ar

all: libsfnt_decoder.a sfntdump test_build

libsfnt_decoder.a: src/sfnt_decoder.o
	$(AR) rcs $@ src/sfnt_decoder.o

src/sfnt_decoder.o: src/sfnt_decoder.c include/sfnt_decoder.h
	$(CC) $(CFLAGS) -c src/sfnt_decoder.c -o src/sfnt_decoder.o

sfntdump: examples/sfntdump.c libsfnt_decoder.a
	$(CC) $(CFLAGS) examples/sfntdump.c libsfnt_decoder.a -o sfntdump

test_build: tests/test_build.c libsfnt_decoder.a
	$(CC) $(CFLAGS) tests/test_build.c libsfnt_decoder.a -o test_build

check: test_build
	./test_build

clean:
	rm -f src/*.o libsfnt_decoder.a sfntdump test_build

.PHONY: all check clean
