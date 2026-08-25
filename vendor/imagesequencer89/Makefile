CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude
BUILD = build
LIB = $(BUILD)/libimagesequencer89.a
.PHONY: all test clean audit
all: $(LIB)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/imagesequencer89.o: src/imagesequencer89.c include/imagesequencer89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/imagesequencer89.c -o $@
$(LIB): $(BUILD)/imagesequencer89.o
	$(AR) rcs $@ $^
$(BUILD)/test_imagesequencer89: tests/test_imagesequencer89.c src/imagesequencer89.c include/imagesequencer89.h | $(BUILD)
	$(CC) $(CFLAGS) src/imagesequencer89.c tests/test_imagesequencer89.c -o $@
$(BUILD)/test_modes: tests/test_modes.c src/imagesequencer89.c include/imagesequencer89.h | $(BUILD)
	$(CC) $(CFLAGS) src/imagesequencer89.c tests/test_modes.c -o $@
test: $(BUILD)/test_imagesequencer89 $(BUILD)/test_modes
	./$(BUILD)/test_imagesequencer89
	./$(BUILD)/test_modes
audit:
	@! grep -R -n -E '\b(malloc|calloc|realloc|free|float|double|long[[:space:]]+long|int64_t|uint64_t)\b' include src
clean:
	rm -rf $(BUILD)
