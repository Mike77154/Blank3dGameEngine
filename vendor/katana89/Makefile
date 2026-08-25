CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -std=c89 -Iinclude
OBJ = src/katana89.o src/katana89_obj.o

all: generate_all custom_provider

src/katana89.o: src/katana89.c include/katana89.h
	$(CC) $(CFLAGS) -c $< -o $@

src/katana89_obj.o: src/katana89_obj.c include/katana89_obj.h include/katana89.h
	$(CC) $(CFLAGS) -c $< -o $@

generate_all: $(OBJ) examples/generate_all.c
	$(CC) $(CFLAGS) examples/generate_all.c $(OBJ) -o $@

custom_provider: src/katana89.o examples/custom_provider.c
	$(CC) $(CFLAGS) examples/custom_provider.c src/katana89.o -o $@

generate: generate_all
	./generate_all

clean:
	rm -f src/*.o generate_all custom_provider generated/*.obj generated/*.mtl


test: all
	$(CC) $(CFLAGS) tests/test_katana89.c src/katana89.o -o tests/test_katana89
	./tests/test_katana89
