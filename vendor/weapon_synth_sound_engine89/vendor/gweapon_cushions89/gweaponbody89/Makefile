CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: build/libgweaponbody89.a build/smoke

build:
	mkdir -p build

build/gweaponbody89.o: src/gweaponbody89.c include/gweaponbody89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gweaponbody89.c -o $@

build/libgweaponbody89.a: build/gweaponbody89.o
	$(AR) rcs $@ $<

build/smoke.o: tests/smoke.c include/gweaponbody89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/smoke.c -o $@

build/smoke: build/gweaponbody89.o build/smoke.o
	$(CC) $(CFLAGS) $^ -o $@

test: build/smoke
	./build/smoke

clean:
	rm -rf build

.PHONY: all test clean
