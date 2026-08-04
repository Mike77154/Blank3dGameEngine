#include "rpyl_ir.h"
#include "rpyl_common.h"
#include "rpyl_builtins.h"

void rpyl_ir_clear(RpylIrProgram* p) {
    size_t i;
    if (p == 0) return;
    for (i = 0u; i < RPYL_IR_MAX_OPS; i++) {
        p->ops[i].kind = RPYL_IR_NOP;
        p->ops[i].a = 0UL;
        p->ops[i].b = 0UL;
        p->ops[i].c = 0UL;
        p->ops[i].flags = 0UL;
    }
    for (i = 0u; i < RPYL_IR_MAX_ARGS; i++) p->args[i] = 0UL;
    for (i = 0u; i < RPYL_IR_MAX_STRINGS; i++) p->strings[i] = 0;
    for (i = 0u; i < RPYL_IR_MAX_LABELS; i++) {
        p->labels[i].name_sid = 0UL;
        p->labels[i].first_op = 0UL;
        p->labels[i].end_op = 0UL;
    }
    if (RPYL_IR_MAX_STRING_BYTES > 0u) p->string_data[0] = '\0';
    p->count = 0u;
    p->arg_count = 0u;
    p->string_count = 0u;
    p->string_bytes = 0u;
    p->label_count = 0u;
    p->overflowed = 0;
}

void rpyl_ir_init(RpylIrProgram* p) {
    rpyl_ir_clear(p);
}

unsigned long rpyl_ir_intern(RpylIrProgram* p, const char* text) {
    size_t i;
    size_t len;
    char* dst;
    if (!p || !text) return 0UL;
    for (i = 0u; i < p->string_count; i++) {
        if (rpyl_common_streq(p->strings[i], text)) return (unsigned long)i;
    }
    if (p->string_count >= (size_t)RPYL_IR_MAX_STRINGS) {
        p->overflowed = 1;
        return 0UL;
    }
    len = rpyl_common_strlen(text);
    if (p->string_bytes + len + 1u > (size_t)RPYL_IR_MAX_STRING_BYTES) {
        p->overflowed = 1;
        return 0UL;
    }
    dst = p->string_data + p->string_bytes;
    (void)rpyl_common_copy(dst, (size_t)RPYL_IR_MAX_STRING_BYTES - p->string_bytes, text);
    p->strings[p->string_count] = dst;
    p->string_bytes += len + 1u;
    p->string_count++;
    return (unsigned long)(p->string_count - 1u);
}

const char* rpyl_ir_string(const RpylIrProgram* p, unsigned long sid) {
    if (!p) return 0;
    if ((size_t)sid >= p->string_count) return 0;
    return p->strings[sid];
}

int rpyl_ir_emit_ex(RpylIrProgram* p, RpylIrOpKind kind, unsigned long a, unsigned long b, unsigned long c, unsigned long flags) {
    RpylIrOp* op;
    if (p == 0) return 0;
    if (p->count >= (size_t)RPYL_IR_MAX_OPS) {
        p->overflowed = 1;
        return 0;
    }
    op = &p->ops[p->count];
    op->kind = kind;
    op->a = a;
    op->b = b;
    op->c = c;
    op->flags = flags;
    p->count++;
    return 1;
}

int rpyl_ir_emit(RpylIrProgram* p, RpylIrOpKind kind, unsigned long a, unsigned long b, unsigned long c) {
    return rpyl_ir_emit_ex(p, kind, a, b, c, 0UL);
}

unsigned long rpyl_ir_emit_args(RpylIrProgram* p, char args[][RPYL_AST_MAX_ARG_TEXT], int argc) {
    unsigned long first;
    int i;
    if (!p || argc < 0) return 0UL;
    first = (unsigned long)p->arg_count;
    for (i = 0; i < argc; i++) {
        if (p->arg_count >= (size_t)RPYL_IR_MAX_ARGS) {
            p->overflowed = 1;
            return first;
        }
        p->args[p->arg_count] = rpyl_ir_intern(p, args[i]);
        p->arg_count++;
    }
    return first;
}

int rpyl_ir_add_label(RpylIrProgram* p, const char* name, unsigned long first_op) {
    RpylIrLabel* label;
    if (!p || !name || !name[0]) return 0;
    if (p->label_count >= (size_t)RPYL_IR_MAX_LABELS) {
        p->overflowed = 1;
        return 0;
    }
    label = &p->labels[p->label_count];
    label->name_sid = rpyl_ir_intern(p, name);
    label->first_op = first_op;
    label->end_op = first_op;
    p->label_count++;
    return p->overflowed ? 0 : 1;
}

int rpyl_ir_close_label(RpylIrProgram* p, const char* name, unsigned long end_op) {
    size_t i;
    const char* s;
    if (!p || !name) return 0;
    for (i = 0u; i < p->label_count; i++) {
        s = rpyl_ir_string(p, p->labels[i].name_sid);
        if (rpyl_common_streq(s, name)) {
            p->labels[i].end_op = end_op;
            return 1;
        }
    }
    return 0;
}

