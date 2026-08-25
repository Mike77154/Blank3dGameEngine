CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

LIB = libgcrosshair_base89.a
OBJ = gcrosshair_base89.o gcrosshair_recipe89.o

all: $(LIB) test_base test_recipe_loader test_recipe_io_provider test_pre_ini_equivalence test_animation_recipes example_preset example_catalog example_abi3 example_recipe_root

gcrosshair_base89.o: src/gcrosshair_base89.c src/gcrosshair_recipe89_internal.h include/gcrosshair_base89.h include/gcrosshair89_types.h include/gcrosshair_recipe89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_base89.c -o gcrosshair_base89.o

gcrosshair_recipe89.o: src/gcrosshair_recipe89.c include/gcrosshair_base89.h include/gcrosshair89_types.h include/gcrosshair_recipe89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_recipe89.c -o gcrosshair_recipe89.o

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_base: tests/test_base.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_base.c $(LIB) -o test_base

test_recipe_loader: tests/test_recipe_loader.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_recipe_loader.c $(LIB) -o test_recipe_loader

test_recipe_io_provider: tests/test_recipe_io_provider.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_recipe_io_provider.c $(LIB) -o test_recipe_io_provider

test_pre_ini_equivalence: tests/test_pre_ini_equivalence.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_pre_ini_equivalence.c $(LIB) -o test_pre_ini_equivalence

test_animation_recipes: tests/test_animation_recipes.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_animation_recipes.c $(LIB) -o test_animation_recipes

example_preset: examples/example_preset.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_preset.c $(LIB) -o example_preset

example_catalog: examples/example_catalog.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_catalog.c $(LIB) -o example_catalog

example_abi3: examples/example_abi3.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_abi3.c $(LIB) -o example_abi3

example_recipe_root: examples/example_recipe_root.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_recipe_root.c $(LIB) -o example_recipe_root

test: test_base test_recipe_loader test_recipe_io_provider test_pre_ini_equivalence test_animation_recipes
	./test_base
	./test_recipe_loader
	./test_recipe_io_provider
	./test_pre_ini_equivalence
	./test_animation_recipes

clean:
	rm -f $(OBJ) $(LIB) test_base test_base.exe test_recipe_loader test_recipe_loader.exe test_recipe_io_provider test_recipe_io_provider.exe test_pre_ini_equivalence test_pre_ini_equivalence.exe test_animation_recipes test_animation_recipes.exe \
	      example_preset example_preset.exe example_catalog example_catalog.exe \
	      example_abi3 example_abi3.exe example_recipe_root example_recipe_root.exe

.PHONY: all test clean
