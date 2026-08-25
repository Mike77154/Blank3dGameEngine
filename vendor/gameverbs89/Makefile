CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
I = -Iinclude
all: test
build:
	mkdir -p build
build/gameverbs89.o: src/gameverbs89.c include/gameverbs89.h | build
	$(CC) $(CFLAGS) $(I) -c $< -o $@
build/test_gameverbs89: tests/test_gameverbs89.c build/gameverbs89.o | build
	$(CC) $(CFLAGS) $(I) $^ -o $@
test: build/test_gameverbs89
	./build/test_gameverbs89
clean:
	rm -rf build
.PHONY: all test clean
