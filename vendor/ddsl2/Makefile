CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -I.
LDFLAGS ?=

SRC = \
  API/ddsl_api.c \
  IR/ir.c \
  Transpiler/transpile.c \
  VM/vm.c \
  alloc/alloc.c \
  arena/arena.c \
  ast/ast.c \
  built-ins/builtins.c \
  bytecode/bytecode.c \
  common/strview.c \
  compiler/compiler.c \
  error/error.c \
  io/io.c \
  lexer/lexer.c \
  opcodes/opcodes.c \
  parser/parser.c \
  polysym/polysym.c \
  program/program.c \
  registry/registry.c \
  runtime/runtime.c \
  semantics/semantics.c \
  span/span.c \
  stdlib/stdlib.c \
  store/store.c \
  stream/stream.c \
  symtab/symtab.c \
  token/token.c \
  types/fixed.c \
  util/util.c \
  value/value.c \
  warper/warper.c

CLI_SRC = CLI/ddsl_cli.c
OUT = build/ddsl_cli
TEST_OUT = build/test_regressions
TEST_SRC = tests/test_regressions.c
TRANSPILED_OUT = build/test_transpiled

all: $(OUT)

build:
	mkdir -p build

$(OUT): $(SRC) $(CLI_SRC) | build
	$(CC) $(CFLAGS) $(SRC) $(CLI_SRC) -o $(OUT) $(LDFLAGS)

$(TEST_OUT): $(SRC) $(TEST_SRC) | build
	$(CC) $(CFLAGS) $(SRC) $(TEST_SRC) -o $(TEST_OUT) $(LDFLAGS)

check: $(OUT) $(TEST_OUT)
	./$(OUT) run examples/scripts/demo.ddsl --dump
	./$(OUT) bc examples/scripts/demo.ddsl >/dev/null
	./$(OUT) transpile examples/scripts/demo.ddsl >/dev/null
	./$(TEST_OUT)
	./$(OUT) transpile tests/data/utf8.ddsl > build/transpiled_utf8.c
	$(CC) $(CFLAGS) -c build/transpiled_utf8.c -o build/transpiled_utf8.o
	./$(OUT) transpile tests/data/parity.ddsl > build/transpiled_parity.c
	$(CC) $(CFLAGS) $(SRC) build/transpiled_parity.c tests/transpiled_runner.c -o $(TRANSPILED_OUT) $(LDFLAGS)
	./$(TRANSPILED_OUT)

clean:
	rm -f $(OUT) $(TEST_OUT) $(TRANSPILED_OUT) build/transpiled_utf8.c build/transpiled_utf8.o build/transpiled_parity.c *.o */*.o

.PHONY: all check clean
