#ifndef RPYL_IR_H
#define RPYL_IR_H

#include <stddef.h>
#include "rpyl_config.h"
#include "rpyl_ast.h"

#ifndef RPYL_IR_MAX_OPS
#define RPYL_IR_MAX_OPS 4096
#endif
#ifndef RPYL_IR_MAX_ARGS
#define RPYL_IR_MAX_ARGS 8192
#endif
#ifndef RPYL_IR_MAX_STRINGS
#define RPYL_IR_MAX_STRINGS 2048
#endif
#ifndef RPYL_IR_MAX_STRING_BYTES
#define RPYL_IR_MAX_STRING_BYTES 65536u
#endif
#ifndef RPYL_IR_MAX_LABELS
#define RPYL_IR_MAX_LABELS 512
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylIrOpKind {
    RPYL_IR_NOP = 0,
    RPYL_IR_DEFINE = 1,
    RPYL_IR_LABEL = 2,
    RPYL_IR_COMMAND = 3,
    RPYL_IR_SET = 4,
    RPYL_IR_JUMP = 5,
    RPYL_IR_CALL = 6,
    RPYL_IR_RETURN = 7,
    RPYL_IR_ENTER_ONCE = 8,
    RPYL_IR_ENTER_ON_ENTER = 9,
    RPYL_IR_END_BLOCK = 10
} RpylIrOpKind;

typedef struct RpylIrOp {
    RpylIrOpKind kind;
    unsigned long a;
    unsigned long b;
    unsigned long c;
    unsigned long flags;
} RpylIrOp;

typedef struct RpylIrLabel {
    unsigned long name_sid;
    unsigned long first_op;
    unsigned long end_op;
} RpylIrLabel;

typedef struct RpylIrProgram {
    RpylIrOp ops[RPYL_IR_MAX_OPS];
    size_t count;

    unsigned long args[RPYL_IR_MAX_ARGS];
    size_t arg_count;

    const char* strings[RPYL_IR_MAX_STRINGS];
    char string_data[RPYL_IR_MAX_STRING_BYTES];
    size_t string_count;
    size_t string_bytes;

    RpylIrLabel labels[RPYL_IR_MAX_LABELS];
    size_t label_count;

    int overflowed;
} RpylIrProgram;

void rpyl_ir_init(RpylIrProgram* p);
void rpyl_ir_clear(RpylIrProgram* p);
unsigned long rpyl_ir_intern(RpylIrProgram* p, const char* text);
const char* rpyl_ir_string(const RpylIrProgram* p, unsigned long sid);
int rpyl_ir_emit(RpylIrProgram* p, RpylIrOpKind kind, unsigned long a, unsigned long b, unsigned long c);
int rpyl_ir_emit_ex(RpylIrProgram* p, RpylIrOpKind kind, unsigned long a, unsigned long b, unsigned long c, unsigned long flags);
unsigned long rpyl_ir_emit_args(RpylIrProgram* p, char args[][RPYL_AST_MAX_ARG_TEXT], int argc);
int rpyl_ir_add_label(RpylIrProgram* p, const char* name, unsigned long first_op);
int rpyl_ir_close_label(RpylIrProgram* p, const char* name, unsigned long end_op);
int rpyl_ir_find_label(const RpylIrProgram* p, const char* name, unsigned long* first_op, unsigned long* end_op);
int rpyl_ir_from_ast(RpylIrProgram* p, AstNode* root);
int rpyl_ir_overflowed(const RpylIrProgram* p);

#ifdef __cplusplus
}
#endif

#endif
