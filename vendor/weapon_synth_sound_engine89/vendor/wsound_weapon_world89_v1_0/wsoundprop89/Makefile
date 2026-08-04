CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundprop89.a smoke

libwsoundprop89.a: build/wsoundprop89.o
	$(AR) rcs $@ $^

build/wsoundprop89.o: src/wsoundprop89.c include/wsoundprop89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundprop89.c -o $@

smoke: tests/smoke.c libwsoundprop89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundprop89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundprop89.a smoke
