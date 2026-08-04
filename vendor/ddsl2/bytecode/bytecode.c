#include "bytecode/bytecode.h"

#include "common/strview.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <string.h> /* memcpy, memset, strlen */

const char *ddsl_bc_op_name(ddsl_bc_op op) {
    switch (op) {
        case DDSL_BC_NOP: return "DDSL_BC_NOP";
        case DDSL_BC_PUSH_NUM: return "DDSL_BC_PUSH_NUM";
        case DDSL_BC_PUSH_STR: return "DDSL_BC_PUSH_STR";
        case DDSL_BC_PUSH_BOOL: return "DDSL_BC_PUSH_BOOL";
        case DDSL_BC_LOAD_IDENT: return "DDSL_BC_LOAD_IDENT";
        case DDSL_BC_TEST_IDENT: return "DDSL_BC_TEST_IDENT";
        case DDSL_BC_NEG: return "DDSL_BC_NEG";
        case DDSL_BC_ADD: return "DDSL_BC_ADD";
        case DDSL_BC_SUB: return "DDSL_BC_SUB";
        case DDSL_BC_MUL: return "DDSL_BC_MUL";
        case DDSL_BC_DIV: return "DDSL_BC_DIV";
        case DDSL_BC_CMP_EQ: return "DDSL_BC_CMP_EQ";
        case DDSL_BC_CMP_NEQ: return "DDSL_BC_CMP_NEQ";
        case DDSL_BC_CMP_LT: return "DDSL_BC_CMP_LT";
        case DDSL_BC_CMP_LTE: return "DDSL_BC_CMP_LTE";
        case DDSL_BC_CMP_GT: return "DDSL_BC_CMP_GT";
        case DDSL_BC_CMP_GTE: return "DDSL_BC_CMP_GTE";
        case DDSL_BC_TRUTHY: return "DDSL_BC_TRUTHY";
        case DDSL_BC_JMP: return "DDSL_BC_JMP";
        case DDSL_BC_JMP_IF_FALSE: return "DDSL_BC_JMP_IF_FALSE";
        case DDSL_BC_JMP_IF_TRUE: return "DDSL_BC_JMP_IF_TRUE";
        case DDSL_BC_POP: return "DDSL_BC_POP";
        case DDSL_BC_STORE_TRUE: return "DDSL_BC_STORE_TRUE";
        case DDSL_BC_STORE_SET: return "DDSL_BC_STORE_SET";
        case DDSL_BC_END: return "DDSL_BC_END";
        default: return "<unknown>";
    }
}

static ddsl_bc_op map_ir_to_bc(ddsl_ir_op op) {
    switch (op) {
        case DDSL_IR_NOP: return DDSL_BC_NOP;
        case DDSL_IR_PUSH_NUM: return DDSL_BC_PUSH_NUM;
        case DDSL_IR_PUSH_STR: return DDSL_BC_PUSH_STR;
        case DDSL_IR_PUSH_BOOL: return DDSL_BC_PUSH_BOOL;
        case DDSL_IR_LOAD_IDENT: return DDSL_BC_LOAD_IDENT;
        case DDSL_IR_TEST_IDENT: return DDSL_BC_TEST_IDENT;
        case DDSL_IR_NEG: return DDSL_BC_NEG;
        case DDSL_IR_ADD: return DDSL_BC_ADD;
        case DDSL_IR_SUB: return DDSL_BC_SUB;
        case DDSL_IR_MUL: return DDSL_BC_MUL;
        case DDSL_IR_DIV: return DDSL_BC_DIV;
        case DDSL_IR_CMP_EQ: return DDSL_BC_CMP_EQ;
        case DDSL_IR_CMP_NEQ: return DDSL_BC_CMP_NEQ;
        case DDSL_IR_CMP_LT: return DDSL_BC_CMP_LT;
        case DDSL_IR_CMP_LTE: return DDSL_BC_CMP_LTE;
        case DDSL_IR_CMP_GT: return DDSL_BC_CMP_GT;
        case DDSL_IR_CMP_GTE: return DDSL_BC_CMP_GTE;
        case DDSL_IR_TRUTHY: return DDSL_BC_TRUTHY;
        case DDSL_IR_JMP: return DDSL_BC_JMP;
        case DDSL_IR_JMP_IF_FALSE: return DDSL_BC_JMP_IF_FALSE;
        case DDSL_IR_JMP_IF_TRUE: return DDSL_BC_JMP_IF_TRUE;
        case DDSL_IR_POP: return DDSL_BC_POP;
        case DDSL_IR_STORE_TRUE: return DDSL_BC_STORE_TRUE;
        case DDSL_IR_STORE_SET: return DDSL_BC_STORE_SET;
        case DDSL_IR_END: return DDSL_BC_END;
        default: break;
    }
    return DDSL_BC_NOP;
}

