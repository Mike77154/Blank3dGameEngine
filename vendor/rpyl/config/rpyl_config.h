#ifndef RPYL_CONFIG_H
#define RPYL_CONFIG_H

/*
    rpyl_config.h

    Compile-time configuration knobs.

    This refactor keeps the runtime C89-friendly and deterministic:
    - libc dynamic storage is not required by the core;
    - all dynamic-looking structures are backed by bounded static/external storage;
    - numeric fractional values are represented by fixed point helpers.

    Override any value before including the headers or with -DNAME=value.
*/

/* ---------------- Strict profile ---------------- */
#ifndef RPYL_STRICT_NO_HEAP
#define RPYL_STRICT_NO_HEAP 1
#endif

/*
   Stdio/file IO is useful for the CLI and host-side toolchain, but embedded
   runtimes can compile the core with -DRPYL_ENABLE_STDIO=0. Buffer/VFS paths
   stay available in that mode.
*/
#ifndef RPYL_ENABLE_STDIO
#define RPYL_ENABLE_STDIO 1
#endif

#ifndef RPYL_ENABLE_FILE_IO
#define RPYL_ENABLE_FILE_IO RPYL_ENABLE_STDIO
#endif

#ifndef RPYL_ENABLE_LOG_STDIO
#define RPYL_ENABLE_LOG_STDIO RPYL_ENABLE_STDIO
#endif

#ifndef RPYL_MAX_CONTEXTS
#define RPYL_MAX_CONTEXTS 4
#endif

#ifndef RPYL_MAX_RUNTIMES
#define RPYL_MAX_RUNTIMES 4
#endif

#ifndef RPYL_MAX_ARENAS
#define RPYL_MAX_ARENAS 4
#endif

#ifndef RPYL_MAX_BYTECODES
#define RPYL_MAX_BYTECODES 4
#endif

#ifndef RPYL_MAX_LANGHOSTS
#define RPYL_MAX_LANGHOSTS 4
#endif

#ifndef RPYL_MAX_SYMBOLHOSTS
#define RPYL_MAX_SYMBOLHOSTS 4
#endif

#ifndef RPYL_STATIC_ALLOC_BYTES
#define RPYL_STATIC_ALLOC_BYTES (128u * 1024u)
#endif

#ifndef RPYL_ARENA_DEFAULT_CAPACITY
#define RPYL_ARENA_DEFAULT_CAPACITY (128u * 1024u)
#endif

/* ---------------- Lexer ---------------- */
#ifndef RPYL_LEXER_MAX_INDENT
#define RPYL_LEXER_MAX_INDENT 32
#endif

#ifndef RPYL_LEXER_MAX_LABEL_KW
#define RPYL_LEXER_MAX_LABEL_KW 32
#endif

/* ---------------- Tokens ---------------- */
#ifndef RPYL_TOKEN_MAX_TEXT
#define RPYL_TOKEN_MAX_TEXT 128
#endif

/* ---------------- AST ---------------- */
#ifndef RPYL_AST_MAX_NAME
#define RPYL_AST_MAX_NAME 128
#endif

#ifndef RPYL_AST_MAX_VALUE
#define RPYL_AST_MAX_VALUE 128
#endif

#ifndef RPYL_AST_MAX_ARGS
#define RPYL_AST_MAX_ARGS 8
#endif

#ifndef RPYL_AST_MAX_ARG_TEXT
#define RPYL_AST_MAX_ARG_TEXT 128
#endif

#ifndef RPYL_AST_MAX_STATIC_NODES
#define RPYL_AST_MAX_STATIC_NODES 4096
#endif

/* ---------------- Parser helpers ---------------- */
#ifndef RPYL_PARSER_MAX_PREARGS
#define RPYL_PARSER_MAX_PREARGS 8
#endif

#ifndef RPYL_PARSER_MAX_BLOCK_NAME
#define RPYL_PARSER_MAX_BLOCK_NAME 128
#endif

