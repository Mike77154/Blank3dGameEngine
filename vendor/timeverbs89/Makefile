CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror

.PHONY: all test audit clean
all: test

test:
	$(CC) $(CFLAGS) -Iinclude src/timeverbs89.c tests/test_timeverbs89.c -o test_timeverbs89
	./test_timeverbs89

audit:
	@! grep -n -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' src/*.c include/*.h
	@! grep -n -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' src/*.c include/*.h

clean:
	rm -f test_timeverbs89 *.o *.a