int rpyl_ir_find_label(const RpylIrProgram* p, const char* name, unsigned long* first_op, unsigned long* end_op) {
    size_t i;
    const char* s;
    if (!p || !name) return 0;
    for (i = 0u; i < p->label_count; i++) {
        s = rpyl_ir_string(p, p->labels[i].name_sid);
        if (rpyl_common_streq(s, name)) {
            if (first_op) *first_op = p->labels[i].first_op;
            if (end_op) *end_op = p->labels[i].end_op;
            return 1;
        }
    }
    return 0;
}

static int ir_emit_node(RpylIrProgram* p, AstNode* node);

static int ir_emit_children(RpylIrProgram* p, AstNode* first) {
    AstNode* n;
    n = first;
    while (n != 0) {
        if (!ir_emit_node(p, n)) return 0;
        n = n->next;
    }
    return p->overflowed ? 0 : 1;
}

static int ir_emit_call(RpylIrProgram* p, AstNode* node) {
    unsigned long first_arg;
    unsigned long name_sid;
    RpylBuiltinKind kind;
    if (!p || !node) return 0;
    first_arg = rpyl_ir_emit_args(p, node->args, node->arg_count);
    name_sid = rpyl_ir_intern(p, node->name);
    kind = rpyl_builtins_kind(node->name);
    if (kind == RPYL_BUILTIN_SET || kind == RPYL_BUILTIN_SET_GLOBAL) {
        unsigned long var_sid;
        unsigned long value_first;
        unsigned long value_count;
        if (node->arg_count < 1) return rpyl_ir_emit(p, RPYL_IR_COMMAND, name_sid, first_arg, (unsigned long)node->arg_count);
        var_sid = p->args[first_arg];
        value_first = first_arg + 1UL;
        value_count = (unsigned long)(node->arg_count - 1);
        return rpyl_ir_emit_ex(p, RPYL_IR_SET, var_sid, value_first, value_count, kind == RPYL_BUILTIN_SET_GLOBAL ? 1UL : 0UL);
    }
    if (kind == RPYL_BUILTIN_JUMP) return rpyl_ir_emit(p, RPYL_IR_JUMP, first_arg, (unsigned long)node->arg_count, 0UL);
    if (kind == RPYL_BUILTIN_CALL) return rpyl_ir_emit(p, RPYL_IR_CALL, first_arg, (unsigned long)node->arg_count, 0UL);
    if (kind == RPYL_BUILTIN_RETURN) return rpyl_ir_emit(p, RPYL_IR_RETURN, 0UL, 0UL, 0UL);
    return rpyl_ir_emit(p, RPYL_IR_COMMAND, name_sid, first_arg, (unsigned long)node->arg_count);
}

static int ir_emit_node(RpylIrProgram* p, AstNode* node) {
    unsigned long name_sid;
    if (!p || !node) return 0;
    if (node->type == AST_DEFINE) {
        name_sid = rpyl_ir_intern(p, node->name);
        return rpyl_ir_emit(p, RPYL_IR_DEFINE, name_sid, rpyl_ir_intern(p, node->value), 0UL);
    }
    if (node->type == AST_CALL) return ir_emit_call(p, node);
    if (node->type == AST_BLOCK) {
        if (rpyl_common_streq(node->name, "once")) {
            if (!rpyl_ir_emit(p, RPYL_IR_ENTER_ONCE, 0UL, 0UL, 0UL)) return 0;
            if (!ir_emit_children(p, node->children)) return 0;
            return rpyl_ir_emit(p, RPYL_IR_END_BLOCK, 0UL, 0UL, 0UL);
        }
        if (rpyl_common_streq(node->name, "on_enter")) {
            if (!rpyl_ir_emit(p, RPYL_IR_ENTER_ON_ENTER, 0UL, 0UL, 0UL)) return 0;
            if (!ir_emit_children(p, node->children)) return 0;
            return rpyl_ir_emit(p, RPYL_IR_END_BLOCK, 0UL, 0UL, 0UL);
        }
        name_sid = rpyl_ir_intern(p, node->name);
        if (!rpyl_ir_emit(p, RPYL_IR_LABEL, name_sid, 0UL, 0UL)) return 0;
        if (!ir_emit_children(p, node->children)) return 0;
        return rpyl_ir_emit(p, RPYL_IR_END_BLOCK, name_sid, 0UL, 0UL);
    }
    return 1;
}

int rpyl_ir_from_ast(RpylIrProgram* p, AstNode* root) {
    AstNode* n;
    if (!p || !root) return 0;
    rpyl_ir_clear(p);
    n = root->children;
    while (n != 0) {
        if (n->type == AST_BLOCK) {
            if (!rpyl_ir_add_label(p, n->name, (unsigned long)p->count)) return 0;
            if (!ir_emit_node(p, n)) return 0;
            if (!rpyl_ir_close_label(p, n->name, (unsigned long)p->count)) return 0;
        } else {
            if (!ir_emit_node(p, n)) return 0;
        }
        n = n->next;
    }
    return p->overflowed ? 0 : 1;
}

int rpyl_ir_overflowed(const RpylIrProgram* p) {
    if (!p) return 0;
    return p->overflowed ? 1 : 0;
}
