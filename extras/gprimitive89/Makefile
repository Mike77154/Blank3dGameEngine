CC ?= gcc
AR ?= ar
CFLAGS ?= -O2
C89FLAGS = -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
BUILD_DIR = build
CORE_OBJ = $(BUILD_DIR)/gprimitive89.o
OBJ_EXPORT_OBJ = $(BUILD_DIR)/gprimitive89_obj.o
LIB = $(BUILD_DIR)/libgprimitive89.a

.PHONY: all clean test example preview-export strict-check

all: $(LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CORE_OBJ): src/gprimitive89.c include/gprimitive89.h src/gprimitive89_polydata.inc | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C89FLAGS) -c src/gprimitive89.c -o $(CORE_OBJ)

$(OBJ_EXPORT_OBJ): src/gprimitive89_obj.c include/gprimitive89_obj.h include/gprimitive89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C89FLAGS) -c src/gprimitive89_obj.c -o $(OBJ_EXPORT_OBJ)

$(LIB): $(CORE_OBJ) $(OBJ_EXPORT_OBJ)
	$(AR) rcs $(LIB) $(CORE_OBJ) $(OBJ_EXPORT_OBJ)

test: | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C89FLAGS) src/gprimitive89.c tests/test_gprimitive89.c -o $(BUILD_DIR)/test_gprimitive89
	$(BUILD_DIR)/test_gprimitive89

example: | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C89FLAGS) src/gprimitive89.c src/gprimitive89_obj.c examples/basic_usage.c -o $(BUILD_DIR)/basic_usage

preview-export: | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C89FLAGS) src/gprimitive89.c src/gprimitive89_obj.c examples/export_preview_meshes.c -o $(BUILD_DIR)/export_preview_meshes

strict-check:
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -c src/gprimitive89.c -o /tmp/gprimitive89_strict.o
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -c src/gprimitive89_obj.c -o /tmp/gprimitive89_obj_strict.o

clean:
	rm -rf $(BUILD_DIR)
