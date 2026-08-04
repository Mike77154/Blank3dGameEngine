CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Isrc

LIB = libgfiremode89.a
OBJ = src/gfiremode89.o

all: $(LIB) test_gfiremode89 demo_gfiremode89

$(OBJ): src/gfiremode89.c src/gfiremode89.h
	$(CC) $(CFLAGS) -c src/gfiremode89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_gfiremode89: src/gfiremode89.c tests/test_gfiremode89.c
	$(CC) $(CFLAGS) $^ -o $@

demo_gfiremode89: src/gfiremode89.c demo/demo_gfiremode89.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_gfiremode89
	./test_gfiremode89

clean:
	rm -f test_gfiremode89 demo_gfiremode89 $(OBJ) $(LIB)
