CC ?= cc
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude
.PHONY: all test audit clean
all: test
build/test_condor_evact89: src/condor_evact89.c tests/test_condor_evact89.c include/condor_evact89.h
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) src/condor_evact89.c tests/test_condor_evact89.c -o $@
test: build/test_condor_evact89
	./build/test_condor_evact89
audit:
	@! grep -R -nE '(malloc|calloc|realloc|free|HeapAlloc|VirtualAlloc)[[:space:]]*\(|(^|[^A-Za-z_])(float|double)([^A-Za-z_]|$$)' src || (echo forbidden-token-found; exit 1)
clean:
	rm -rf build
