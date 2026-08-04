CC ?= gcc
AR ?= ar
CLANG ?= clang
LLVM_AR ?= llvm-ar
CFLAGS ?= -O2 -Wall -Wextra -pedantic -std=c89
INCLUDES = -Iinclude
CORE_OBJ = build/chuecka89.o
WAV_OBJ = build/chuecka89_wav.o
LIB = build/libchuecka89.a
CORE32_OBJ = build/chuecka89_32.o
LIB32 = build/libchuecka89_i386.a
WIN32_OBJ = build/chuecka89_win32.o
WIN32_LIB = build/libchuecka89_win32.a

all: $(LIB) render_presets render_atom_fx size_report test_smoke

build:
	mkdir -p build

$(CORE_OBJ): src/chuecka89.c include/chuecka89.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c src/chuecka89.c -o $(CORE_OBJ)

$(WAV_OBJ): src/chuecka89_wav.c include/chuecka89_wav.h include/chuecka89.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c src/chuecka89_wav.c -o $(WAV_OBJ)

$(LIB): $(CORE_OBJ)
	$(AR) rcs $(LIB) $(CORE_OBJ)

render_presets: $(LIB) $(WAV_OBJ) demo/render_presets.c
	$(CC) $(CFLAGS) $(INCLUDES) demo/render_presets.c $(WAV_OBJ) $(LIB) -o $@

render_atom_fx: $(LIB) $(WAV_OBJ) demo/render_atom_fx.c
	$(CC) $(CFLAGS) $(INCLUDES) demo/render_atom_fx.c $(WAV_OBJ) $(LIB) -o $@

size_report: $(LIB) demo/size_report.c
	$(CC) $(CFLAGS) $(INCLUDES) demo/size_report.c $(LIB) -o $@

test_smoke: $(LIB) tests/test_smoke.c
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_smoke.c $(LIB) -o $@

$(CORE32_OBJ): src/chuecka89.c include/chuecka89.h | build
	$(CC) $(CFLAGS) -m32 $(INCLUDES) -c src/chuecka89.c -o $(CORE32_OBJ)

$(LIB32): $(CORE32_OBJ)
	$(AR) rcs $(LIB32) $(CORE32_OBJ)

core32: $(LIB32)

$(WIN32_OBJ): src/chuecka89.c include/chuecka89.h | build
	$(CLANG) --target=i686-w64-windows-gnu $(CFLAGS) $(INCLUDES) -c src/chuecka89.c -o $(WIN32_OBJ)

$(WIN32_LIB): $(WIN32_OBJ)
	$(LLVM_AR) rcs $(WIN32_LIB) $(WIN32_OBJ)

win32: $(WIN32_LIB)

preview: render_presets
	./render_presets

test: test_smoke
	./test_smoke

clean:
	rm -rf build
	rm -f render_presets render_atom_fx size_report test_smoke
	rm -f $(CORE32_OBJ) $(LIB32) $(WIN32_OBJ) $(WIN32_LIB)
	rm -f wav/*.wav

.PHONY: all core32 win32 preview test clean
