CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic -O2
INCLUDES = -Iinclude -Isrc

LIB = libbvh.a
CLI = bvh

SRC = \
  src/bvh_common.c \
  src/bvh_arena.c \
  src/bvh_polysym.c \
  src/bvh_token.c \
  src/bvh_lexer.c \
  src/bvh_value.c \
  src/bvh_ast.c \
  src/bvh_parser.c \
  src/bvh_io.c \
  src/bvh_ir.c \
  src/bvh_bytecode.c \
  src/bvh_vm.c \
  src/bvh_runtime.c \
  src/bvh_transpile.c \
  src/bvh_api.c

OBJ = $(SRC:.c=.o)

all: $(LIB) $(CLI)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(CLI): tools/bvh.o $(LIB)
	$(CC) $(CFLAGS) -o $@ tools/bvh.o $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) tools/bvh.o $(LIB) $(CLI)
	rm -f out.bvbc out.json

run: all
	./bvh run examples/$(EX).bhud

.PHONY: all clean run
