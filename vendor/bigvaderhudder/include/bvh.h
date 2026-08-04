#ifndef BVH_H_INCLUDED
#define BVH_H_INCLUDED

/*
  BigVaderHudder (BVH) - C89 fixed-memory HUD DSL toolkit.

  Design rules:
    - C89 source
    - caller owns one BVH_Context
    - bounded buffers and counters
    - Q16.16 fixed-point for fractional values
    - no renderer dependency
*/

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BVH_VERSION_MAJOR 2
#define BVH_VERSION_MINOR 0

#define BVH_ERRMSG_MAX 256
#define BVH_MAX_SOURCE_SIZE 131072UL
#define BVH_ARENA_SIZE 196608UL
#define BVH_MAX_SYMBOLS 2048
#define BVH_MAX_ITEMS 256
#define BVH_MAX_PROPS 1024
#define BVH_MAX_IR_OPS 2048
#define BVH_MAX_BYTECODE_SIZE 131072UL
#define BVH_MAX_VM_STACK 64
#define BVH_TOKEN_TEXT_MAX 96
#define BVH_VALUE_TEXT_MAX 128

#define BVH_NIL (-1)
#define BVH_FIXED_SHIFT 16
#define BVH_FIXED_ONE (1L << BVH_FIXED_SHIFT)

typedef long BVH_Fixed;

typedef struct BVH_Error {
    int line;
    int col;
    char message[BVH_ERRMSG_MAX];
} BVH_Error;

typedef enum BVH_TokenType {
    BVH_TOK_EOF = 0,
    BVH_TOK_EOL,
    BVH_TOK_IDENT,
    BVH_TOK_NUMBER,
    BVH_TOK_STRING,
    BVH_TOK_COLOR,
    BVH_TOK_COMMA,
    BVH_TOK_PLUS,
    BVH_TOK_MINUS,
    BVH_TOK_SLASH,
    BVH_TOK_DOTDOT,
    BVH_TOK_BLOCK_OPEN,
    BVH_TOK_BLOCK_CLOSE,
    BVH_TOK_OTHER
} BVH_TokenType;

typedef struct BVH_Token {
    BVH_TokenType type;
    const char *start;
    unsigned long len;
    int line;
    int col;
} BVH_Token;

typedef enum BVH_EntityKind {
    BVH_ENTITY_HUD = 1,
    BVH_ENTITY_NODE = 2,
    BVH_ENTITY_GROUP = 3,
    BVH_ENTITY_ANIMATION = 4
} BVH_EntityKind;

typedef enum BVH_AstKind {
    BVH_AST_HUD = 1,
    BVH_AST_NODE = 2,
    BVH_AST_GROUP = 3,
    BVH_AST_ANIMATION = 4
} BVH_AstKind;

typedef enum BVH_Anchor {
    BVH_ANCHOR_NONE = 0,
    BVH_ANCHOR_TOP_LEFT,
    BVH_ANCHOR_TOP_CENTER,
    BVH_ANCHOR_TOP_RIGHT,
    BVH_ANCHOR_CENTER,
    BVH_ANCHOR_BOTTOM_LEFT,
    BVH_ANCHOR_BOTTOM_CENTER,
    BVH_ANCHOR_BOTTOM_RIGHT
} BVH_Anchor;

typedef enum BVH_ValueKind {
    BVH_VALUE_EMPTY = 0,
    BVH_VALUE_RAW,
    BVH_VALUE_INT,
    BVH_VALUE_FIXED,
    BVH_VALUE_VEC2I,
    BVH_VALUE_COLOR,
    BVH_VALUE_ANCHOR_POS,
    BVH_VALUE_SYMBOL,
    BVH_VALUE_RANGE
} BVH_ValueKind;

typedef struct BVH_Value {
    BVH_ValueKind kind;
    long a;
    long b;
    long c;
    unsigned long u;
    int sym;
} BVH_Value;

