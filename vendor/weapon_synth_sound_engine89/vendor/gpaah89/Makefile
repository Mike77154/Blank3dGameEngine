CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

CORE_OBJ = build/gpaah89.o
TOOL_OBJ = build/gpaah_render.o
TEST_OBJ = build/gpaah89_stress.o

all: build/gpaah_render build/gpaah89_stress

build:
	mkdir -p build

build/gpaah89.o: src/gpaah89.c include/gpaah89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gpaah89.c -o $@

build/gpaah_render.o: tools/gpaah_render.c include/gpaah89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tools/gpaah_render.c -o $@

build/gpaah89_stress.o: tests/gpaah89_stress.c include/gpaah89.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c tests/gpaah89_stress.c -o $@

build/gpaah_render: $(CORE_OBJ) $(TOOL_OBJ)
	$(CC) $(CFLAGS) $(CORE_OBJ) $(TOOL_OBJ) -o $@

build/gpaah89_stress: $(CORE_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(CORE_OBJ) $(TEST_OBJ) -o $@

samples: build/gpaah_render
	mkdir -p samples/base_presets
	./build/gpaah_render

test: build/gpaah89_stress
	./build/gpaah89_stress

clean:
	rm -rf build samples/base_presets/*.wav

.PHONY: all samples test clean
