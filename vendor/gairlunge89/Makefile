CC = gcc
AR = ar
CFLAGS = -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude
BUILD = build
LIB = $(BUILD)/libgairlunge89.a
OBJ = $(BUILD)/gairlunge89.o

.PHONY: all clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/gairlunge89.c include/gairlunge89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/gairlunge89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

clean:
	rm -rf $(BUILD)
