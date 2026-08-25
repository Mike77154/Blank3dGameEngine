CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
all: test

test: tests/test_bulletspin89
	./tests/test_bulletspin89

tests/test_bulletspin89: src/bulletspin89.c tests/test_bulletspin89.c include/bulletspin89.h
	$(CC) $(CFLAGS) -Iinclude src/bulletspin89.c tests/test_bulletspin89.c -o $@

clean:
	rm -f tests/test_bulletspin89
