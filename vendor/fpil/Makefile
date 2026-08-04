CC ?= gcc
AR ?= ar
CFLAGS ?= -O2
STRICT = -std=c89 -pedantic -Wall -Wextra -Wshadow -Wstrict-prototypes \
         -Wmissing-prototypes -Wold-style-definition -Wdeclaration-after-statement \
         -Wcast-qual -Wwrite-strings -Wconversion -Wsign-conversion
CPPFLAGS = -Ifpi_core -Ifpi_builtins -Ifpi_stndlib

CORE_SRCS = \
  fpi_core/fpi_alloc.c \
  fpi_core/fpi_api.c \
  fpi_core/fpi_arena.c \
  fpi_core/fpi_ast.c \
  fpi_core/fpi_bytecode.c \
  fpi_core/fpi_compiler.c \
  fpi_core/fpi_error.c \
  fpi_core/fpi_ext.c \
  fpi_core/fpi_io.c \
  fpi_core/fpi_ir.c \
  fpi_core/fpi_lexer.c \
  fpi_core/fpi_opcodes.c \
  fpi_core/fpi_parser.c \
  fpi_core/fpi_polysym.c \
  fpi_core/fpi_program.c \
  fpi_core/fpi_registry.c \
  fpi_core/fpi_runtime.c \
  fpi_core/fpi_semantics.c \
  fpi_core/fpi_span.c \
  fpi_core/fpi_store.c \
  fpi_core/fpi_stream.c \
  fpi_core/fpi_symtab.c \
  fpi_core/fpi_token.c \
  fpi_core/fpi_transpiler.c \
  fpi_core/fpi_util.c \
  fpi_core/fpi_value.c \
  fpi_core/fpi_vm.c \
  fpi_core/fpi_warper.c

BUILTIN_SRCS = fpi_builtins/fpi_builtins.c
STDLIB_SRCS = \
  fpi_stndlib/activation_fpi.c \
  fpi_stndlib/conditional_eval_adverbs_fpi.c \
  fpi_stndlib/state_manager_fpi.c \
  fpi_stndlib/time_binding_fpi.c \
  fpi_stndlib/value_resolver_fpi.c \
  fpi_stndlib/var_system_fpi.c
CORE_OBJS = $(CORE_SRCS:.c=.o)
BUILTIN_OBJS = $(BUILTIN_SRCS:.c=.o)
STDLIB_OBJS = $(STDLIB_SRCS:.c=.o)

.PHONY: all clean test strict audit example

all: libfpil_core.a libfpil_builtins.a libfpil_stdlib.a fpi_cli

libfpil_core.a: $(CORE_OBJS)
	$(AR) rcs $@ $(CORE_OBJS)

libfpil_builtins.a: $(BUILTIN_OBJS)
	$(AR) rcs $@ $(BUILTIN_OBJS)

libfpil_stdlib.a: $(STDLIB_OBJS)
	$(AR) rcs $@ $(STDLIB_OBJS)

fpi_cli: tools/fpi_cli.o libfpil_stdlib.a libfpil_builtins.a libfpil_core.a
	$(CC) $(CFLAGS) $(STRICT) tools/fpi_cli.o libfpil_stdlib.a libfpil_builtins.a libfpil_core.a -o $@

fpi_tests: tests/fpi_tests.o libfpil_stdlib.a libfpil_builtins.a libfpil_core.a
	$(CC) $(CFLAGS) $(STRICT) tests/fpi_tests.o libfpil_stdlib.a libfpil_builtins.a libfpil_core.a -o $@

fpi_example_host: examples/custom_state_host.o libfpil_builtins.a libfpil_core.a
	$(CC) $(CFLAGS) $(STRICT) examples/custom_state_host.o libfpil_builtins.a libfpil_core.a -o $@

example: fpi_example_host
	./fpi_example_host

%.o: %.c
	$(CC) $(CFLAGS) $(STRICT) $(CPPFLAGS) -c $< -o $@

test: fpi_tests
	./fpi_tests

strict:
	$(MAKE) clean
	$(MAKE) CFLAGS="-O0" all fpi_tests fpi_example_host

# Source audit: reject allocation calls and native real-number types/functions.
audit:
	@! grep -RInE '(^|[^[:alnum:]_])(malloc|calloc|realloc|free|strtod|strtof|fmod|fabs|sin|cos)[[:space:]]*\(' \
	  --include='*.c' --include='*.h' fpi_core fpi_builtins fpi_stndlib tools tests
	@! grep -RInE '(^|[^[:alnum:]_])(double|float)([^[:alnum:]_]|$$)' \
	  --include='*.c' --include='*.h' fpi_core fpi_builtins fpi_stndlib tools tests
	@echo "audit passed: no dynamic allocation or native real-number arithmetic"

clean:
	rm -f $(CORE_OBJS) $(BUILTIN_OBJS) $(STDLIB_OBJS) tools/fpi_cli.o tests/fpi_tests.o examples/custom_state_host.o \
	      libfpil_core.a libfpil_builtins.a libfpil_stdlib.a fpi_cli fpi_tests fpi_example_host
