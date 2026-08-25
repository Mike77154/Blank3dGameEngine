CC ?= gcc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror

.PHONY: all test audit clean
all: test
test:
	$(CC) $(CFLAGS) -Iinclude src/generalvideoconfigc89.c tests/test_generalvideoconfigc89.c -o tests/generalvideoconfigc89_test
	./tests/generalvideoconfigc89_test
audit:
	@! grep -RIn -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' src include || (echo allocator_found && exit 1)
	@! grep -RIn -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' src include || (echo forbidden_numeric_found && exit 1)
clean:
	rm -f tests/generalvideoconfigc89_test
