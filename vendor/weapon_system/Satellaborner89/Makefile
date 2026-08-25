CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
all: test
test:
	$(CC) $(CFLAGS) -Iinclude src/satellaborner89.c tests/test_satellaborner89.c -o test_satellaborner89
	./test_satellaborner89
clean:
	rm -f test_satellaborner89
