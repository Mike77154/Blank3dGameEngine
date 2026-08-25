CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
all: test

test: tests/test_bulletinline89
	./tests/test_bulletinline89

tests/test_bulletinline89: src/bulletinline89.c tests/test_bulletinline89.c include/bulletinline89.h
	$(CC) $(CFLAGS) -Iinclude src/bulletinline89.c tests/test_bulletinline89.c -o $@

clean:
	rm -f tests/test_bulletinline89
