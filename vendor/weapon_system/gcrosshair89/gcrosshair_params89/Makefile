CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

LIB = libgcrosshair_params89.a
OBJ = gcrosshair_params89.o

all: $(LIB) test_params test_shapes example_style example_shapes

$(OBJ): src/gcrosshair_params89.c include/gcrosshair_params89.h include/gcrosshair89_types.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_params89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_params: tests/test_params.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_params.c $(LIB) -o test_params

test_shapes: tests/test_shapes.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_shapes.c $(LIB) -o test_shapes

example_style: examples/example_style.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_style.c $(LIB) -o example_style

example_shapes: examples/example_shapes.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_shapes.c $(LIB) -o example_shapes

test: test_params test_shapes
	./test_params
	./test_shapes

clean:
	rm -f $(OBJ) $(LIB) test_params test_params.exe test_shapes test_shapes.exe \
	      example_style example_style.exe example_shapes example_shapes.exe

.PHONY: all test clean
