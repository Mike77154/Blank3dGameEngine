CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude
AR ?= ar
BUILD_DIR := build

LIB_SRC := $(wildcard src/*.c)
DEMO_SRC := demo/pbb_item_demo.c
TEST_SRC := tests/pbb_item_tests.c
LIB_OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(LIB_SRC))

.PHONY: all demo test lib clean

all: lib demo test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

lib: $(BUILD_DIR)/libpbb_item_system.a

$(BUILD_DIR)/libpbb_item_system.a: $(LIB_OBJ)
	$(AR) rcs $@ $(LIB_OBJ)

demo: $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(DEMO_SRC) -o $(BUILD_DIR)/pbb_item_demo

test: $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(BUILD_DIR)/pbb_item_tests
	./$(BUILD_DIR)/pbb_item_tests

clean:
	rm -rf $(BUILD_DIR)
