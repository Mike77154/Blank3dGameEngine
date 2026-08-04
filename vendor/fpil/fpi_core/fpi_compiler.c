#include "fpi_compiler.h"

int fpi_compile_estimate(const FPI_AST* ast, int* out_code_count, int* out_constant_count, FPI_U32* out_string_bytes) {
    int i;
    int code_count;
    FPI_U32 string_bytes;
    if (!ast) return FPI_ERR_ARGUMENT;
    code_count = 1;
    for (i = 0; i < ast->rule_count; i++) {
        code_count += (int)ast->rules[i].condition_count * 2;
        code_count += (int)ast->rules[i].action_count;
        code_count += 1;
        if (code_count > FPI_PROGRAM_MAX_CODE) return FPI_ERR_PROGRAM_INVALID;
    }
    string_bytes = 0UL;
    for (i = 0; i < ast->term_count; i++) {
        if (ast->terms[i].value.has)
            string_bytes += (FPI_U32)ast->terms[i].value.text_length + 1UL;
    }
    if (out_code_count) *out_code_count = code_count;
    if (out_constant_count) *out_constant_count = ast->term_count;
    if (out_string_bytes) *out_string_bytes = string_bytes;
    return FPI_OK;
}

static int copy_ir_value(const FPI_Value* source, FPI_Value* destination, char* string_pool, FPI_U32 string_capacity, FPI_U32* string_used) {
    int i;
    FPI_U32 needed;
    if (!source || !destination || !string_used) return FPI_ERR_ARGUMENT;
    *destination = *source;
    if (!source->has) {
        destination->s = "";
        destination->len = 0;
        return FPI_OK;
    }
    needed = (FPI_U32)source->len + 1UL;
    if (!string_pool || needed > string_capacity - *string_used) return FPI_ERR_ARENA_EXHAUSTED;
    destination->s = string_pool + *string_used;
    for (i = 0; i < source->len; i++) string_pool[*string_used + (FPI_U32)i] = source->s[i];
    string_pool[*string_used + (FPI_U32)source->len] = '\0';
    *string_used += needed;
    return FPI_OK;
}

int fpi_compile_ir(const FPI_IR* ir, FPI_U32 generation, FPI_Arena* arena, FPI_Program* out_program, FPI_Error* error) {
    int code_count;
    int i;
    int j;
    int ip;
    int const_index;
    int jump_start;
    FPI_U32 string_bytes;
    FPI_U32 string_used;
    char* string_pool;
    const FPI_IRRule* ir_rule;
    const FPI_IRTerm* ir_term;
    FPI_ProgramRule* program_rule;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!ir || !arena || !out_program) return FPI_ERR_ARGUMENT;
    fpi_program_init(out_program);
    code_count = 1;
    string_bytes = 0UL;
    for (i = 0; i < ir->rule_count; i++) {
        code_count += ir->rules[i].condition_count * 2 + ir->rules[i].action_count + 1;
    }
    if (code_count > FPI_PROGRAM_MAX_CODE || ir->term_count > FPI_PROGRAM_MAX_CONSTS) {
        fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, span, "compiled program exceeds configured limits");
        return FPI_ERR_PROGRAM_INVALID;
    }
    for (i = 0; i < ir->term_count; i++) {
        if (ir->terms[i].value.has) string_bytes += (FPI_U32)ir->terms[i].value.len + 1UL;
    }
    out_program->code = (FPI_Instruction*)fpi_arena_alloc(arena, (FPI_U32)code_count * (FPI_U32)sizeof(FPI_Instruction), 8UL);
    out_program->constants = ir->term_count > 0 ? (FPI_Value*)fpi_arena_alloc(arena, (FPI_U32)ir->term_count * (FPI_U32)sizeof(FPI_Value), 8UL) : 0;
    out_program->rules = ir->rule_count > 0 ? (FPI_ProgramRule*)fpi_arena_alloc(arena, (FPI_U32)ir->rule_count * (FPI_U32)sizeof(FPI_ProgramRule), 8UL) : 0;
    string_pool = string_bytes > 0UL ? (char*)fpi_arena_alloc(arena, string_bytes, 1UL) : 0;
    if (!out_program->code || (ir->term_count > 0 && !out_program->constants) ||
        (ir->rule_count > 0 && !out_program->rules) || (string_bytes > 0UL && !string_pool)) {
        fpi_program_init(out_program);
        fpi_error_set(error, FPI_ERR_ARENA_EXHAUSTED, span, "arena cannot hold compiled program");
        return FPI_ERR_ARENA_EXHAUSTED;
    }
    out_program->code_count = code_count;
    out_program->constant_count = ir->term_count;
    out_program->rule_count = ir->rule_count;
    out_program->source_generation = generation;
    string_used = 0UL;
    ip = 0;
    const_index = 0;
    for (i = 0; i < ir->rule_count; i++) {
        ir_rule = &ir->rules[i];
        program_rule = &out_program->rules[i];
        program_rule->condition_ip = ip;
        jump_start = ip;
        for (j = 0; j < ir_rule->condition_count; j++) {
            ir_term = &ir->terms[ir_rule->condition_first + j];
            if (copy_ir_value(&ir_term->value, &out_program->constants[const_index], string_pool, string_bytes, &string_used) != FPI_OK)
                return FPI_ERR_ARENA_EXHAUSTED;
            out_program->code[ip].opcode = FPI_OP_EVAL_COND;
            out_program->code[ip].a = ir_term->symbol_id;
            out_program->code[ip].b = const_index;
            ip++;
            out_program->code[ip].opcode = FPI_OP_JUMP_IF_FALSE;
            out_program->code[ip].a = 0;
            out_program->code[ip].b = 0;
            ip++;
            const_index++;
        }
        program_rule->action_ip = ip;
        for (j = 0; j < ir_rule->action_count; j++) {
            ir_term = &ir->terms[ir_rule->action_first + j];
            if (copy_ir_value(&ir_term->value, &out_program->constants[const_index], string_pool, string_bytes, &string_used) != FPI_OK)
                return FPI_ERR_ARENA_EXHAUSTED;
            out_program->code[ip].opcode = FPI_OP_EXEC_ACT;
            out_program->code[ip].a = ir_term->symbol_id;
            out_program->code[ip].b = const_index;
            ip++;
            const_index++;
        }
        out_program->code[ip].opcode = FPI_OP_RULE_FIRED;
        out_program->code[ip].a = i;
        out_program->code[ip].b = 0;
        ip++;
        program_rule->end_ip = ip;
        while (jump_start < program_rule->action_ip) {
            if (out_program->code[jump_start].opcode == FPI_OP_JUMP_IF_FALSE)
                out_program->code[jump_start].a = program_rule->end_ip;
            jump_start++;
        }
    }
    out_program->code[ip].opcode = FPI_OP_HALT;
    out_program->code[ip].a = 0;
    out_program->code[ip].b = 0;
    return fpi_program_validate(out_program, error);
}

int fpi_compile_ast(const FPI_AST* ast, FPI_U32 generation, FPI_Arena* arena, FPI_Program* out_program, FPI_Error* error) {
    FPI_IR ir;
    int rc;
    if (!ast || !arena || !out_program) return FPI_ERR_ARGUMENT;
    rc = fpi_ir_build(ast, arena, &ir, error);
    if (rc != FPI_OK) return rc;
    return fpi_compile_ir(&ir, generation, arena, out_program, error);
}
