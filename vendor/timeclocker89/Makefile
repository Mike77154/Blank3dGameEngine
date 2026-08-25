CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
AR ?= ar
ARFLAGS ?= rcs

LIB = libtimeclocker.a
OBJ = build/timeclocker.o

all: $(LIB)

build:
	mkdir -p build

$(OBJ): src/timeclocker.c include/timeclocker.h | build
	$(CC) $(CFLAGS) -c src/timeclocker.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

test: build/test_timeclocker
	./build/test_timeclocker

build/test_timeclocker: src/timeclocker.c tests/test_timeclocker.c include/timeclocker.h | build
	$(CC) $(CFLAGS) src/timeclocker.c tests/test_timeclocker.c -o build/test_timeclocker

example: build/example_game_loop
	./build/example_game_loop

build/example_game_loop: src/timeclocker.c examples/example_game_loop.c include/timeclocker.h | build
	$(CC) $(CFLAGS) src/timeclocker.c examples/example_game_loop.c -o build/example_game_loop

audit:
	@! grep -n -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' src/*.c include/*.h
	@! grep -n -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' src/*.c include/*.h

clean:
	rm -rf build $(LIB) test_timeclocker example_game_loop
