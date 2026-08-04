CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: build/libgballisticcrack89.a build/smoke

build:
	mkdir -p build

build/gballisticcrack89.o: src/gballisticcrack89.c include/gballisticcrack89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gballisticcrack89.c -o $@

build/libgballisticcrack89.a: build/gballisticcrack89.o
	$(AR) rcs $@ $<

build/smoke.o: tests/smoke.c include/gballisticcrack89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/smoke.c -o $@

build/smoke: build/gballisticcrack89.o build/smoke.o
	$(CC) $(CFLAGS) $^ -o $@

test: build/smoke
	./build/smoke

clean:
	rm -rf build

.PHONY: all test clean
