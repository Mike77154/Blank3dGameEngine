CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
NAME = gtrigger89

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/gtrigger89.c tests/test_gtrigger89.c -o test_gtrigger89
	./test_gtrigger89

clean:
	rm -f test_gtrigger89
