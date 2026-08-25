CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
all: test

test: tests/test_bulletcircle89
	./tests/test_bulletcircle89

tests/test_bulletcircle89: src/bulletcircle89.c tests/test_bulletcircle89.c include/bulletcircle89.h
	$(CC) $(CFLAGS) -Iinclude src/bulletcircle89.c tests/test_bulletcircle89.c -o $@

clean:
	rm -f tests/test_bulletcircle89
