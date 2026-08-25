CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror
NAME = gplayerprojectileaim89

all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/gplayerprojectileaim89.c tests/test_gplayerprojectileaim89.c -o test_gplayerprojectileaim89
	./test_gplayerprojectileaim89

clean:
	rm -f test_gplayerprojectileaim89
