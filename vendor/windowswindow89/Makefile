CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror

.PHONY: audit

audit:
	@! grep -R -n -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' include src
	@! grep -R -n -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b' include src
	@echo "WindowsWindow89 source audit PASS"
