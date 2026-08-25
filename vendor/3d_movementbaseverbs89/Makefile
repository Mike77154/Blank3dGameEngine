CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

OBJ = build/3d_movementbaseverbs89.o
LIB = build/lib3d_movementbaseverbs89.a
TEST = build/test_movementbaseverbs89
EXAMPLE = build/provider_example

all: $(LIB) $(TEST) $(EXAMPLE)

build:
	mkdir -p build

$(OBJ): src/3d_movementbaseverbs89.c include/3d_movementbaseverbs89.h | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/3d_movementbaseverbs89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

$(TEST): tests/test_movementbaseverbs89.c $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_movementbaseverbs89.c $(OBJ) -o $(TEST)

$(EXAMPLE): examples/provider_example.c $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) examples/provider_example.c $(OBJ) -o $(EXAMPLE)

test: $(TEST)
	./$(TEST)

clean:
	rm -rf build

.PHONY: all test clean
