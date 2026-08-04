#ifndef DDSL_IR_H
#define DDSL_IR_H

#include "ast/ast.h"
#include "arena/arena.h"
#include "error/error.h"
#include "types/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================ IR ============================
 * IR minimalista tipo "stack machine" con saltos.
 *
 * Se genera desde el AST y luego se baja a bytecode.
 */

typedef enum ddsl_ir_op {
    DDSL_IR_NOP = 0,

    /* stack */
    DDSL_IR_PUSH_NUM,
    DDSL_IR_PUSH_STR,
    DDSL_IR_PUSH_BOOL,
    DDSL_IR_LOAD_IDENT,
    DDSL_IR_TEST_IDENT,

    DDSL_IR_NEG,
    DDSL_IR_ADD,
    DDSL_IR_SUB,
    DDSL_IR_MUL,
    DDSL_IR_DIV,

    DDSL_IR_CMP_EQ,
    DDSL_IR_CMP_NEQ,
    DDSL_IR_CMP_LT,
    DDSL_IR_CMP_LTE,
    DDSL_IR_CMP_GT,
    DDSL_IR_CMP_GTE,

    DDSL_IR_TRUTHY,

    /* control flow */
    DDSL_IR_JMP,
    DDSL_IR_JMP_IF_FALSE,
    DDSL_IR_JMP_IF_TRUE,

    /* misc */
    DDSL_IR_POP,

    /* actions */
    DDSL_IR_STORE_TRUE,
    DDSL_IR_STORE_SET,

    DDSL_IR_END
} ddsl_ir_op;

typedef struct ddsl_ir_inst {
    ddsl_ir_op op;
    int a;            /* jump target o bool inmediato */
    ddsl_fixed num;       /* inmediato numérico */
    ddsl_strview sv;  /* inmediato string/ident */
} ddsl_ir_inst;

typedef struct ddsl_ir_program {
    ddsl_ir_inst *code; /* dentro de la arena */
    int count;
} ddsl_ir_program;

const char *ddsl_ir_op_name(ddsl_ir_op op);

/* AST -> IR */
int ddsl_ir_build(ddsl_arena *arena, const ddsl_program *prog, ddsl_ir_program **out_ir, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_IR_H */
