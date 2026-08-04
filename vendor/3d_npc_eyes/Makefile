CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude
ARFLAGS ?= rcs

LIB = lib3d_npc_eyes.a
OBJ = build/3d_npc_eyes.o
TEST_BIN = build/test_3d_npc_eyes
DEMO_BASIC = build/demo_basic
DEMO_GRID = build/demo_grid_raycast
DEMO_FRUSTUM = build/demo_frustum

all: $(LIB)

build:
	mkdir -p build

$(OBJ): src/3d_npc_eyes.c include/3d_npc_eyes.h | build
	$(CC) $(CFLAGS) -c src/3d_npc_eyes.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

$(TEST_BIN): tests/test_3d_npc_eyes.c $(LIB) | build
	$(CC) $(CFLAGS) tests/test_3d_npc_eyes.c $(LIB) -o $(TEST_BIN)

$(DEMO_BASIC): examples/demo_basic.c $(LIB) | build
	$(CC) $(CFLAGS) examples/demo_basic.c $(LIB) -o $(DEMO_BASIC)

$(DEMO_GRID): examples/demo_grid_raycast.c $(LIB) | build
	$(CC) $(CFLAGS) examples/demo_grid_raycast.c $(LIB) -o $(DEMO_GRID)

$(DEMO_FRUSTUM): examples/demo_frustum.c $(LIB) | build
	$(CC) $(CFLAGS) examples/demo_frustum.c $(LIB) -o $(DEMO_FRUSTUM)

test: $(TEST_BIN)
	$(TEST_BIN)

examples: $(DEMO_BASIC) $(DEMO_GRID) $(DEMO_FRUSTUM)

check:
	python3 scripts/check_forbidden.py

clean:
	rm -rf build $(LIB)

.PHONY: all test examples check clean
