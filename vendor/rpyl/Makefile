CC ?= gcc
CFLAGS ?= -ansi -pedantic-errors -Wall -Wextra -Werror -Wdeclaration-after-statement
INCLUDES = \
 -IAPI -Iprogram -Icommon -Iconfig -Iutil -Ispan -Ierror -Iio -Istream \
 -Iarena -Ialloc -Itypes -Ivalue -Itoken -Ilexer -Iparser -Iast \
 -Isemantics -Isymtab -Ipolysym -Iregistry -Istore -IIR -Iopcodes \
 -Ibytecode -IVM -Iruntime -Ibuiltins -Istdlib_propia -Iwarper \
 -Itranspiler -Icompiler
CORE_SRCS = \
 common/rpyl_common.c \
 span/rpyl_span.c \
 error/rpyl_error.c \
 io/rpyl_io.c \
 symtab/rpyl_symtab.c \
 registry/rpyl_registry.c \
 store/rpyl_store.c \
 IR/rpyl_ir.c \
 opcodes/rpyl_opcodes.c \
 builtins/rpyl_builtins.c \
 semantics/rpyl_semantics.c \
 compiler/rpyl_compiler.c \
 warper/rpyl_warper.c \
 program/rpyl.c \
 alloc/rpyl_alloc.c \
 arena/rpyl_arena.c \
 ast/rpyl_ast.c \
 bytecode/rpyl_bytecode.c \
 stdlib_propia/rpyl_extlang.c \
 value/rpyl_fixed.c \
 lexer/rpyl_lexer.c \
 parser/rpyl_parser.c \
 polysym/rpyl_polysym.c \
 runtime/rpyl_runtime.c \
 VM/rpyl_vm.c \
 transpiler/rpyl_transpile.c \
 value/rpyl_value.c
CLI_SRCS = CLI/rpyl_cli.c

all: build/rpyl_cli


build/rpyl_cli: $(CORE_SRCS) $(CLI_SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE_SRCS) $(CLI_SRCS) -o build/rpyl_cli

check:
	$(CC) $(CFLAGS) $(INCLUDES) -c $(CORE_SRCS) $(CLI_SRCS)
	rm -f *.o

strict-core:
	$(CC) $(CFLAGS) $(INCLUDES) -DRPYL_ENABLE_STDIO=0 -DRPYL_ENABLE_FILE_IO=0 -DRPYL_ENABLE_LOG_STDIO=0 -c $(CORE_SRCS)
	rm -f *.o

clean:
	rm -rf build
	rm -f *.o


build/test_modules: $(CORE_SRCS) tests/test_modules.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE_SRCS) tests/test_modules.c -o build/test_modules

build/test_extlang_sanitized: $(CORE_SRCS) tests/test_extlang_sanitized.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -DRPYL_ENABLE_LOG_STDIO=0 $(CORE_SRCS) tests/test_extlang_sanitized.c -o build/test_extlang_sanitized

test: build/test_modules build/test_extlang_sanitized
	./build/test_modules
	./build/test_extlang_sanitized
