#include "fpi_ir.h"

void fpi_ir_init(FPI_IR* ir) {
    if (!ir) return;
    ir->rules = 0;
    ir->rule_count = 0;
    ir->terms = 0;
    ir->term_count = 0;
}

int fpi_ir_build(const FPI_AST* ast, FPI_Arena* arena, FPI_IR* out_ir, FPI_Error* error) {
    int i;
    int j;
    int term_index;
    const FPI_Rule* rule;
    const FPI_Term* term;
    FPI_IRRule* ir_rule;
    FPI_IRTerm* ir_term;
    FPI_Span span;
    fpi_span_clear(&span);
    if (!ast || !arena || !out_ir) return FPI_ERR_ARGUMENT;
    fpi_ir_init(out_ir);
    if (ast->rule_count > 0) {
        out_ir->rules = (FPI_IRRule*)fpi_arena_alloc(arena, (FPI_U32)ast->rule_count * (FPI_U32)sizeof(FPI_IRRule), 8UL);
        if (!out_ir->rules) {
            fpi_error_set(error, FPI_ERR_ARENA_EXHAUSTED, span, "arena cannot hold IR rules");
            return FPI_ERR_ARENA_EXHAUSTED;
        }
    }
    if (ast->term_count > 0) {
        out_ir->terms = (FPI_IRTerm*)fpi_arena_alloc(arena, (FPI_U32)ast->term_count * (FPI_U32)sizeof(FPI_IRTerm), 8UL);
        if (!out_ir->terms) {
            fpi_error_set(error, FPI_ERR_ARENA_EXHAUSTED, span, "arena cannot hold IR terms");
            return FPI_ERR_ARENA_EXHAUSTED;
        }
    }
    out_ir->rule_count = ast->rule_count;
    term_index = 0;
    for (i = 0; i < ast->rule_count; i++) {
        rule = &ast->rules[i];
        ir_rule = &out_ir->rules[i];
        ir_rule->condition_first = term_index;
        ir_rule->condition_count = (int)rule->condition_count;
        for (j = 0; j < (int)rule->condition_count; j++) {
            term = fpi_ast_condition_at(ast, rule, j);
            if (!term) return FPI_ERR_SEMANTIC;
            ir_term = &out_ir->terms[term_index++];
            ir_term->name_space = FPI_NS_CONDITION;
            ir_term->symbol_id = term->symbol_id;
            fpi_ast_value_view(ast, &term->value, &ir_term->value);
        }
        ir_rule->action_first = term_index;
        ir_rule->action_count = (int)rule->action_count;
        for (j = 0; j < (int)rule->action_count; j++) {
            term = fpi_ast_action_at(ast, rule, j);
            if (!term) return FPI_ERR_SEMANTIC;
            ir_term = &out_ir->terms[term_index++];
            ir_term->name_space = FPI_NS_ACTION;
            ir_term->symbol_id = term->symbol_id;
            fpi_ast_value_view(ast, &term->value, &ir_term->value);
        }
    }
    out_ir->term_count = term_index;
    return FPI_OK;
}
