CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: build/libglatetail89.a build/smoke

build:
	mkdir -p build

build/glatetail89.o: src/glatetail89.c include/glatetail89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/glatetail89.c -o $@

build/libglatetail89.a: build/glatetail89.o
	$(AR) rcs $@ $<

build/smoke.o: tests/smoke.c include/glatetail89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/smoke.c -o $@

build/smoke: build/glatetail89.o build/smoke.o
	$(CC) $(CFLAGS) $^ -o $@

test: build/smoke
	./build/smoke

clean:
	rm -rf build

.PHONY: all test clean
