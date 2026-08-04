CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundroom89.a smoke

libwsoundroom89.a: build/wsoundroom89.o
	$(AR) rcs $@ $^

build/wsoundroom89.o: src/wsoundroom89.c include/wsoundroom89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundroom89.c -o $@

smoke: tests/smoke.c libwsoundroom89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundroom89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundroom89.a smoke
