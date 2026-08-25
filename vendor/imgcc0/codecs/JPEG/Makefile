CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -O2 -Wall -Wextra -pedantic
CPPFLAGS ?= -Iinclude

BUILD_DIR := build
LIB_OBJ := $(BUILD_DIR)/c89jpeg.o
LIB_A := $(BUILD_DIR)/libc89jpeg.a

EXAMPLES := \
	$(BUILD_DIR)/example_source_resume_encode \
	$(BUILD_DIR)/example_source_resume_session

TEST_BIN := $(BUILD_DIR)/test_c89jpeg
PROGRESSIVE_TEST_BIN := $(BUILD_DIR)/test_progressive

.PHONY: all clean test examples

all: $(LIB_A)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_OBJ): src/c89jpeg.c include/c89jpeg.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB_A): $(LIB_OBJ)
	$(AR) rcs $@ $<

$(BUILD_DIR)/example_source_resume_encode: examples/example_source_resume_encode.c $(LIB_A)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB_A) -o $@

$(BUILD_DIR)/example_source_resume_session: examples/example_source_resume_session.c $(LIB_A)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB_A) -o $@

$(TEST_BIN): tests/test_c89jpeg.c $(LIB_A)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIB_A) -o $@

$(PROGRESSIVE_TEST_BIN): tests/test_progressive.c tests/fixtures/progressive_pair89.h $(LIB_A)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_progressive.c $(LIB_A) -o $@

test: $(TEST_BIN) $(PROGRESSIVE_TEST_BIN)
	./$(TEST_BIN)
	./$(PROGRESSIVE_TEST_BIN)

examples: $(EXAMPLES)
	./$(BUILD_DIR)/example_source_resume_encode
	./$(BUILD_DIR)/example_source_resume_session

clean:
	rm -rf $(BUILD_DIR)
