CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Werror
INCLUDES = -Iinclude

.PHONY: all test clean
all: test

test:
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_equipment_system89.c src/equipment_system89.c -o test_equipment_system89
	./test_equipment_system89

clean:
	rm -f test_equipment_system89