/* ---------------- Public context ---------------- */
#ifndef RPYL_CTX_MAX_USER_COMMANDS
#define RPYL_CTX_MAX_USER_COMMANDS 64
#endif

#ifndef RPYL_CTX_MAX_CMD_NAME
#define RPYL_CTX_MAX_CMD_NAME 128
#endif

#ifndef RPYL_CTX_MAX_START_BLOCK
#define RPYL_CTX_MAX_START_BLOCK 128
#endif

/* ---------------- Runtime ---------------- */
#ifndef RPYL_RUNTIME_MAX_COMMANDS
#define RPYL_RUNTIME_MAX_COMMANDS 64
#endif

#ifndef RPYL_RUNTIME_MAX_DEFINES
#define RPYL_RUNTIME_MAX_DEFINES 256
#endif

#ifndef RPYL_RUNTIME_MAX_STACK
#define RPYL_RUNTIME_MAX_STACK 64
#endif

#ifndef RPYL_RUNTIME_MAX_SCOPES
#define RPYL_RUNTIME_MAX_SCOPES 32
#endif

#ifndef RPYL_RUNTIME_MAX_VARS_PER_SCOPE
#define RPYL_RUNTIME_MAX_VARS_PER_SCOPE 64
#endif

#ifndef RPYL_RUNTIME_MAX_ONCE_NODES
#define RPYL_RUNTIME_MAX_ONCE_NODES 256
#endif

#ifndef RPYL_RUNTIME_MAX_ENTERED_BLOCKS
#define RPYL_RUNTIME_MAX_ENTERED_BLOCKS 128
#endif

#ifndef RPYL_RUNTIME_MAX_NAME
#define RPYL_RUNTIME_MAX_NAME 128
#endif

#ifndef RPYL_RUNTIME_MAX_VALUE
#define RPYL_RUNTIME_MAX_VALUE 256
#endif

#ifndef RPYL_RUNTIME_MAX_ARGS
#define RPYL_RUNTIME_MAX_ARGS 8
#endif

#ifndef RPYL_RUNTIME_MAX_JOINED
#define RPYL_RUNTIME_MAX_JOINED 256
#endif

/* ---------------- Fixed point ---------------- */
#ifndef RPYL_FX_SHIFT
#define RPYL_FX_SHIFT 16
#endif

#ifndef RPYL_FX_ONE
#define RPYL_FX_ONE 65536L
#endif

/* ---------------- Bytecode ---------------- */
#ifndef RPYL_BC_MAX_CODE
#define RPYL_BC_MAX_CODE 16384
#endif

#ifndef RPYL_BC_MAX_STRINGS
#define RPYL_BC_MAX_STRINGS 2048
#endif

#ifndef RPYL_BC_MAX_STRING_BYTES
#define RPYL_BC_MAX_STRING_BYTES 65536u
#endif

#ifndef RPYL_BC_MAX_LABELS
#define RPYL_BC_MAX_LABELS 512
#endif

#ifndef RPYL_BC_MAX_DEFINES
#define RPYL_BC_MAX_DEFINES 512
#endif

/* ---------------- Extlang (optional) ---------------- */
#ifndef RPYL_EXTLANG_MAX_PLUGINS
#define RPYL_EXTLANG_MAX_PLUGINS 32
#endif

#ifndef RPYL_EXTLANG_MAX_INITS
#define RPYL_EXTLANG_MAX_INITS 64
#endif

#ifndef RPYL_EXTLANG_MAX_INIT_CODE
#define RPYL_EXTLANG_MAX_INIT_CODE 8192u
#endif

#ifndef RPYL_EXTLANG_MAX_SCRIPT_BYTES
#define RPYL_EXTLANG_MAX_SCRIPT_BYTES 131072u
#endif

#ifndef RPYL_EXTLANG_MAX_RAW_LINES
#define RPYL_EXTLANG_MAX_RAW_LINES 512
#endif

