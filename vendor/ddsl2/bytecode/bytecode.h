#ifndef DDSL_BYTECODE_H
#define DDSL_BYTECODE_H

#include "arena/arena.h"
#include "error/error.h"
#include "IR/ir.h"
#include "types/fixed.h"
#include "store/store.h"   /* DDSL_MAX_KEY_LEN */
#include "common/strview.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================ Bytecode ============================
 * "Bytecode" aquí es una secuencia compacta de instrucciones.
 * No hay asignador dinámico: todo se guarda en arena.
 */

typedef enum ddsl_bc_op {
    DDSL_BC_NOP = 0,

    DDSL_BC_PUSH_NUM,
    DDSL_BC_PUSH_STR,
    DDSL_BC_PUSH_BOOL,
    DDSL_BC_LOAD_IDENT,
    DDSL_BC_TEST_IDENT,

    DDSL_BC_NEG,
    DDSL_BC_ADD,
    DDSL_BC_SUB,
    DDSL_BC_MUL,
    DDSL_BC_DIV,

    DDSL_BC_CMP_EQ,
    DDSL_BC_CMP_NEQ,
    DDSL_BC_CMP_LT,
    DDSL_BC_CMP_LTE,
    DDSL_BC_CMP_GT,
    DDSL_BC_CMP_GTE,

    DDSL_BC_TRUTHY,

    DDSL_BC_JMP,
    DDSL_BC_JMP_IF_FALSE,
    DDSL_BC_JMP_IF_TRUE,

    DDSL_BC_POP,

    /* acciones (key ya normalizada a snake_case) */
    DDSL_BC_STORE_TRUE,
    DDSL_BC_STORE_SET,

    DDSL_BC_END
} ddsl_bc_op;

typedef struct ddsl_bc_ins {
    ddsl_bc_op op;
    int a;              /* jump target o bool inmediato */
    ddsl_fixed num;         /* inmediato numérico */
    ddsl_strview sv;    /* inmediato string/ident */
    const char *key;    /* para STORE_* (key_norm) */
} ddsl_bc_ins;

typedef struct ddsl_bc_program {
    ddsl_bc_ins *code; /* dentro de arena */
    int count;
} ddsl_bc_program;

const char *ddsl_bc_op_name(ddsl_bc_op op);

/* IR -> Bytecode (normaliza keys) */
int ddsl_bytecode_from_ir(ddsl_arena *arena, const ddsl_ir_program *ir, ddsl_bc_program **out_bc, ddsl_error *err);

/* Atajos: source -> IR / bytecode */
int ddsl_compile_source_to_ir(ddsl_arena *arena, const char *source, ddsl_ir_program **out_ir, ddsl_error *err);
int ddsl_compile_source_to_bytecode(ddsl_arena *arena, const char *source, ddsl_bc_program **out_bc, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_BYTECODE_H */
