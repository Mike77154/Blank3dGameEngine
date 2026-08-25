CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
GVEH_INCLUDE ?= ../../include

all: test

test: tests/test_vehiclephysics89
	./tests/test_vehiclephysics89

tests/test_vehiclephysics89: src/vehiclephysics89.c tests/test_vehiclephysics89.c
	$(CC) $(CFLAGS) -Iinclude -I$(GVEH_INCLUDE) src/vehiclephysics89.c tests/test_vehiclephysics89.c ../../src/gveh_math.c ../../src/gveh_body.c -o $@

clean:
	rm -f tests/test_vehiclephysics89
