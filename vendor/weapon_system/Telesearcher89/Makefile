CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
all: test
test:
	$(CC) $(CFLAGS) -Iinclude src/telesearcher89.c tests/test_telesearcher89.c -o test_telesearcher89
	./test_telesearcher89
clean:
	rm -f test_telesearcher89
