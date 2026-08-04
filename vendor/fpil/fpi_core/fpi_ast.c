#include "fpi_ast.h"

static void value_ref_clear(FPI_ValueRef* value) {
    if (!value) return;
    value->text_offset = 0UL;
    value->text_length = 0;
    value->has = 0;
    value->kind = FPI_VALUE_NONE;
    value->fixed = 0L;
}

void fpi_ast_init(FPI_AST* ast) {
    if (!ast) return;
    ast->rule_count = 0;
    ast->term_count = 0;
    ast->value_pool_used = 0UL;
    ast->value_pool[0] = '\0';
}

void ast_clear_rule(AST_Rule* rule) {
    if (!rule) return;
    rule->condition_first = 0;
    rule->condition_count = 0;
    rule->action_first = 0;
    rule->action_count = 0;
    rule->priority = 0;
    rule->flags = 0;
    fpi_span_clear(&rule->span);
}

int fpi_ast_begin_rule(FPI_AST* ast, FPI_Span span, FPI_Error* error) {
    int index;
    if (!ast) return FPI_ERR_ARGUMENT;
    if (ast->rule_count >= FPI_MAX_RULES) {
        fpi_error_set(error, FPI_ERR_TOO_MANY_RULES, span, "script exceeds FPI_MAX_RULES");
        return FPI_ERR_TOO_MANY_RULES;
    }
    index = ast->rule_count++;
    ast_clear_rule(&ast->rules[index]);
    ast->rules[index].condition_first = (FPI_U16)ast->term_count;
    ast->rules[index].action_first = (FPI_U16)ast->term_count;
    ast->rules[index].span = span;
    return index;
}

AST_Rule* ast_add_rule(AST_Script* ast) {
    int index;
    FPI_Span span;
    fpi_span_clear(&span);
    index = fpi_ast_begin_rule(ast, span, 0);
    if (index < 0) return 0;
    return &ast->rules[index];
}

void ast_init(AST_Script* ast) { fpi_ast_init(ast); }

int fpi_ast_begin_actions(FPI_AST* ast, int rule_index, FPI_Error* error) {
    FPI_Rule* rule;
    if (!ast || rule_index < 0 || rule_index >= ast->rule_count) return FPI_ERR_ARGUMENT;
    rule = &ast->rules[rule_index];
    if (rule->condition_count == 0) {
        fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "rule requires at least one condition");
        return FPI_ERR_SEMANTIC;
    }
    rule->action_first = (FPI_U16)ast->term_count;
    return FPI_OK;
}

static int ast_add_term(FPI_AST* ast, FPI_Rule* rule, int is_action, int symbol_id, const FPI_ValueRef* value, FPI_Span span, FPI_Error* error) {
    FPI_Term* term;
    FPI_U16* count;
    if (ast->term_count >= FPI_MAX_TERMS) {
        fpi_error_set(error, FPI_ERR_TOO_MANY_TERMS, span, "script exceeds FPI_MAX_TERMS");
        return FPI_ERR_TOO_MANY_TERMS;
    }
    count = is_action ? &rule->action_count : &rule->condition_count;
    if (*count >= FPI_MAX_TERMS_PER_SIDE) {
        fpi_error_set(error, FPI_ERR_TOO_MANY_TERMS_IN_RULE, span, "rule side exceeds FPI_MAX_TERMS_PER_SIDE");
        return FPI_ERR_TOO_MANY_TERMS_IN_RULE;
    }
    term = &ast->terms[ast->term_count++];
    term->symbol_id = symbol_id;
    if (value) term->value = *value;
    else value_ref_clear(&term->value);
    term->span = span;
    (*count)++;
    return FPI_OK;
}

int fpi_ast_add_condition(FPI_AST* ast, int rule_index, int symbol_id, const FPI_ValueRef* value, FPI_Span span, FPI_Error* error) {
    if (!ast || rule_index < 0 || rule_index >= ast->rule_count) return FPI_ERR_ARGUMENT;
    return ast_add_term(ast, &ast->rules[rule_index], 0, symbol_id, value, span, error);
}

int fpi_ast_add_action(FPI_AST* ast, int rule_index, int symbol_id, const FPI_ValueRef* value, FPI_Span span, FPI_Error* error) {
    if (!ast || rule_index < 0 || rule_index >= ast->rule_count) return FPI_ERR_ARGUMENT;
    return ast_add_term(ast, &ast->rules[rule_index], 1, symbol_id, value, span, error);
}

int fpi_ast_pool_begin(const FPI_AST* ast, FPI_U32* mark) {
    if (!ast || !mark) return FPI_ERR_ARGUMENT;
    *mark = ast->value_pool_used;
    return FPI_OK;
}

