CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/expandiblefire89.c tests/test_expandiblefire89.c -o test_expandiblefire89
	./test_expandiblefire89

clean:
	rm -f test_expandiblefire89
