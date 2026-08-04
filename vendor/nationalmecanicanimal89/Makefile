CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude -Isrc

BUILD_DIR = build
CORE_OBJ = $(BUILD_DIR)/nationalmecanicanimal89.o

.PHONY: all lib test demo clean

all: lib test demo

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CORE_OBJ): src/nationalmecanicanimal89.c include/nationalmecanicanimal89.h src/nm89_sin_table.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/nationalmecanicanimal89.c -o $(CORE_OBJ)

lib: $(CORE_OBJ)
	$(AR) rcs $(BUILD_DIR)/libnationalmecanicanimal89.a $(CORE_OBJ)

test: $(CORE_OBJ) tests/test_nm89.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE_OBJ) tests/test_nm89.c -o $(BUILD_DIR)/test_nm89
	$(BUILD_DIR)/test_nm89

demo: $(CORE_OBJ) examples/pistol_provider_demo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE_OBJ) examples/pistol_provider_demo.c -o $(BUILD_DIR)/pistol_provider_demo

clean:
	rm -rf $(BUILD_DIR)
