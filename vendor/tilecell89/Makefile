CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude
BUILD = build
LIB = $(BUILD)/libtilecell89.a
.PHONY: all test clean audit
all: $(LIB)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/tilecell89.o: src/tilecell89.c include/tilecell89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/tilecell89.c -o $@
$(LIB): $(BUILD)/tilecell89.o
	$(AR) rcs $@ $^
$(BUILD)/test_tilecell89: tests/test_tilecell89.c src/tilecell89.c include/tilecell89.h | $(BUILD)
	$(CC) $(CFLAGS) src/tilecell89.c tests/test_tilecell89.c -o $@
$(BUILD)/test_spacing: tests/test_spacing.c src/tilecell89.c include/tilecell89.h | $(BUILD)
	$(CC) $(CFLAGS) src/tilecell89.c tests/test_spacing.c -o $@
test: $(BUILD)/test_tilecell89 $(BUILD)/test_spacing
	./$(BUILD)/test_tilecell89
	./$(BUILD)/test_spacing
audit:
	@! grep -R -n -E '\b(malloc|calloc|realloc|free|float|double|long[[:space:]]+long|int64_t|uint64_t)\b' include src
clean:
	rm -rf $(BUILD)
