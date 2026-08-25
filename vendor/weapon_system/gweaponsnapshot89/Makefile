CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
NAME = gweaponsnapshot89

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/gweaponsnapshot89.c tests/test_gweaponsnapshot89.c -o test_gweaponsnapshot89
	./test_gweaponsnapshot89

clean:
	rm -f test_gweaponsnapshot89
