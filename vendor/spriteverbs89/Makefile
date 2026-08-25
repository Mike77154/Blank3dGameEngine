CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCS = -Iinclude -I../SpriteAsset89/include -Iadapters/aseprite
all: test

test: build/test_spriteverbs89
	./build/test_spriteverbs89
build/test_spriteverbs89: src/spriteverbs89.c adapters/aseprite/spriteverbs89_aseprite.c ../SpriteAsset89/src/spriteasset89.c tests/test_spriteverbs89.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCS) $^ -o $@
clean:
	rm -rf build
