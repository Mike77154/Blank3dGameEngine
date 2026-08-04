#include "IR/ir.h"
#include "semantics/semantics.h"

#include <string.h> /* memset */

const char *ddsl_ir_op_name(ddsl_ir_op op) {
    switch (op) {
        case DDSL_IR_NOP: return "DDSL_IR_NOP";
        case DDSL_IR_PUSH_NUM: return "DDSL_IR_PUSH_NUM";
        case DDSL_IR_PUSH_STR: return "DDSL_IR_PUSH_STR";
        case DDSL_IR_PUSH_BOOL: return "DDSL_IR_PUSH_BOOL";
        case DDSL_IR_LOAD_IDENT: return "DDSL_IR_LOAD_IDENT";
        case DDSL_IR_TEST_IDENT: return "DDSL_IR_TEST_IDENT";
        case DDSL_IR_NEG: return "DDSL_IR_NEG";
        case DDSL_IR_ADD: return "DDSL_IR_ADD";
        case DDSL_IR_SUB: return "DDSL_IR_SUB";
        case DDSL_IR_MUL: return "DDSL_IR_MUL";
        case DDSL_IR_DIV: return "DDSL_IR_DIV";
        case DDSL_IR_CMP_EQ: return "DDSL_IR_CMP_EQ";
        case DDSL_IR_CMP_NEQ: return "DDSL_IR_CMP_NEQ";
        case DDSL_IR_CMP_LT: return "DDSL_IR_CMP_LT";
        case DDSL_IR_CMP_LTE: return "DDSL_IR_CMP_LTE";
        case DDSL_IR_CMP_GT: return "DDSL_IR_CMP_GT";
        case DDSL_IR_CMP_GTE: return "DDSL_IR_CMP_GTE";
        case DDSL_IR_TRUTHY: return "DDSL_IR_TRUTHY";
        case DDSL_IR_JMP: return "DDSL_IR_JMP";
        case DDSL_IR_JMP_IF_FALSE: return "DDSL_IR_JMP_IF_FALSE";
        case DDSL_IR_JMP_IF_TRUE: return "DDSL_IR_JMP_IF_TRUE";
        case DDSL_IR_POP: return "DDSL_IR_POP";
        case DDSL_IR_STORE_TRUE: return "DDSL_IR_STORE_TRUE";
        case DDSL_IR_STORE_SET: return "DDSL_IR_STORE_SET";
        case DDSL_IR_END: return "DDSL_IR_END";
        default: return "<unknown>";
    }
}

typedef struct ir_builder {
    ddsl_ir_inst *code;
    int count;
    int cap;
    ddsl_error *err;
} ir_builder;

static int ir_emit(ir_builder *b, ddsl_ir_inst inst) {
    if (!b || !b->code) return -1;
    if (b->count >= b->cap) {
        if (b->err) ddsl_error_set(b->err, 0, 0, 0, "ir: sin espacio (cap insuficiente)" );
        return -1;
    }
    b->code[b->count] = inst;
    return b->count++;
}

static int ir_emit_op0(ir_builder *b, ddsl_ir_op op) {
    ddsl_ir_inst in;
    memset(&in, 0, sizeof(in));
    in.op = op;
    return ir_emit(b, in);
}

static int ir_emit_num(ir_builder *b, ddsl_fixed num) {
    ddsl_ir_inst in;
    memset(&in, 0, sizeof(in));
    in.op = DDSL_IR_PUSH_NUM;
    in.num = num;
    return ir_emit(b, in);
}

static int ir_emit_bool(ir_builder *b, int v) {
    ddsl_ir_inst in;
    memset(&in, 0, sizeof(in));
    in.op = DDSL_IR_PUSH_BOOL;
    in.a = v ? 1 : 0;
    return ir_emit(b, in);
}

static int ir_emit_sv(ir_builder *b, ddsl_ir_op op, ddsl_strview sv) {
    ddsl_ir_inst in;
    memset(&in, 0, sizeof(in));
    in.op = op;
    in.sv = sv;
    return ir_emit(b, in);
}

static int ir_emit_jmp(ir_builder *b, ddsl_ir_op op, int target_placeholder) {
    ddsl_ir_inst in;
    memset(&in, 0, sizeof(in));
    in.op = op;
    in.a = target_placeholder;
    return ir_emit(b, in);
}

