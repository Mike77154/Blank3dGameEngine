CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ideps
ROOT := ../..

all: libgscopepresets89.a

libgscopepresets89.a: gscopepresets89.o
	$(AR) rcs $@ $^

gscopepresets89.o: src/gscopepresets89.c include/gscopepresets89.h deps/gscopevector89.h deps/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscopepresets89.c -o $@

demo_presets: src/gscopepresets89.c examples/demo_presets.c ../gscopevector89/src/gscopevector89.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopevector89/include -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c ../gscopevector89/src/gscopevector89.c src/gscopepresets89.c examples/demo_presets.c -o $@

test_all_presets: src/gscopepresets89.c tests/test_all_presets.c ../gscopevector89/src/gscopevector89.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopevector89/include -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c ../gscopevector89/src/gscopevector89.c src/gscopepresets89.c tests/test_all_presets.c -o $@

verify_192_bin: src/gscopepresets89.c tests/verify_pack192.c ../gscopevector89/src/gscopevector89.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopevector89/include -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c ../gscopevector89/src/gscopevector89.c src/gscopepresets89.c tests/verify_pack192.c -o tests/verify_pack192

demo: demo_presets
	cd $(ROOT) && modules/gscopepresets89/demo_presets

test: test_all_presets verify_192_bin
	cd $(ROOT) && modules/gscopepresets89/test_all_presets
	cd $(ROOT) && modules/gscopepresets89/tests/verify_pack192

verify_192: verify_192_bin
	cd $(ROOT) && modules/gscopepresets89/tests/verify_pack192

clean:
	rm -f *.o *.a demo_presets demo_presets.exe test_all_presets test_all_presets.exe tests/verify_pack192 tests/verify_pack192.exe preview/gscope_preview_generator preview/gscope_preview_generator.exe

.PHONY: all demo clean test verify_192
