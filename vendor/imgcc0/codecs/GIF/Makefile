CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude -O2
AR ?= ar
ARFLAGS ?= rcs

LIB_SRCS = \
    src/giff_common.c \
    src/giff_decoder.c \
    src/giff_encoder.c \
    src/giff_lzw_dec.c \
    src/giff_lzw_enc.c \
    src/giff_palette.c \
    src/giff_anim.c

LIB_OBJS = $(LIB_SRCS:.c=.o)

all: lib/libgiff.a bin/giff_smoke

lib/libgiff.a: $(LIB_OBJS)
	mkdir -p lib
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

bin/giff_smoke: examples/smoke.o lib/libgiff.a
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ examples/smoke.o lib/libgiff.a

examples/smoke.o: examples/smoke.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

tests/test_smoke: tests/test_smoke.c lib/libgiff.a
	mkdir -p tests
	$(CC) $(CFLAGS) -o $@ tests/test_smoke.c lib/libgiff.a

tests/fuzz_like: tests/fuzz_like.c lib/libgiff.a
	mkdir -p tests
	$(CC) $(CFLAGS) -o $@ tests/fuzz_like.c lib/libgiff.a

test: tests/test_smoke
	./tests/test_smoke

fuzz: tests/fuzz_like
	./tests/fuzz_like

clean:
	rm -f src/*.o examples/*.o tests/test_smoke bin/giff_smoke lib/libgiff.a

.PHONY: all clean test fuzz