static void ir_patch(ir_builder *b, int at, int target) {
    if (!b || !b->code) return;
    if (at < 0 || at >= b->count) return;
    b->code[at].a = target;
}

/* ============================ Counting ============================ */

static int count_value(ddsl_expr *e);
static int count_bool(ddsl_expr *e);

static int count_bool(ddsl_expr *e) {
    ddsl_op op;
    int n;

    if (!e) return 0;

    if (e->kind == DDSL_EXPR_IDENT) {
        return 1; /* TEST_IDENT */
    }

    if (e->kind == DDSL_EXPR_BINARY) {
        op = e->as.binary.op;
        if (op == DDSL_OP_AND || op == DDSL_OP_OR) {
            /* lhs, jmp_if_*, rhs, jmp_end, push_bool(shortcircuit) */
            n = 0;
            n += count_bool(e->as.binary.lhs);
            n += 1; /* jmp_if_false/true */
            n += count_bool(e->as.binary.rhs);
            n += 1; /* jmp end */
            n += 1; /* push_bool */
            return n;
        }

        if (op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
            op == DDSL_OP_GT || op == DDSL_OP_GTE) {
            return count_value(e->as.binary.lhs) + count_value(e->as.binary.rhs) + 1;
        }
    }

    /* fallback: eval value + truthy */
    return count_value(e) + 1;
}

static int count_value(ddsl_expr *e) {
    ddsl_op op;
    if (!e) return 0;

    switch (e->kind) {
        case DDSL_EXPR_NUMBER:
        case DDSL_EXPR_STRING:
        case DDSL_EXPR_IDENT:
            return 1;
        case DDSL_EXPR_UNARY:
            return count_value(e->as.unary.rhs) + 1;
        case DDSL_EXPR_BINARY:
            op = e->as.binary.op;
            if (op == DDSL_OP_AND || op == DDSL_OP_OR ||
                op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
                op == DDSL_OP_GT || op == DDSL_OP_GTE) {
                return count_bool(e);
            }
            return count_value(e->as.binary.lhs) + count_value(e->as.binary.rhs) + 1;
        default:
            break;
    }
    return 0;
}

static int count_actions(ddsl_action *a) {
    int n;
    n = 0;
    while (a) {
        if (a->kind == DDSL_ACTION_FLAG) {
            n += 1;
        } else {
            n += count_value(a->value);
            n += 1;
        }
        a = a->next;
    }
    return n;
}

static int count_stmt(ddsl_stmt *s) {
    int n;
    if (!s) return 0;

    if (s->kind == DDSL_STMT_ACTIONS) {
        return count_actions(s->as.actions.actions);
    }
    if (s->kind == DDSL_STMT_EXPR) {
        return count_bool(s->as.expr.expr) + 1; /* POP */
    }
    if (s->kind == DDSL_STMT_IF) {
        ddsl_if_clause *c;
        n = 0;
        for (c = s->as.ifs.clauses; c; c = c->next) {
            if (c->is_else) {
                n += count_actions(c->actions);
                break;
            } else {
                n += count_bool(c->cond);
                n += 1; /* jmp_if_false */
                n += count_actions(c->actions);
                n += 1; /* jmp end */
            }
        }
        return n;
    }

    return 0;
}

static int count_program(const ddsl_program *p) {
    ddsl_stmt *s;
    int n;
    n = 0;
    if (!p) return 0;
    for (s = p->first; s; s = s->next) {
        n += count_stmt(s);
    }
    return n;
}

/* ============================ Build ============================ */

static int emit_bin_op(ir_builder *b, ddsl_op op) {
    if (op == DDSL_OP_PLUS) return ir_emit_op0(b, DDSL_IR_ADD);
    if (op == DDSL_OP_MINUS) return ir_emit_op0(b, DDSL_IR_SUB);
    if (op == DDSL_OP_MUL) return ir_emit_op0(b, DDSL_IR_MUL);
    if (op == DDSL_OP_DIV) return ir_emit_op0(b, DDSL_IR_DIV);
    return -1;
}

