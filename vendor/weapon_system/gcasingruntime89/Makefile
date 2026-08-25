CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
NAME = gcasingruntime89

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/gcasingruntime89.c tests/test_gcasingruntime89.c -o test_gcasingruntime89
	./test_gcasingruntime89

clean:
	rm -f test_gcasingruntime89
