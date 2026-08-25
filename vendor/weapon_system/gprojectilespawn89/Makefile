CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
NAME = gprojectilespawn89

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/gprojectilespawn89.c tests/test_gprojectilespawn89.c -o test_gprojectilespawn89
	./test_gprojectilespawn89

clean:
	rm -f test_gprojectilespawn89
