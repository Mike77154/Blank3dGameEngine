CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: build/libgcinemathump89.a build/smoke

build:
	mkdir -p build

build/gcinemathump89.o: src/gcinemathump89.c include/gcinemathump89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcinemathump89.c -o $@

build/libgcinemathump89.a: build/gcinemathump89.o
	$(AR) rcs $@ $<

build/smoke.o: tests/smoke.c include/gcinemathump89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/smoke.c -o $@

build/smoke: build/gcinemathump89.o build/smoke.o
	$(CC) $(CFLAGS) $^ -o $@

test: build/smoke
	./build/smoke

clean:
	rm -rf build

.PHONY: all test clean
