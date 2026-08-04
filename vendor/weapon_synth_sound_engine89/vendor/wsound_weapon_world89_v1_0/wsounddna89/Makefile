CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsounddna89.a smoke

libwsounddna89.a: build/wsounddna89.o
	$(AR) rcs $@ $^

build/wsounddna89.o: src/wsounddna89.c include/wsounddna89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsounddna89.c -o $@

smoke: tests/smoke.c libwsounddna89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsounddna89.a -o smoke
	./smoke

clean:
	rm -rf build libwsounddna89.a smoke
