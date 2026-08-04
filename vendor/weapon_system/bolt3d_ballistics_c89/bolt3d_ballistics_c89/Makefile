CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
ARFLAGS ?= rcs

BUILD_DIR = build
LIB = $(BUILD_DIR)/libbolt3d_ballistics_c89.a

SRC = \
    src/b3d_fixed.c \
    src/b3d_vec3.c \
    src/b3d_transform.c \
    src/b3d_collision.c \
    src/b3d_events.c \
    src/b3d_projectile.c \
    src/b3d_solver.c \
    src/b3d_world.c

OBJ = \
    $(BUILD_DIR)/b3d_fixed.o \
    $(BUILD_DIR)/b3d_vec3.o \
    $(BUILD_DIR)/b3d_transform.o \
    $(BUILD_DIR)/b3d_collision.o \
    $(BUILD_DIR)/b3d_events.o \
    $(BUILD_DIR)/b3d_projectile.o \
    $(BUILD_DIR)/b3d_solver.o \
    $(BUILD_DIR)/b3d_world.o

EXAMPLES = \
    $(BUILD_DIR)/demo_basic \
    $(BUILD_DIR)/demo_hooks \
    $(BUILD_DIR)/demo_solver

TESTS = \
    $(BUILD_DIR)/test_fixed \
    $(BUILD_DIR)/test_collision \
    $(BUILD_DIR)/test_projectiles \
    $(BUILD_DIR)/test_hitscan \
    $(BUILD_DIR)/test_transform \
    $(BUILD_DIR)/test_solver

.PHONY: all clean test audit examples tests

all: $(LIB) examples tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/b3d_fixed.o: src/b3d_fixed.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_fixed.c -o $@

$(BUILD_DIR)/b3d_vec3.o: src/b3d_vec3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_vec3.c -o $@

$(BUILD_DIR)/b3d_transform.o: src/b3d_transform.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_transform.c -o $@

$(BUILD_DIR)/b3d_collision.o: src/b3d_collision.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_collision.c -o $@

$(BUILD_DIR)/b3d_events.o: src/b3d_events.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_events.c -o $@

$(BUILD_DIR)/b3d_projectile.o: src/b3d_projectile.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_projectile.c -o $@

$(BUILD_DIR)/b3d_solver.o: src/b3d_solver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_solver.c -o $@

$(BUILD_DIR)/b3d_world.o: src/b3d_world.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/b3d_world.c -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

examples: $(EXAMPLES)

$(BUILD_DIR)/demo_basic: examples/demo_basic.c $(LIB)
	$(CC) $(CFLAGS) examples/demo_basic.c $(LIB) -o $@

$(BUILD_DIR)/demo_hooks: examples/demo_hooks.c $(LIB)
	$(CC) $(CFLAGS) examples/demo_hooks.c $(LIB) -o $@

$(BUILD_DIR)/demo_solver: examples/demo_solver.c $(LIB)
	$(CC) $(CFLAGS) examples/demo_solver.c $(LIB) -o $@

tests: $(TESTS)

$(BUILD_DIR)/test_fixed: tests/test_fixed.c $(LIB)
	$(CC) $(CFLAGS) tests/test_fixed.c $(LIB) -o $@

$(BUILD_DIR)/test_collision: tests/test_collision.c $(LIB)
	$(CC) $(CFLAGS) tests/test_collision.c $(LIB) -o $@

$(BUILD_DIR)/test_projectiles: tests/test_projectiles.c $(LIB)
	$(CC) $(CFLAGS) tests/test_projectiles.c $(LIB) -o $@

$(BUILD_DIR)/test_hitscan: tests/test_hitscan.c $(LIB)
	$(CC) $(CFLAGS) tests/test_hitscan.c $(LIB) -o $@

$(BUILD_DIR)/test_transform: tests/test_transform.c $(LIB)
	$(CC) $(CFLAGS) tests/test_transform.c $(LIB) -o $@

$(BUILD_DIR)/test_solver: tests/test_solver.c $(LIB)
	$(CC) $(CFLAGS) tests/test_solver.c $(LIB) -o $@

test: $(TESTS)
	$(BUILD_DIR)/test_fixed
	$(BUILD_DIR)/test_collision
	$(BUILD_DIR)/test_projectiles
	$(BUILD_DIR)/test_hitscan
	$(BUILD_DIR)/test_transform
	$(BUILD_DIR)/test_solver

audit:
	tools/audit_forbidden_tokens.sh

clean:
	rm -rf $(BUILD_DIR)
