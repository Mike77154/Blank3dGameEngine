CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
ARFLAGS ?= rcs

BUILD_DIR := build
BIN_DIR := bin

LIB := $(BUILD_DIR)/libecs89.a
CORE_OBJ := $(BUILD_DIR)/ecs89.o

EXAMPLES := $(BIN_DIR)/demo_basic $(BIN_DIR)/demo_integration
TESTS := $(BIN_DIR)/test_ecs89

all: dirs $(LIB) $(EXAMPLES) $(TESTS)

dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(CORE_OBJ): src/ecs89.c include/ecs89.h
	$(CC) $(CFLAGS) -c src/ecs89.c -o $@

$(LIB): $(CORE_OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(BIN_DIR)/demo_basic: examples/demo_basic.c $(LIB)
	$(CC) $(CFLAGS) examples/demo_basic.c $(LIB) -o $@

$(BIN_DIR)/demo_integration: examples/demo_integration.c $(LIB)
	$(CC) $(CFLAGS) examples/demo_integration.c $(LIB) -o $@

$(BIN_DIR)/test_ecs89: tests/test_ecs89.c $(LIB)
	$(CC) $(CFLAGS) tests/test_ecs89.c $(LIB) -o $@

test: $(BIN_DIR)/test_ecs89
	$(BIN_DIR)/test_ecs89

run-basic: $(BIN_DIR)/demo_basic
	$(BIN_DIR)/demo_basic

run-integration: $(BIN_DIR)/demo_integration
	$(BIN_DIR)/demo_integration

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all dirs test run-basic run-integration clean
