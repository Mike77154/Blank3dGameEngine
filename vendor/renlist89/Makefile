CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude
BUILD = build
LIB = $(BUILD)/librenlist89.a
.PHONY: all test clean audit
all: $(LIB)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/renlist89.o: src/renlist89.c include/renlist89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/renlist89.c -o $@
$(LIB): $(BUILD)/renlist89.o
	$(AR) rcs $@ $^
$(BUILD)/test_renlist89: tests/test_renlist89.c src/renlist89.c include/renlist89.h | $(BUILD)
	$(CC) $(CFLAGS) src/renlist89.c tests/test_renlist89.c -o $@
$(BUILD)/test_multi: tests/test_multi.c src/renlist89.c include/renlist89.h | $(BUILD)
	$(CC) $(CFLAGS) src/renlist89.c tests/test_multi.c -o $@
test: $(BUILD)/test_renlist89 $(BUILD)/test_multi
	./$(BUILD)/test_renlist89
	./$(BUILD)/test_multi
audit:
	@! grep -R -n -E '\b(malloc|calloc|realloc|free|float|double|long[[:space:]]+long|int64_t|uint64_t)\b' include src
clean:
	rm -rf $(BUILD)
