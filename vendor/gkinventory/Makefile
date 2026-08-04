CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
AR ?= ar
ARFLAGS ?= rcs

CORE_OBJS = build/gkinv.o build/gkinv_fixed.o

all: build/libgkinv.a build/test_gkinv build/example_basic build/example_grid build/example_hooks

build:
	mkdir -p build

build/gkinv.o: src/gkinv.c include/gkinv.h include/gkinv_fixed.h include/gkinv_types.h include/gkinv_config.h | build
	$(CC) $(CFLAGS) -c src/gkinv.c -o build/gkinv.o

build/gkinv_fixed.o: src/gkinv_fixed.c include/gkinv_fixed.h include/gkinv_types.h include/gkinv_config.h | build
	$(CC) $(CFLAGS) -c src/gkinv_fixed.c -o build/gkinv_fixed.o

build/libgkinv.a: $(CORE_OBJS)
	$(AR) $(ARFLAGS) build/libgkinv.a $(CORE_OBJS)

build/test_gkinv: tests/test_gkinv.c build/libgkinv.a
	$(CC) $(CFLAGS) tests/test_gkinv.c build/libgkinv.a -o build/test_gkinv

build/example_basic: examples/example_basic.c build/libgkinv.a
	$(CC) $(CFLAGS) examples/example_basic.c build/libgkinv.a -o build/example_basic

build/example_grid: examples/example_grid.c build/libgkinv.a
	$(CC) $(CFLAGS) examples/example_grid.c build/libgkinv.a -o build/example_grid

build/example_hooks: examples/example_hooks.c build/libgkinv.a
	$(CC) $(CFLAGS) examples/example_hooks.c build/libgkinv.a -o build/example_hooks

check: build/test_gkinv
	./build/test_gkinv

clean:
	rm -rf build

.PHONY: all check clean