int fpi_ast_pool_push(FPI_AST* ast, char c, FPI_Error* error, FPI_Span span) {
    if (!ast) return FPI_ERR_ARGUMENT;
    if (ast->value_pool_used >= FPI_VALUE_POOL_BYTES - 1UL) {
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "AST value pool exhausted");
        return FPI_ERR_STRING_POOL_FULL;
    }
    ast->value_pool[ast->value_pool_used++] = c;
    return FPI_OK;
}

int fpi_ast_pool_finish(FPI_AST* ast, FPI_U32 mark, FPI_ValueRef* out_ref, FPI_Error* error, FPI_Span span) {
    FPI_U32 len;
    FPI_Fixed fixed;
    int rc;
    if (!ast || !out_ref || mark > ast->value_pool_used) return FPI_ERR_ARGUMENT;
    len = ast->value_pool_used - mark;
    if (len > 65535UL) {
        fpi_ast_pool_rollback(ast, mark);
        fpi_error_set(error, FPI_ERR_VALUE_TOO_LONG, span, "single RHS exceeds 65535 bytes");
        return FPI_ERR_VALUE_TOO_LONG;
    }
    if (ast->value_pool_used >= FPI_VALUE_POOL_BYTES) {
        fpi_ast_pool_rollback(ast, mark);
        fpi_error_set(error, FPI_ERR_STRING_POOL_FULL, span, "AST value pool exhausted");
        return FPI_ERR_STRING_POOL_FULL;
    }
    ast->value_pool[ast->value_pool_used++] = '\0';
    out_ref->text_offset = mark;
    out_ref->text_length = (FPI_U16)len;
    out_ref->has = 1;
    out_ref->kind = FPI_VALUE_TEXT;
    out_ref->fixed = 0L;
    rc = fpi_fixed_parse(ast->value_pool + mark, &fixed);
    if (rc == FPI_OK) {
        out_ref->kind = FPI_VALUE_FIXED;
        out_ref->fixed = fixed;
    } else if (rc == FPI_ERR_NUMBER_OVERFLOW) {
        fpi_ast_pool_rollback(ast, mark);
        fpi_error_set(error, rc, span, "fixed-point literal is outside Q16.16 range");
        return rc;
    }
    return FPI_OK;
}

void fpi_ast_pool_rollback(FPI_AST* ast, FPI_U32 mark) {
    if (!ast || mark > ast->value_pool_used) return;
    ast->value_pool_used = mark;
    if (mark < FPI_VALUE_POOL_BYTES) ast->value_pool[mark] = '\0';
}

const char* fpi_ast_value_text(const FPI_AST* ast, const FPI_ValueRef* ref) {
    if (!ast || !ref || !ref->has || ref->text_offset >= ast->value_pool_used) return "";
    return ast->value_pool + ref->text_offset;
}

void fpi_ast_value_view(const FPI_AST* ast, const FPI_ValueRef* ref, FPI_Value* out_value) {
    if (!out_value) return;
    fpi_value_clear(out_value);
    if (!ast || !ref || !ref->has) return;
    out_value->has = 1;
    out_value->kind = ref->kind;
    out_value->fixed = ref->fixed;
    out_value->is_int = (ref->kind == FPI_VALUE_FIXED && (ref->fixed & (FPI_FIXED_ONE - 1L)) == 0L);
    out_value->i = out_value->is_int ? (int)fpi_fixed_to_int(ref->fixed) : 0;
    out_value->s = fpi_ast_value_text(ast, ref);
    out_value->len = (int)ref->text_length;
}

const FPI_Term* fpi_ast_condition_at(const FPI_AST* ast, const FPI_Rule* rule, int index) {
    int term_index;
    if (!ast || !rule || index < 0 || index >= (int)rule->condition_count) return 0;
    term_index = (int)rule->condition_first + index;
    if (term_index < 0 || term_index >= ast->term_count) return 0;
    return &ast->terms[term_index];
}

const FPI_Term* fpi_ast_action_at(const FPI_AST* ast, const FPI_Rule* rule, int index) {
    int term_index;
    if (!ast || !rule || index < 0 || index >= (int)rule->action_count) return 0;
    term_index = (int)rule->action_first + index;
    if (term_index < 0 || term_index >= ast->term_count) return 0;
    return &ast->terms[term_index];
}

FPI_U32 fpi_ast_memory_used(const FPI_AST* ast) {
    if (!ast) return 0UL;
    return (FPI_U32)ast->rule_count * (FPI_U32)sizeof(FPI_Rule) +
           (FPI_U32)ast->term_count * (FPI_U32)sizeof(FPI_Term) +
           ast->value_pool_used;
}