static int emit_cmp_op(ir_builder *b, ddsl_op op) {
    if (op == DDSL_OP_EQ) return ir_emit_op0(b, DDSL_IR_CMP_EQ);
    if (op == DDSL_OP_NEQ) return ir_emit_op0(b, DDSL_IR_CMP_NEQ);
    if (op == DDSL_OP_LT) return ir_emit_op0(b, DDSL_IR_CMP_LT);
    if (op == DDSL_OP_LTE) return ir_emit_op0(b, DDSL_IR_CMP_LTE);
    if (op == DDSL_OP_GT) return ir_emit_op0(b, DDSL_IR_CMP_GT);
    if (op == DDSL_OP_GTE) return ir_emit_op0(b, DDSL_IR_CMP_GTE);
    return -1;
}

static int build_value(ir_builder *b, ddsl_expr *e);
static int build_bool(ir_builder *b, ddsl_expr *e);

static int build_bool(ir_builder *b, ddsl_expr *e) {
    ddsl_op op;
    int j;
    int jend;
    int false_label;
    int end_label;

    if (!b || !e) return 0;

    if (e->kind == DDSL_EXPR_IDENT) {
        return (ir_emit_sv(b, DDSL_IR_TEST_IDENT, e->as.ident) >= 0) ? 1 : 0;
    }

    if (e->kind == DDSL_EXPR_BINARY) {
        op = e->as.binary.op;

        if (op == DDSL_OP_AND) {
            if (!build_bool(b, e->as.binary.lhs)) return 0;
            j = ir_emit_jmp(b, DDSL_IR_JMP_IF_FALSE, -1);
            if (j < 0) return 0;
            if (!build_bool(b, e->as.binary.rhs)) return 0;
            jend = ir_emit_jmp(b, DDSL_IR_JMP, -1);
            if (jend < 0) return 0;
            false_label = b->count;
            if (ir_emit_bool(b, 0) < 0) return 0;
            end_label = b->count;
            ir_patch(b, j, false_label);
            ir_patch(b, jend, end_label);
            return 1;
        }

        if (op == DDSL_OP_OR) {
            if (!build_bool(b, e->as.binary.lhs)) return 0;
            j = ir_emit_jmp(b, DDSL_IR_JMP_IF_TRUE, -1);
            if (j < 0) return 0;
            if (!build_bool(b, e->as.binary.rhs)) return 0;
            jend = ir_emit_jmp(b, DDSL_IR_JMP, -1);
            if (jend < 0) return 0;
            false_label = b->count;
            if (ir_emit_bool(b, 1) < 0) return 0;
            end_label = b->count;
            ir_patch(b, j, false_label);
            ir_patch(b, jend, end_label);
            return 1;
        }

        if (op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
            op == DDSL_OP_GT || op == DDSL_OP_GTE) {
            if (!build_value(b, e->as.binary.lhs)) return 0;
            if (!build_value(b, e->as.binary.rhs)) return 0;
            return (emit_cmp_op(b, op) >= 0) ? 1 : 0;
        }
    }

    /* fallback: eval value + truthy */
    if (!build_value(b, e)) return 0;
    return (ir_emit_op0(b, DDSL_IR_TRUTHY) >= 0) ? 1 : 0;
}

static int build_value(ir_builder *b, ddsl_expr *e) {
    ddsl_op op;

    if (!b || !e) return 0;

    switch (e->kind) {
        case DDSL_EXPR_NUMBER:
            return (ir_emit_num(b, e->as.number) >= 0) ? 1 : 0;
        case DDSL_EXPR_STRING:
            return (ir_emit_sv(b, DDSL_IR_PUSH_STR, e->as.string) >= 0) ? 1 : 0;
        case DDSL_EXPR_IDENT:
            return (ir_emit_sv(b, DDSL_IR_LOAD_IDENT, e->as.ident) >= 0) ? 1 : 0;
        case DDSL_EXPR_UNARY:
            if (e->as.unary.op == DDSL_OP_MINUS) {
                if (!build_value(b, e->as.unary.rhs)) return 0;
                return (ir_emit_op0(b, DDSL_IR_NEG) >= 0) ? 1 : 0;
            }
            return 0;
        case DDSL_EXPR_BINARY:
            op = e->as.binary.op;
            if (op == DDSL_OP_AND || op == DDSL_OP_OR ||
                op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
                op == DDSL_OP_GT || op == DDSL_OP_GTE) {
                return build_bool(b, e);
            }
            if (!build_value(b, e->as.binary.lhs)) return 0;
            if (!build_value(b, e->as.binary.rhs)) return 0;
            return (emit_bin_op(b, op) >= 0) ? 1 : 0;
        default:
            break;
    }
    return 0;
}

