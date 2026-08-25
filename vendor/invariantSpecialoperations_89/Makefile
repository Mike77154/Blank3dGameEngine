CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude -Iadapters/ddsl2 -Iadapters/flags89 -I../flags
CORE = src/invariantSpecialoperations_89.c
DDSL = adapters/ddsl2/invariantSpecialoperations_89_ddsl2.c
FLAGS = adapters/flags89/invariantSpecialoperations_89_flags89.c \
        ../flags/flags_fx.c ../flags/flags_pool.c ../flags/flags_value.c ../flags/flagstore.c

.PHONY: all test clean audit
all: test

test: tests/test_core tests/test_ddsl2 tests/test_flags89
	./tests/test_core
	./tests/test_ddsl2
	./tests/test_flags89

tests/test_core: $(CORE) tests/test_invariantSpecialoperations_89.c
	$(CC) $(CFLAGS) -Iinclude $^ -o $@

tests/test_ddsl2: $(CORE) $(DDSL) tests/test_ddsl2_adapter.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

tests/test_flags89: $(CORE) $(FLAGS) tests/test_flags89_adapter.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

audit:
	@! grep -R -n -E '(malloc|calloc|realloc|free)[[:space:]]*[(]' include src adapters tests examples
	@! grep -R -n -E '(^|[^[:alnum:]_])(float|double)([^[:alnum:]_]|$$)' include src adapters tests examples
	@echo "invariantSpecialoperations_89 audit: OK"

clean:
	rm -f tests/test_core tests/test_ddsl2 tests/test_flags89
