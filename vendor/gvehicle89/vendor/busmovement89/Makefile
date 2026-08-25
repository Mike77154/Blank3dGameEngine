CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
GVEH_INCLUDE ?= ../../include

all: check

check:
	$(CC) $(CFLAGS) -Iinclude -I$(GVEH_INCLUDE) -I../carmovement89/include -fsyntax-only src/busmovement89.c

clean:
	@true
