CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

all: libgsway89.a

gsway89.o: gsway89/src/gsway89.c gsway89/include/gsway89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -c gsway89/src/gsway89.c -o $@

libgsway89.a: gsway89.o
	$(AR) rcs $@ $<

clean:
	rm -f gsway89.o libgsway89.a
