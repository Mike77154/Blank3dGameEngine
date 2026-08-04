CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundaction89.a smoke

libwsoundaction89.a: build/wsoundaction89.o
	$(AR) rcs $@ $^

build/wsoundaction89.o: src/wsoundaction89.c include/wsoundaction89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundaction89.c -o $@

smoke: tests/smoke.c libwsoundaction89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundaction89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundaction89.a smoke