typedef struct BVH_Arena {
    unsigned char bytes[BVH_ARENA_SIZE];
    unsigned long used;
} BVH_Arena;

typedef struct BVH_Symbol {
    unsigned long hash;
    unsigned long off;
    unsigned long len;
} BVH_Symbol;

typedef struct BVH_AstProp {
    int key_sym;
    int raw_sym;
    BVH_Value value;
    int line;
    int col;
    int next;
} BVH_AstProp;

typedef struct BVH_AstItem {
    BVH_AstKind kind;
    int name_sym;
    int first_prop;
    int last_prop;
    int first_child;
    int last_child;
    int next;
    int line;
    int col;
} BVH_AstItem;

typedef struct BVH_Program {
    int first_item;
    int last_item;
    int item_count;
    int prop_count;
} BVH_Program;

typedef enum BVH_IrCode {
    BVH_IR_BEGIN = 1,
    BVH_IR_PROP = 2,
    BVH_IR_END = 3
} BVH_IrCode;

typedef struct BVH_IrOp {
    BVH_IrCode code;
    int entity_kind;
    int name_sym;
    int key_sym;
    int raw_sym;
    BVH_Value value;
    int line;
    int col;
} BVH_IrOp;

typedef struct BVH_IR {
    BVH_IrOp ops[BVH_MAX_IR_OPS];
    int count;
} BVH_IR;

typedef struct BVH_Bytecode {
    unsigned char data[BVH_MAX_BYTECODE_SIZE];
    unsigned long len;
} BVH_Bytecode;

typedef struct BVH_Context {
    char source[BVH_MAX_SOURCE_SIZE + 1UL];
    unsigned long source_len;

    BVH_Arena arena;
    BVH_Symbol symbols[BVH_MAX_SYMBOLS];
    int symbol_count;

    BVH_Program program;
    BVH_AstItem items[BVH_MAX_ITEMS];
    BVH_AstProp props[BVH_MAX_PROPS];

    BVH_IR ir;
    BVH_Bytecode bytecode;
} BVH_Context;

typedef struct BVH_RuntimeCallbacks {
    void (*on_begin)(void *user, int entity_kind, const char *name);
    void (*on_end)(void *user, int entity_kind);
    void (*on_prop)(void *user, const char *key, const char *raw, const BVH_Value *value);
} BVH_RuntimeCallbacks;

void bvh_context_init(BVH_Context *ctx);
void bvh_build_reset(BVH_Context *ctx);

int bvh_parse_string(BVH_Context *ctx, const char *src, BVH_Error *err);
int bvh_parse_file(BVH_Context *ctx, const char *path, BVH_Error *err);

int bvh_build_ir(BVH_Context *ctx, BVH_Error *err);
int bvh_compile_program(BVH_Context *ctx, BVH_Error *err);
int bvh_exec_bytecode(const BVH_Bytecode *bc, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err);

int bvh_run_string(BVH_Context *ctx, const char *src, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err);
int bvh_run_file(BVH_Context *ctx, const char *path, const BVH_RuntimeCallbacks *callbacks, void *user, BVH_Error *err);

int bvh_transpile_json(const BVH_Context *ctx, FILE *out, int pretty, BVH_Error *err);

const char *bvh_symbol_text(const BVH_Context *ctx, int sym);
const char *bvh_token_name(BVH_TokenType type);
const char *bvh_value_kind_name(BVH_ValueKind kind);
const char *bvh_entity_kind_name(int kind);
const char *bvh_anchor_name(int anchor);

int bvh_value_format(const BVH_Context *ctx, const BVH_Value *value, char *out, unsigned long cap);

void bvh_dump_tokens(BVH_Context *ctx, FILE *out);
void bvh_dump_ast(const BVH_Context *ctx, FILE *out);
void bvh_dump_ir(const BVH_Context *ctx, FILE *out);
void bvh_dump_symbols(const BVH_Context *ctx, FILE *out);

#ifdef __cplusplus
}
#endif

#endif
