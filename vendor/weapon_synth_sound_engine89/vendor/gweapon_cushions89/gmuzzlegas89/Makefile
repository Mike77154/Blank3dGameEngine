CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: build/libgmuzzlegas89.a build/smoke

build:
	mkdir -p build

build/gmuzzlegas89.o: src/gmuzzlegas89.c include/gmuzzlegas89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gmuzzlegas89.c -o $@

build/libgmuzzlegas89.a: build/gmuzzlegas89.o
	$(AR) rcs $@ $<

build/smoke.o: tests/smoke.c include/gmuzzlegas89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/smoke.c -o $@

build/smoke: build/gmuzzlegas89.o build/smoke.o
	$(CC) $(CFLAGS) $^ -o $@

test: build/smoke
	./build/smoke

clean:
	rm -rf build

.PHONY: all test clean
