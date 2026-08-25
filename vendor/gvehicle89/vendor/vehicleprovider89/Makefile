CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
GVEH_INCLUDE ?= ../../include

all: test

test: tests/test_vehicleprovider89
	./tests/test_vehicleprovider89

check: test

tests/test_vehicleprovider89: src/vehicleprovider89.c tests/test_vehicleprovider89.c
	$(CC) $(CFLAGS) -Iinclude -I$(GVEH_INCLUDE) src/vehicleprovider89.c tests/test_vehicleprovider89.c -o $@

clean:
	rm -f tests/test_vehicleprovider89
