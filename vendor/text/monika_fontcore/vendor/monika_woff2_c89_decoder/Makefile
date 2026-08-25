CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude
AR ?= ar

CORE_OBJS = build/w2f_core.o build/w2f_brotli_stub.o

all: build libw2f.a w2f_cli_static test_core test_hmtx

build:
	mkdir -p build

build/w2f_core.o: src/w2f_core.c include/w2f.h include/w2f_types.h include/w2f_config.h | build
	$(CC) $(CFLAGS) -c src/w2f_core.c -o $@

build/w2f_brotli_stub.o: src/w2f_brotli_stub.c include/w2f.h | build
	$(CC) $(CFLAGS) -c src/w2f_brotli_stub.c -o $@

build/w2f_brotli_google_static.o: src/w2f_brotli_google_static.c include/w2f.h | build
	$(CC) $(CFLAGS) -DW2F_USE_GOOGLE_BROTLI -c src/w2f_brotli_google_static.c -o $@

libw2f.a: $(CORE_OBJS)
	$(AR) rcs $@ $(CORE_OBJS)

w2f_cli_static: examples/w2f_cli_static.c libw2f.a
	$(CC) $(CFLAGS) examples/w2f_cli_static.c libw2f.a -o $@

w2f_cli_static_brotli: examples/w2f_cli_static.c build/w2f_core.o build/w2f_brotli_stub.o build/w2f_brotli_google_static.o
	$(CC) $(CFLAGS) -DW2F_USE_GOOGLE_BROTLI examples/w2f_cli_static.c build/w2f_core.o build/w2f_brotli_stub.o build/w2f_brotli_google_static.o -lbrotlidec -lbrotlicommon -o $@

test_core: tests/test_w2f_core_copybrotli.c libw2f.a
	$(CC) $(CFLAGS) tests/test_w2f_core_copybrotli.c libw2f.a -o $@

test_hmtx: tests/test_w2f_hmtx_transform.c libw2f.a
	$(CC) $(CFLAGS) tests/test_w2f_hmtx_transform.c libw2f.a -o $@

check: test_core test_hmtx
	./test_core
	./test_hmtx

check-brotli: w2f_cli_static_brotli
	./w2f_cli_static_brotli tests/tiny.woff2 tests/tiny_out.ttf
	./w2f_cli_static_brotli tests/comp.woff2 tests/comp_out.ttf

clean:
	rm -rf build libw2f.a w2f_cli_static w2f_cli_static_brotli test_core test_hmtx
