#ifndef RPYL_BYTECODE_H
#define RPYL_BYTECODE_H

/*
    rpyl_bytecode.h

    Tiny bounded bytecode format for RPYL.

    Design goals:
    - C89-friendly (no stdint.h, no C99 features required).
    - Mutable bytecode owns fixed internal tables.
    - External/transpiled bytecode can still point at static const arrays.
*/

#include <stddef.h> /* size_t */

#include "rpyl_config.h"
#include "rpyl_types.h"
#include "rpyl_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPYL_BC_VERSION 1

typedef enum {
    RPYL_BC_OP_NOP = 0,
    RPYL_BC_OP_CMD = 1,
    RPYL_BC_OP_SET = 2,
    RPYL_BC_OP_CALL = 3,
    RPYL_BC_OP_JUMP = 4,
    RPYL_BC_OP_RETURN = 5,
    RPYL_BC_OP_ONCE_CHECK = 6,
    RPYL_BC_OP_ON_ENTER_CHECK = 7,
    RPYL_BC_OP_END = 8
} RpylBcOp;

#define RPYL_BC_SET_GLOBAL 1

typedef struct {
    rpyl_u32 name_sid;
    rpyl_u32 ip;
    rpyl_u32 end_ip;
} RpylBcLabel;

typedef struct {
    rpyl_u32 name_sid;
    rpyl_u32 value_sid;
} RpylBcDefine;

typedef struct RpylBytecode {
    rpyl_u32 version;

    /* Active views. Mutable bytecode points these at the storage below. */
    rpyl_u32* code;
    size_t code_count;
    size_t code_cap;

    const char** strings;
    size_t string_count;
    size_t string_cap;

    RpylBcLabel* labels;
    size_t label_count;
    size_t label_cap;

    RpylBcDefine* defines;
    size_t define_count;
    size_t define_cap;

    int next_once_id;
    int overflowed;
    int from_pool;
    size_t string_bytes;

    /* Fixed storage for mutable bytecode owned by a context. */
    rpyl_u32 code_storage[RPYL_BC_MAX_CODE];
    const char* string_storage[RPYL_BC_MAX_STRINGS];
    char string_data[RPYL_BC_MAX_STRING_BYTES];
    RpylBcLabel label_storage[RPYL_BC_MAX_LABELS];
    RpylBcDefine define_storage[RPYL_BC_MAX_DEFINES];
} RpylBytecode;

RpylBytecode* rpyl_bytecode_create(void);
void rpyl_bytecode_destroy(RpylBytecode* bc);
void rpyl_bytecode_clear(RpylBytecode* bc);

int rpyl_bytecode_compile_ast(RpylBytecode* out, AstNode* root);
size_t rpyl_bytecode_save_buffer(const RpylBytecode* bc, void* out_data, size_t out_capacity);
int rpyl_bytecode_load_buffer(RpylBytecode* bc, const void* data, size_t len);
int rpyl_bytecode_save(const RpylBytecode* bc, const char* path);
int rpyl_bytecode_load_into(RpylBytecode* bc, const char* path);
int rpyl_bytecode_copy_into(RpylBytecode* dst, const RpylBytecode* src);
int rpyl_bytecode_copy(RpylBytecode* dst, const RpylBytecode* src);

#ifdef __cplusplus
}
#endif

#endif /* RPYL_BYTECODE_H */
