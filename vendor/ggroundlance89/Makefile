CC = gcc
AR = ar
CFLAGS = -std=c89 -pedantic -Wall -Wextra -O2 -Iinclude
BUILD = build
LIB = $(BUILD)/libggroundlance89.a
OBJ = $(BUILD)/ggroundlance89.o

.PHONY: all clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(OBJ): src/ggroundlance89.c include/ggroundlance89.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/ggroundlance89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

clean:
	rm -rf $(BUILD)
