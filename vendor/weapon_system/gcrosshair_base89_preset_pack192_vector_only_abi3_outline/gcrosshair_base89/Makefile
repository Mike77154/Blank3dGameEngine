CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

LIB = libgcrosshair_base89.a
OBJ = gcrosshair_base89.o

all: $(LIB) test_base example_preset example_catalog example_abi3

$(OBJ): src/gcrosshair_base89.c include/gcrosshair_base89.h include/gcrosshair89_types.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_base89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_base: tests/test_base.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_base.c $(LIB) -o test_base

example_preset: examples/example_preset.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_preset.c $(LIB) -o example_preset

example_catalog: examples/example_catalog.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_catalog.c $(LIB) -o example_catalog

example_abi3: examples/example_abi3.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_abi3.c $(LIB) -o example_abi3

test: test_base
	./test_base

clean:
	rm -f $(OBJ) $(LIB) test_base test_base.exe \
	      example_preset example_preset.exe \
	      example_catalog example_catalog.exe \
      example_abi3 example_abi3.exe

.PHONY: all test clean
