CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude
BUILD = build
LIB = $(BUILD)/libskyboxrecipe89.a
OBJ = $(BUILD)/skyboxrecipe89.o
TEST = $(BUILD)/test_skyboxrecipe89

all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/skyboxrecipe89.c include/skyboxrecipe89.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c src/skyboxrecipe89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

check: $(TEST)
	./$(TEST)

$(TEST): tests/test_skyboxrecipe89.c src/skyboxrecipe89.c include/skyboxrecipe89.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_skyboxrecipe89.c src/skyboxrecipe89.c -o $(TEST)

clean:
	rm -rf $(BUILD)

.PHONY: all check clean
