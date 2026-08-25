CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCS = -Iinclude
all: test

test: build/test_assetroute89
	./build/test_assetroute89
build/test_assetroute89: src/assetroute89.c tests/test_assetroute89.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCS) $^ -o $@
clean:
	rm -rf build
