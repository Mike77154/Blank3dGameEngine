CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude -I../gcrosshair_base89/include -I../gcrosshair_core89/include
LIB = libgcrosshair_anim89.a
OBJ = gcrosshair_anim89.o
BASELIB = ../gcrosshair_base89/libgcrosshair_base89.a
CORELIB = ../gcrosshair_core89/libgcrosshair_core89.a
all: $(LIB) test_anim
$(OBJ): src/gcrosshair_anim89.c include/gcrosshair_anim89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_anim89.c -o $(OBJ)
$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)
test_anim: tests/test_anim.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_anim.c $(LIB) $(CORELIB) -o test_anim
test: test_anim
	./test_anim
clean:
	rm -f $(OBJ) $(LIB) test_anim test_anim.exe
.PHONY: all test clean
