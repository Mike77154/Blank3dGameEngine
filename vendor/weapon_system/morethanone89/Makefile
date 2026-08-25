CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

.PHONY: all test clean
all: test

test: tests/test_morethanone89
	./tests/test_morethanone89

tests/test_morethanone89: src/morethanone89.c tests/test_morethanone89.c include/morethanone89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/morethanone89.c tests/test_morethanone89.c -o $@

clean:
	rm -f tests/test_morethanone89