static const char *arena_strdup(ddsl_arena *a, const char *s) {
    size_t n;
    char *p;
    if (!a || !s) return NULL;
    n = strlen(s);
    p = (char *)ddsl_arena_alloc(a, n + 1, 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = '\0';
    return (const char *)p;
}

static ddsl_strview arena_dup_sv(ddsl_arena *arena, ddsl_strview sv) {
    ddsl_strview out;
    char *p;

    out.data = NULL;
    out.len = 0;
    if (!arena || sv.len <= 0) return out;
    p = (char *)ddsl_arena_alloc(arena, (size_t)sv.len + 1U, 1);
    if (!p) return out;
    memcpy(p, sv.data, (size_t)sv.len);
    p[sv.len] = '\0';
    out.data = p;
    out.len = sv.len;
    return out;
}

static const char *norm_key_to_arena(ddsl_arena *arena, ddsl_strview name_sv) {
    char raw[128];
    char norm[DDSL_MAX_KEY_LEN];
    ddsl_sv_to_cstr(name_sv, raw, (int)sizeof(raw));
    ddsl_norm_key(raw, norm, sizeof(norm));
    return arena_strdup(arena, norm);
}

int ddsl_bytecode_from_ir(ddsl_arena *arena, const ddsl_ir_program *ir, ddsl_bc_program **out_bc, ddsl_error *err) {
    ddsl_bc_program *bc;
    ddsl_bc_ins *code;
    int i;

    if (err) ddsl_error_clear(err);
    if (out_bc) *out_bc = NULL;

    if (!arena || !ir || !ir->code || ir->count <= 0 || !out_bc) {
        if (err) ddsl_error_set(err, 0, 0, 0, "bytecode: argumentos inválidos" );
        return 0;
    }

    bc = (ddsl_bc_program *)ddsl_arena_alloc_zero(arena, sizeof(ddsl_bc_program), sizeof(void *));
    if (!bc) {
        if (err) ddsl_error_set(err, 0, 0, 0, "bytecode: sin memoria (arena)" );
        return 0;
    }

    code = (ddsl_bc_ins *)ddsl_arena_alloc_zero(arena, (size_t)ir->count * sizeof(ddsl_bc_ins), sizeof(void *));
    if (!code) {
        if (err) ddsl_error_set(err, 0, 0, 0, "bytecode: sin memoria para code (arena)" );
        return 0;
    }

    for (i = 0; i < ir->count; ++i) {
        ddsl_ir_inst in;
        ddsl_bc_ins out;
        const char *k;

        in = ir->code[i];
        memset(&out, 0, sizeof(out));
        out.op = map_ir_to_bc(in.op);
        out.a = in.a;
        out.num = in.num;
        out.sv = arena_dup_sv(arena, in.sv);
        out.key = NULL;

        if (in.sv.len > 0 && !out.sv.data) {
            if (err) ddsl_error_set(err, 0, 0, 0, "bytecode: sin memoria para literal/ident (arena)");
            return 0;
        }

        if (in.op == DDSL_IR_STORE_SET || in.op == DDSL_IR_STORE_TRUE) {
            k = norm_key_to_arena(arena, in.sv);
            if (!k) {
                if (err) ddsl_error_set(err, 0, 0, 0, "bytecode: sin memoria para key_norm (arena)" );
                return 0;
            }
            out.key = k;
        }

        code[i] = out;
    }

    bc->code = code;
    bc->count = ir->count;
    *out_bc = bc;
    return 1;
}

int ddsl_compile_source_to_ir(ddsl_arena *arena, const char *source, ddsl_ir_program **out_ir, ddsl_error *err) {
    ddsl_token_vec toks;
    ddsl_program *prog;
    size_t mark;

    if (err) ddsl_error_clear(err);
    if (out_ir) *out_ir = NULL;

    if (!arena || !source || !out_ir) {
        if (err) ddsl_error_set(err, 0, 0, 0, "compile: argumentos inválidos" );
        return 0;
    }

    mark = ddsl_arena_mark(arena);

    ddsl_tokens_init(&toks);
    if (!ddsl_lex_all(source, &toks, err)) {
        ddsl_tokens_reset(&toks);
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    prog = NULL;
    if (!ddsl_parse_program(&toks, arena, &prog, err)) {
        ddsl_tokens_reset(&toks);
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    if (!ddsl_ir_build(arena, prog, out_ir, err)) {
        ddsl_tokens_reset(&toks);
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    ddsl_tokens_reset(&toks);
    return 1;
}

int ddsl_compile_source_to_bytecode(ddsl_arena *arena, const char *source, ddsl_bc_program **out_bc, ddsl_error *err) {
    ddsl_ir_program *ir;
    size_t mark;

    if (err) ddsl_error_clear(err);
    if (out_bc) *out_bc = NULL;

    if (!arena || !source || !out_bc) {
        if (err) ddsl_error_set(err, 0, 0, 0, "compile: argumentos inválidos" );
        return 0;
    }

    mark = ddsl_arena_mark(arena);

    ir = NULL;
    if (!ddsl_compile_source_to_ir(arena, source, &ir, err)) {
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    if (!ddsl_bytecode_from_ir(arena, ir, out_bc, err)) {
        ddsl_arena_rewind(arena, mark);
        return 0;
    }

    return 1;
}
