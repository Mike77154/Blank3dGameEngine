CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Werror
INCLUDES = -Iinclude

.PHONY: all test clean
all: test

test:
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_actor_system89.c src/actor_system89.c -o test_actor_system89
	./test_actor_system89

clean:
	rm -f test_actor_system89
