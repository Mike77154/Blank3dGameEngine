CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Werror -O2
CPPFLAGS ?= -Iinclude
BUILD_DIR := build
LIB_OBJ := $(BUILD_DIR)/gproj2d89.o
TEST_BIN := $(BUILD_DIR)/test_gproj2d89
DEMO_BIN := $(BUILD_DIR)/demo_svg

.PHONY: all test audit preview clean

all: $(LIB_OBJ) $(TEST_BIN) $(DEMO_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_OBJ): src/gproj2d89.c include/gproj2d89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/gproj2d89.c -o $@

$(TEST_BIN): src/gproj2d89.c tests/test_gproj2d89.c include/gproj2d89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) src/gproj2d89.c tests/test_gproj2d89.c -o $@

$(DEMO_BIN): src/gproj2d89.c examples/demo_svg.c include/gproj2d89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) src/gproj2d89.c examples/demo_svg.c -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

audit:
	./tests/static_audit.sh

preview: $(DEMO_BIN)
	./$(DEMO_BIN) preview/gproj2d89_preview.svg
	python3 tools/svg_to_png.py preview/gproj2d89_preview.svg preview/gproj2d89_preview.png

clean:
	rm -rf $(BUILD_DIR)
