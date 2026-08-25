CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude
BUILD = build
LIB = $(BUILD)/libstaticsprite89.a

.PHONY: all test clean audit
all: $(LIB)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/staticsprite89.o: src/staticsprite89.c include/staticsprite89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/staticsprite89.c -o $@
$(LIB): $(BUILD)/staticsprite89.o
	$(AR) rcs $@ $^
$(BUILD)/test_staticsprite89: tests/test_staticsprite89.c src/staticsprite89.c include/staticsprite89.h | $(BUILD)
	$(CC) $(CFLAGS) src/staticsprite89.c tests/test_staticsprite89.c -o $@
test: $(BUILD)/test_staticsprite89
	./$(BUILD)/test_staticsprite89
audit:
	@! grep -R -n -E '\b(malloc|calloc|realloc|free|float|double|long[[:space:]]+long|int64_t|uint64_t)\b' include src
clean:
	rm -rf $(BUILD)
