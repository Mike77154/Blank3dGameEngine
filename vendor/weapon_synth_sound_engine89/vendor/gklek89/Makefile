CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2
INCLUDES = -Iinclude

all: build/libgklek89.a build/render_gklek89 build/test_smoke

build:
	mkdir -p build audio

build/gklek89.o: src/gklek89.c include/gklek89.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/libgklek89.a: build/gklek89.o
	$(AR) rcs $@ $^

build/render_gklek89.o: demo/render_gklek89.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/render_gklek89: build/render_gklek89.o build/libgklek89.a
	$(CC) $(CFLAGS) $^ -o $@

build/test_smoke.o: tests/smoke.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/test_smoke: build/test_smoke.o build/libgklek89.a
	$(CC) $(CFLAGS) $^ -o $@

test: all
	./build/test_smoke
	./build/render_gklek89

clean:
	rm -rf build audio/*.wav
