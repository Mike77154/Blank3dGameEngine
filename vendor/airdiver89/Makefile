CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2
COMMON = ../vertical_motion89_common/include
INCLUDES = -Iinclude -I$(COMMON)
BUILD = build
OBJ = $(BUILD)/airdiver89.o
LIB = $(BUILD)/libairdiver89.a

.PHONY: all clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/airdiver89.c include/airdiver89.h $(COMMON)/vmotion89_provider.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c src/airdiver89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

clean:
	rm -rf $(BUILD)
