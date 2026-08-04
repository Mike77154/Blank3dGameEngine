CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ivendor/gvoice89/include

all: build/libgweaponvoice89.a build/test_smoke build/stress_demo

build/gvoice89.o: vendor/gvoice89/src/gvoice89.c vendor/gvoice89/include/gvoice89.h
	mkdir -p build
	$(CC) $(CFLAGS) -c vendor/gvoice89/src/gvoice89.c -o $@

build/gweaponvoice89.o: src/gweaponvoice89.c include/gweaponvoice89.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/gweaponvoice89.c -o $@

build/libgweaponvoice89.a: build/gvoice89.o build/gweaponvoice89.o
	$(AR) rcs $@ $^

build/test_smoke: tests/smoke.c build/libgweaponvoice89.a
	$(CC) $(CFLAGS) tests/smoke.c build/libgweaponvoice89.a -o $@

build/stress_demo: demo/render_256_voice_weapon_scene.c build/libgweaponvoice89.a
	$(CC) $(CFLAGS) demo/render_256_voice_weapon_scene.c build/libgweaponvoice89.a -o $@

test: all
	./build/test_smoke

clean:
	rm -rf build/*
