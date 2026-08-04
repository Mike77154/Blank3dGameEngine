CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

all: libgaimquery89.a

gaimquery89.o: gaimquery89/src/gaimquery89.c gaimquery89/include/gaimquery89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -c gaimquery89/src/gaimquery89.c -o $@

libgaimquery89.a: gaimquery89.o
	$(AR) rcs $@ $<

clean:
	rm -f gaimquery89.o libgaimquery89.a
