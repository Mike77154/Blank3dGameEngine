CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundricochet89.a smoke

libwsoundricochet89.a: build/wsoundricochet89.o
	$(AR) rcs $@ $^

build/wsoundricochet89.o: src/wsoundricochet89.c include/wsoundricochet89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundricochet89.c -o $@

smoke: tests/smoke.c libwsoundricochet89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundricochet89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundricochet89.a smoke
