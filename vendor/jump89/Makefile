CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2
COMMON = ../vertical_motion89_common/include
INCLUDES = -Iinclude -I$(COMMON)
BUILD = build
OBJ = $(BUILD)/jump89.o
LIB = $(BUILD)/libjump89.a

.PHONY: all clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/jump89.c include/jump89.h $(COMMON)/vmotion89_provider.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c src/jump89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

clean:
	rm -rf $(BUILD)
