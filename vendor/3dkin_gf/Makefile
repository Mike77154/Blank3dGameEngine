CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra
CPPFLAGS ?= -Iinclude
ARFLAGS ?= rcs

BUILD = build
LIB = $(BUILD)/libgk3d.a
OBJ = $(BUILD)/gk3d.o
TEST = $(BUILD)/test_gk3d
DEMO1 = $(BUILD)/condition_demo
DEMO2 = $(BUILD)/verb_demo

.PHONY: all lib examples test clean verify

all: lib examples test

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/gk3d.c include/gk3d.h include/gk3d_fixed.h include/gk3d_config.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gk3d.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

lib: $(LIB)

$(DEMO1): examples/condition_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/condition_demo.c $(LIB) -o $(DEMO1)

$(DEMO2): examples/verb_demo.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/verb_demo.c $(LIB) -o $(DEMO2)

examples: $(DEMO1) $(DEMO2)

$(TEST): tests/test_gk3d.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_gk3d.c $(LIB) -o $(TEST)

test: $(TEST)
	./$(TEST)

verify: clean all

clean:
	rm -rf $(BUILD)
