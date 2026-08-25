CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic
GVEH_INCLUDE ?= ../../include

all: check

check:
	$(CC) $(CFLAGS) -Iinclude -I$(GVEH_INCLUDE) -fsyntax-only src/spacemovement89.c

clean:
	@true
