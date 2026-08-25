CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude
AR ?= ar
ARFLAGS ?= rcs
LDLIBS ?= -lz

SRC = src/psd89_core.c src/psd89_util.c src/psd89_stdio.c src/psd89_zip.c src/psd89_read.c src/psd89_write.c src/psd89_compose.c src/psd89_vector_bezier.c src/psd89_mask_global_tags.c src/psd89_zip_diag.c src/psd89_advanced.c
OBJ = $(SRC:.c=.o)
LIB = libpsd89.a
EXAMPLES = examples/file_roundtrip examples/layer_mask_roundtrip examples/group_roundtrip examples/advanced_roundtrip examples/fidelity_roundtrip examples/fidelity_roundtrip_v23
TESTS = tests/test_v12 tests/test_v13 tests/test_v14 tests/test_v15 tests/test_v16 tests/test_v17 tests/test_v18 tests/test_v20 tests/test_v21 tests/test_v22 tests/test_v23 tests/corpus_runner

all: $(LIB) $(EXAMPLES) $(TESTS)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

examples/file_roundtrip: examples/file_roundtrip.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

examples/layer_mask_roundtrip: examples/layer_mask_roundtrip.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

examples/group_roundtrip: examples/group_roundtrip.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

examples/advanced_roundtrip: examples/advanced_roundtrip.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

examples/fidelity_roundtrip: examples/fidelity_roundtrip.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

examples/fidelity_roundtrip_v23: examples/fidelity_roundtrip_v23.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v12: tests/test_v12.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v13: tests/test_v13.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v14: tests/test_v14.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v15: tests/test_v15.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v16: tests/test_v16.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v17: tests/test_v17.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v18: tests/test_v18.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v20: tests/test_v20.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v21: tests/test_v21.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v22: tests/test_v22.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/test_v23: tests/test_v23.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

tests/corpus_runner: tests/corpus_runner.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB) $(LDLIBS)

test: all
	./tests/test_v12
	./tests/test_v13
	./tests/test_v14
	./tests/test_v15
	./tests/test_v16
	./tests/test_v17
	./tests/test_v18
	./tests/test_v20
	./tests/test_v21
	./tests/test_v22
	./tests/test_v23
	./tests/corpus_runner

clean:
	rm -f $(OBJ) $(LIB) $(EXAMPLES) $(TESTS)

.PHONY: all test clean
