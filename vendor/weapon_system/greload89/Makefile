CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Isrc

LIB = libgreload89.a
OBJ = src/greload89.o

all: $(LIB) test_greload89 demo_greload89

$(OBJ): src/greload89.c src/greload89.h
	$(CC) $(CFLAGS) -c src/greload89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_greload89: src/greload89.c tests/test_greload89.c
	$(CC) $(CFLAGS) $^ -o $@

demo_greload89: src/greload89.c demo/demo_greload89.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_greload89
	./test_greload89

clean:
	rm -f test_greload89 demo_greload89 $(OBJ) $(LIB)
