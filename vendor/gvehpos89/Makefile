CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude
AR ?= ar
ARFLAGS ?= rcs

LIB = libgvpos89.a
OBJ = src/gvpos.o

all: $(LIB) demo_gvpos test_basic

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

src/gvpos.o: src/gvpos.c include/gvpos.h
	$(CC) $(CFLAGS) -c src/gvpos.c -o src/gvpos.o

demo_gvpos: demo/demo_gvpos.c $(LIB)
	$(CC) $(CFLAGS) demo/demo_gvpos.c $(LIB) -o demo_gvpos

test_basic: tests/test_basic.c $(LIB)
	$(CC) $(CFLAGS) tests/test_basic.c $(LIB) -o test_basic

check: test_basic demo_gvpos
	./test_basic
	./demo_gvpos

clean:
	rm -f src/*.o $(LIB) demo_gvpos test_basic
