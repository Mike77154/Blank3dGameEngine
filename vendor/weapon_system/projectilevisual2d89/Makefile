CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude

all: test

test_projectilevisual2d89: src/projectilevisual2d89.c tests/test_projectilevisual2d89.c include/projectilevisual2d89.h
	$(CC) $(CFLAGS) src/projectilevisual2d89.c tests/test_projectilevisual2d89.c -o $@

test: test_projectilevisual2d89
	./test_projectilevisual2d89

clean:
	rm -f test_projectilevisual2d89 *.o *.a

.PHONY: all test clean