#ifndef RPYL_EXTLANG_MAX_HOSTS
#define RPYL_EXTLANG_MAX_HOSTS RPYL_MAX_LANGHOSTS
#endif

#ifndef RPYL_EXTLANG_MAX_CODE_BYTES
#define RPYL_EXTLANG_MAX_CODE_BYTES (RPYL_EXTLANG_MAX_INITS * RPYL_EXTLANG_MAX_INIT_CODE)
#endif

#ifndef RPYL_FILE_MAX_BYTES
#define RPYL_FILE_MAX_BYTES RPYL_EXTLANG_MAX_SCRIPT_BYTES
#endif

/* ---------------- PolySym ---------------- */
#ifndef RPYL_POLYSYM_MAX_SYMBOLS
#define RPYL_POLYSYM_MAX_SYMBOLS 128
#endif

#ifndef RPYL_POLYSYM_MAX_ATTACHMENTS
#define RPYL_POLYSYM_MAX_ATTACHMENTS 32
#endif

#ifndef RPYL_POLYSYM_MAX_HOSTS
#define RPYL_POLYSYM_MAX_HOSTS RPYL_MAX_SYMBOLHOSTS
#endif

#ifndef RPYL_POLYSYM_MAX_NAME
#define RPYL_POLYSYM_MAX_NAME 64
#endif

#ifndef RPYL_POLYSYM_MAX_CMD
#define RPYL_POLYSYM_MAX_CMD 64
#endif

/* ---------------- Transpiler ---------------- */
#ifndef RPYL_TRANSPILE_MAX_PREFIX
#define RPYL_TRANSPILE_MAX_PREFIX 128
#endif

/* ---------------- Extlang buffers ---------------- */
#ifndef RPYL_EXTLANG_MAX_LANG
#define RPYL_EXTLANG_MAX_LANG 64
#endif

#ifndef RPYL_EXTLANG_MAX_PATH
#define RPYL_EXTLANG_MAX_PATH 512
#endif

#ifndef RPYL_EXTLANG_MAX_LINE
#define RPYL_EXTLANG_MAX_LINE 4096
#endif

/* Required indentation for non-blank lines inside init <lang>: blocks. */
#ifndef RPYL_EXTLANG_BODY_INDENT
#define RPYL_EXTLANG_BODY_INDENT 4
#endif

/* ---------------- PolySym buffers ---------------- */
#ifndef RPYL_POLYSYM_MAX_TOKEN
#define RPYL_POLYSYM_MAX_TOKEN 256
#endif

#ifndef RPYL_POLYSYM_MAX_PATH
#define RPYL_POLYSYM_MAX_PATH 512
#endif

#ifndef RPYL_POLYSYM_MAX_LINE
#define RPYL_POLYSYM_MAX_LINE 4096
#endif

#ifndef RPYL_POLYSYM_MAX_OUT_LINE
#define RPYL_POLYSYM_MAX_OUT_LINE 8192
#endif

#ifndef RPYL_POLYSYM_MAX_SCRIPT_BYTES
#define RPYL_POLYSYM_MAX_SCRIPT_BYTES 131072u
#endif

/* ---------------- IO ---------------- */
#ifndef RPYL_IO_VFS_CHUNK
#define RPYL_IO_VFS_CHUNK 4096
#endif

#ifndef RPYL_FALLBACK_AST_NODES
#define RPYL_FALLBACK_AST_NODES RPYL_AST_MAX_STATIC_NODES
#endif

#ifndef RPYL_EXTLANG_MAX_TPL
#define RPYL_EXTLANG_MAX_TPL 32
#endif

#ifndef RPYL_POLYSYM_MAX_TPL
#define RPYL_POLYSYM_MAX_TPL 32
#endif

#ifndef RPYL_RUNTIME_MAX_INSTANCES
#define RPYL_RUNTIME_MAX_INSTANCES RPYL_MAX_RUNTIMES
#endif

#endif /* RPYL_CONFIG_H */
