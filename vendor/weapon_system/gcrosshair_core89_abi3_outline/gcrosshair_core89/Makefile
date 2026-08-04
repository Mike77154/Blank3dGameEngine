CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

LIB = libgcrosshair_core89.a
OBJ = gcrosshair_core89.o

all: $(LIB) test_core test_abi3_shapes test_outline example_callbacks

$(OBJ): src/gcrosshair_core89.c include/gcrosshair_core89.h include/gcrosshair89_types.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_core89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_core: tests/test_core.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_core.c $(LIB) -o test_core

test_abi3_shapes: tests/test_abi3_shapes.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_abi3_shapes.c $(LIB) -o test_abi3_shapes

test_outline: tests/test_outline.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_outline.c $(LIB) -o test_outline

example_callbacks: examples/example_callbacks.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_callbacks.c $(LIB) -o example_callbacks

test: test_core test_abi3_shapes test_outline
	./test_core
	./test_abi3_shapes
	./test_outline

clean:
	rm -f $(OBJ) $(LIB) test_core test_core.exe test_abi3_shapes test_abi3_shapes.exe test_outline test_outline.exe example_callbacks example_callbacks.exe

.PHONY: all test clean
