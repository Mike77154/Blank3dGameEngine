#ifndef VAR_DSL89_H
#define VAR_DSL89_H

/*
 * var_dsl89 - tiny, host-agnostic GameMaker-like variable DSL front-end.
 * C89, no heap, no float/double. CC0-1.0.
 *
 * Authoring surface:
 *   hp = 100;
 *   hp -= 25;
 *   can_fire = true;
 *   var damage = 25;
 *   global.score += 1000;
 *   self.weapon.can_fire = false;
 *   copy = other_value;
 *
 * The library owns no variable storage. It parses statements and emits
 * operations to a provider. An unqualified right-hand-side variable uses
 * RESOLVE scope so the host can implement local -> self resolution.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VDSL89_NAME_MAX
#define VDSL89_NAME_MAX 48
#endif

#ifndef VDSL89_STRING_MAX
#define VDSL89_STRING_MAX 96
#endif

#ifndef VDSL89_STATEMENT_MAX
#define VDSL89_STATEMENT_MAX 256
#endif

typedef long vdsl89_i32;
typedef unsigned long vdsl89_u32;

typedef enum vdsl89_scope {
    VDSL89_SCOPE_INSTANCE = 0,
    VDSL89_SCOPE_LOCAL = 1,
    VDSL89_SCOPE_GLOBAL = 2,
    VDSL89_SCOPE_RESOLVE = 3
} vdsl89_scope;

typedef enum vdsl89_value_type {
    VDSL89_VALUE_NONE = 0,
    VDSL89_VALUE_FIXED = 1,
    VDSL89_VALUE_BOOL = 2,
    VDSL89_VALUE_STRING = 3
} vdsl89_value_type;

/* Q16.16 fixed point. 1.0 == 65536. */
typedef struct vdsl89_value {
    vdsl89_value_type type;
    vdsl89_i32 fixed_q16;
    int boolean;
    char string_value[VDSL89_STRING_MAX];
} vdsl89_value;

typedef enum vdsl89_opcode {
    VDSL89_OP_SET = 1,
    VDSL89_OP_ADD = 2,
    VDSL89_OP_SUB = 3
} vdsl89_opcode;

typedef enum vdsl89_rhs_kind {
    VDSL89_RHS_LITERAL = 0,
    VDSL89_RHS_VARIABLE = 1
} vdsl89_rhs_kind;

typedef struct vdsl89_command {
    vdsl89_opcode opcode;
    vdsl89_scope scope;
    int declares_local;
    char name[VDSL89_NAME_MAX];
    vdsl89_value value;
    vdsl89_rhs_kind rhs_kind;
    vdsl89_scope rhs_scope;
    char rhs_name[VDSL89_NAME_MAX];
} vdsl89_command;

typedef int (*vdsl89_emit_fn)(void *user, const vdsl89_command *command);

typedef struct vdsl89_provider {
    vdsl89_emit_fn emit;
    void *user;
} vdsl89_provider;

typedef enum vdsl89_result {
    VDSL89_OK = 0,
    VDSL89_ERR_NULL = -1,
    VDSL89_ERR_SYNTAX = -2,
    VDSL89_ERR_NAME_TOO_LONG = -3,
    VDSL89_ERR_STRING_TOO_LONG = -4,
    VDSL89_ERR_BAD_NUMBER = -5,
    VDSL89_ERR_PROVIDER = -6,
    VDSL89_ERR_STATEMENT_TOO_LONG = -7
} vdsl89_result;

void vdsl89_value_none(vdsl89_value *v);
void vdsl89_value_fixed_raw(vdsl89_value *v, vdsl89_i32 raw_q16);
void vdsl89_value_bool(vdsl89_value *v, int boolean);
int vdsl89_value_string(vdsl89_value *v, const char *text);

/* Converts decimal text, e.g. "-12.75", into Q16.16 without floats. */
int vdsl89_fixed_from_text(const char *text, vdsl89_i32 *out_q16);

/* Parse one assignment/add/sub statement. Trailing semicolon is optional. */
int vdsl89_parse_line(const char *line, vdsl89_command *out_command);

/* Parse and immediately emit one statement through provider. */
int vdsl89_execute_line(const char *line, const vdsl89_provider *provider);

/* Execute a newline/semicolon separated statement buffer. Quoted strings are
 * respected; # and // comments are ignored outside strings. */
int vdsl89_execute_buffer(const char *text, const vdsl89_provider *provider,
                          int *out_statement_count);

const char *vdsl89_result_string(int result);

#ifdef __cplusplus
}
#endif

#endif