static int build_actions(ir_builder *b, ddsl_action *a) {
    while (a) {
        if (a->kind == DDSL_ACTION_FLAG) {
            if (ir_emit_sv(b, DDSL_IR_STORE_TRUE, a->name) < 0) return 0;
        } else {
            if (!build_value(b, a->value)) return 0;
            if (ir_emit_sv(b, DDSL_IR_STORE_SET, a->name) < 0) return 0;
        }
        a = a->next;
    }
    return 1;
}

static int build_stmt(ir_builder *b, ddsl_stmt *s) {
    if (!b || !s) return 1;

    if (s->kind == DDSL_STMT_ACTIONS) {
        return build_actions(b, s->as.actions.actions);
    }

    if (s->kind == DDSL_STMT_EXPR) {
        if (!build_bool(b, s->as.expr.expr)) return 0;
        return (ir_emit_op0(b, DDSL_IR_POP) >= 0) ? 1 : 0;
    }

    if (s->kind == DDSL_STMT_IF) {
        ddsl_if_clause *c;
        int end_jump_head;
        int jfalse;
        int jend;
        int next_label;
        int end_label;
        int next_jump;

        /* Lista enlazada temporal dentro del campo a de cada JMP.
         * Evita un array fijo y permite cualquier número de elif dentro
         * de la capacidad general del AST/IR, sin heap.
         */
        end_jump_head = -1;

        for (c = s->as.ifs.clauses; c; c = c->next) {
            if (c->is_else) {
                if (!build_actions(b, c->actions)) return 0;
                break;
            }

            if (!build_bool(b, c->cond)) return 0;
            jfalse = ir_emit_jmp(b, DDSL_IR_JMP_IF_FALSE, -1);
            if (jfalse < 0) return 0;

            if (!build_actions(b, c->actions)) return 0;

            jend = ir_emit_jmp(b, DDSL_IR_JMP, end_jump_head);
            if (jend < 0) return 0;
            end_jump_head = jend;

            next_label = b->count;
            ir_patch(b, jfalse, next_label);
        }

        end_label = b->count;
        while (end_jump_head >= 0) {
            next_jump = b->code[end_jump_head].a;
            ir_patch(b, end_jump_head, end_label);
            end_jump_head = next_jump;
        }

        /* si no hubo ELSE, el último JMP_IF_FALSE ya quedó parcheado al final */
        return 1;
    }

    return 1;
}

int ddsl_ir_build(ddsl_arena *arena, const ddsl_program *prog, ddsl_ir_program **out_ir, ddsl_error *err) {
    ddsl_ir_program *ir;
    ir_builder b;
    int cap;
    ddsl_stmt *s;

    if (err) ddsl_error_clear(err);
    if (out_ir) *out_ir = NULL;

    if (!arena || !prog || !out_ir) {
        if (err) ddsl_error_set(err, 0, 0, 0, "ir: argumentos inválidos" );
        return 0;
    }

    if (!ddsl_semantics_check_program(prog, err)) return 0;

    cap = count_program(prog) + 1; /* END */
    if (cap < 1) cap = 1;

    ir = (ddsl_ir_program *)ddsl_arena_alloc_zero(arena, sizeof(ddsl_ir_program), sizeof(void *));
    if (!ir) {
        if (err) ddsl_error_set(err, 0, 0, 0, "ir: sin memoria (arena)" );
        return 0;
    }

    ir->code = (ddsl_ir_inst *)ddsl_arena_alloc_zero(arena, (size_t)cap * sizeof(ddsl_ir_inst), sizeof(void *));
    if (!ir->code) {
        if (err) ddsl_error_set(err, 0, 0, 0, "ir: sin memoria para code (arena)" );
        return 0;
    }

    b.code = ir->code;
    b.count = 0;
    b.cap = cap;
    b.err = err;

    for (s = prog->first; s; s = s->next) {
        if (!build_stmt(&b, s)) return 0;
    }

    if (ir_emit_op0(&b, DDSL_IR_END) < 0) return 0;
    ir->count = b.count;
    *out_ir = ir;
    return 1;
}
