CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
ARFLAGS ?= rcs

LIB = libnumsys.a
OBJ = build/numsys.o
TEST = build/test_numsys
EXAMPLES = \
	build/example_basic \
	build/example_construct_fusion_style \
	build/example_modifiers \
	build/example_snapshot

all: $(LIB) $(TEST) $(EXAMPLES)

build:
	mkdir -p build

$(OBJ): src/numsys.c include/numsys.h | build
	$(CC) $(CFLAGS) -c src/numsys.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

$(TEST): src/numsys.c tests/test_numsys.c include/numsys.h include/numsys_packs.h | build
	$(CC) $(CFLAGS) src/numsys.c tests/test_numsys.c -o $(TEST)

build/example_basic: src/numsys.c examples/example_basic.c include/numsys.h include/numsys_packs.h | build
	$(CC) $(CFLAGS) src/numsys.c examples/example_basic.c -o build/example_basic

build/example_construct_fusion_style: src/numsys.c examples/example_construct_fusion_style.c include/numsys.h include/numsys_packs.h | build
	$(CC) $(CFLAGS) src/numsys.c examples/example_construct_fusion_style.c -o build/example_construct_fusion_style

build/example_modifiers: src/numsys.c examples/example_modifiers.c include/numsys.h include/numsys_packs.h | build
	$(CC) $(CFLAGS) src/numsys.c examples/example_modifiers.c -o build/example_modifiers

build/example_snapshot: src/numsys.c examples/example_snapshot.c include/numsys.h include/numsys_packs.h | build
	$(CC) $(CFLAGS) src/numsys.c examples/example_snapshot.c -o build/example_snapshot

check: $(TEST) $(EXAMPLES)
	./$(TEST)
	./build/example_basic
	./build/example_construct_fusion_style
	./build/example_modifiers
	./build/example_snapshot

clean:
	rm -rf build $(LIB)

.PHONY: all check clean
