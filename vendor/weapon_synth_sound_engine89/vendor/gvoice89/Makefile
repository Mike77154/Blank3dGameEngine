CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude

all: build/libgvoice89.a build/test_smoke build/virtual_256_demo

build/gvoice89.o: src/gvoice89.c include/gvoice89.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/gvoice89.c -o $@

build/libgvoice89.a: build/gvoice89.o
	$(AR) rcs $@ $^

build/test_smoke: tests/smoke.c build/libgvoice89.a
	$(CC) $(CFLAGS) tests/smoke.c build/libgvoice89.a -o $@

build/virtual_256_demo: demo/virtual_256_demo.c build/libgvoice89.a
	$(CC) $(CFLAGS) demo/virtual_256_demo.c build/libgvoice89.a -o $@

test: all
	./build/test_smoke

clean:
	rm -rf build/*
