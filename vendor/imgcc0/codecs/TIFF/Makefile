CC ?= cc
AR ?= ar
RANLIB ?= ranlib
CFLAGS ?= -std=c89 -O2 -Wall -Wextra -pedantic
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lz

LIB = libtifx.a
OBJ = src/tifx.o src/tifx_dispatch.o src/tifx_bigtiff_parse.o src/tifx_bigtiff_write.o
TEST_BIN = tests/test_tifx
EXAMPLE_BINS = examples/write_gradient examples/write_compressed examples/write_bilevel examples/write_multipage examples/write_subifd_tree examples/inspect_tiff

.PHONY: all clean test

all: $(LIB) $(TEST_BIN) $(EXAMPLE_BINS)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^
	$(RANLIB) $@

src/tifx.o: src/tifx.c include/tifx.h src/tifx_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/tifx_dispatch.o: src/tifx_dispatch.c include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/tifx_bigtiff_parse.o: src/tifx_bigtiff_parse.c include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/tifx_bigtiff_write.o: src/tifx_bigtiff_write.c include/tifx.h src/tifx_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BIN): tests/test_tifx.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_tifx.c $(LIB) $(LDLIBS) -o $@

examples/write_gradient: examples/write_gradient.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/write_gradient.c $(LIB) $(LDLIBS) -o $@

examples/write_compressed: examples/write_compressed.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/write_compressed.c $(LIB) $(LDLIBS) -o $@

examples/write_bilevel: examples/write_bilevel.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/write_bilevel.c $(LIB) $(LDLIBS) -o $@

examples/write_multipage: examples/write_multipage.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/write_multipage.c $(LIB) $(LDLIBS) -o $@

examples/write_subifd_tree: examples/write_subifd_tree.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/write_subifd_tree.c $(LIB) $(LDLIBS) -o $@

examples/inspect_tiff: examples/inspect_tiff.c $(LIB) include/tifx.h
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/inspect_tiff.c $(LIB) $(LDLIBS) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(LIB) $(TEST_BIN) $(EXAMPLE_BINS)
