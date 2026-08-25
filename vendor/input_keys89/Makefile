CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude
BUILD = build
OBJ = $(BUILD)/input_keys89.o
LIB = $(BUILD)/libinput_keys89.a
TEST = $(BUILD)/test_input_keys89
EXAMPLE = $(BUILD)/name_to_hid

.PHONY: all test example audit clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/input_keys89.c include/input_keys89.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c src/input_keys89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

$(TEST): tests/test_input_keys89.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_input_keys89.c $(LIB) -o $(TEST)

test: $(TEST)
	./$(TEST)

$(EXAMPLE): examples/name_to_hid.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) examples/name_to_hid.c $(LIB) -o $(EXAMPLE)

example: $(EXAMPLE)
	./$(EXAMPLE)

audit:
	@! grep -RInE '(^|[^[:alnum:]_])(malloc|calloc|realloc|free)[[:space:]]*\(' --include='*.c' --include='*.h' include src tests examples
	@! grep -RInE '(^|[^[:alnum:]_])(float|double)([^[:alnum:]_]|$$)' --include='*.c' --include='*.h' include src tests examples
	@echo "input_keys89 audit: no allocation, float or double"

clean:
	rm -rf $(BUILD)
