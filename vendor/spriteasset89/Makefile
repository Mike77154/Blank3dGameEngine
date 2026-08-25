CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCS = -Iinclude
all: test

test: build/test_spriteasset89 build/test_renlist89
	./build/test_spriteasset89
	./build/test_renlist89

build/test_spriteasset89: src/spriteasset89.c src/spriteasset89_softblit.c src/spriteasset89_renlist89.c tests/test_spriteasset89.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCS) $^ -o $@
build/test_renlist89: src/spriteasset89.c src/spriteasset89_renlist89.c tests/test_renlist89.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCS) $^ -o $@
clean:
	rm -rf build
