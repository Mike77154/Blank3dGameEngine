CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude -I../gcrosshair_core89/include

LIB = libgcrosshair_provider89.a
OBJ = gcrosshair_provider89.o
CORELIB = ../gcrosshair_core89/libgcrosshair_core89.a

all: $(LIB) test_provider example_external_provider example_standalone_ppm

$(OBJ): src/gcrosshair_provider89.c include/gcrosshair_provider89.h ../gcrosshair_core89/include/gcrosshair_core89.h ../gcrosshair_core89/include/gcrosshair89_types.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_provider89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_provider: tests/test_provider.c $(LIB) $(CORELIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_provider.c $(LIB) $(CORELIB) -o test_provider

example_external_provider: examples/example_external_provider.c $(LIB) $(CORELIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_external_provider.c $(LIB) $(CORELIB) -o example_external_provider

example_standalone_ppm: examples/example_standalone_ppm.c $(LIB) $(CORELIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_standalone_ppm.c $(LIB) $(CORELIB) -o example_standalone_ppm

test: test_provider
	./test_provider

clean:
	rm -f $(OBJ) $(LIB) test_provider test_provider.exe \
	      example_external_provider example_external_provider.exe \
	      example_standalone_ppm example_standalone_ppm.exe provider_standalone.ppm

.PHONY: all test clean
