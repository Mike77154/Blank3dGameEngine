CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror

.PHONY: all test audit clean
all: test

test:
	$(CC) $(CFLAGS) -I. rt_time.c tests/test_rt_time.c -o test_rt_time89
	./test_rt_time89

audit:
	@! grep -n -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' rt_time.c rt_time.h
	@! grep -n -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' rt_time.c rt_time.h

clean:
	rm -f test_rt_time89 *.o *.a
