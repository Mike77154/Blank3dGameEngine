CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ideps

all: libgscopepresets89.a

libgscopepresets89.a: gscopepresets89.o
	$(AR) rcs $@ $^

gscopepresets89.o: src/gscopepresets89.c src/gscopepresets89_expanded_shapes.inc src/gscopepresets89_expanded_registry.inc include/gscopepresets89.h deps/gscopevector89.h deps/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscopepresets89.c -o $@

demo_presets: src/gscopepresets89.c examples/demo_presets.c ../gscopevector89/src/gscopevector89.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopevector89/include -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c ../gscopevector89/src/gscopevector89.c src/gscopepresets89.c examples/demo_presets.c -o $@

test_all_presets: src/gscopepresets89.c tests/test_all_presets.c ../gscopevector89/src/gscopevector89.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopevector89/include -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c ../gscopevector89/src/gscopevector89.c src/gscopepresets89.c tests/test_all_presets.c -o $@

demo: demo_presets

test: test_all_presets
	./test_all_presets

clean:
	rm -f *.o *.a demo_presets demo_presets.exe test_all_presets test_all_presets.exe tests/verify_pack192 tests/verify_pack192.exe preview/gscope_preview_generator preview/gscope_preview_generator.exe

.PHONY: all demo clean test

preview_generator: src/gscopepresets89.c src/gscopepresets89_expanded_shapes.inc src/gscopepresets89_expanded_registry.inc preview/gscope_preview_runtime.c preview/gscope_preview_main.c
	$(CC) $(CFLAGS) src/gscopepresets89.c preview/gscope_preview_runtime.c preview/gscope_preview_main.c -o preview/gscope_preview_generator

preview_ppm: preview_generator
	mkdir -p generated_preview
	./preview/gscope_preview_generator

.PHONY: preview_generator preview_ppm

verify_192: src/gscopepresets89.c src/gscopepresets89_expanded_shapes.inc src/gscopepresets89_expanded_registry.inc preview/gscope_preview_runtime.c tests/verify_pack192.c
	$(CC) $(CFLAGS) src/gscopepresets89.c preview/gscope_preview_runtime.c tests/verify_pack192.c -o tests/verify_pack192
	./tests/verify_pack192

.PHONY: verify_192
