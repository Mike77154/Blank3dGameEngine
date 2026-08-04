#include "fpi_runtime.h"

#define FPI_MATCH_BYTES ((FPI_MAX_RULES + 7) / 8)

static void match_clear(unsigned char* bits) {
    int i;
    for (i = 0; i < FPI_MATCH_BYTES; i++) bits[i] = 0;
}

static void match_set(unsigned char* bits, int index) {
    bits[index >> 3] = (unsigned char)(bits[index >> 3] | (unsigned char)(1U << (index & 7)));
}

static int match_get(const unsigned char* bits, int index) {
    return (bits[index >> 3] & (unsigned char)(1U << (index & 7))) != 0;
}

static int rule_matches(void* user, const FPI_AST* ast, const FPI_Rule* rule, const FPI_Bindings* bindings, FPI_Error* error) {
    int i;
    int rc;
    const FPI_Term* term;
    FPI_Value value;
    if (!bindings || !bindings->eval_cond) {
        fpi_error_set(error, FPI_ERR_ARGUMENT, rule->span, "condition evaluator is missing");
        return FPI_ERR_ARGUMENT;
    }
    for (i = 0; i < (int)rule->condition_count; i++) {
        term = fpi_ast_condition_at(ast, rule, i);
        if (!term) {
            fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "condition term index is invalid");
            return FPI_ERR_SEMANTIC;
        }
        fpi_ast_value_view(ast, &term->value, &value);
        rc = bindings->eval_cond(user, term->symbol_id, &value);
        if (rc < 0) {
            fpi_error_set(error, rc, term->span, "condition dispatch failed");
            return rc;
        }
        if (rc == 0) return 0;
    }
    return 1;
}

static int execute_rule(void* user, const FPI_AST* ast, const FPI_Rule* rule, const FPI_Bindings* bindings, FPI_Error* error) {
    int i;
    const FPI_Term* term;
    FPI_Value value;
    if (!bindings || !bindings->exec_act) return FPI_OK;
    for (i = 0; i < (int)rule->action_count; i++) {
        term = fpi_ast_action_at(ast, rule, i);
        if (!term) {
            fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "action term index is invalid");
            return FPI_ERR_SEMANTIC;
        }
        fpi_ast_value_view(ast, &term->value, &value);
        bindings->exec_act(user, term->symbol_id, &value);
    }
    return FPI_OK;
}

int fpi_runtime_tick(void* user, const FPI_AST* ast, const FPI_Bindings* bindings, const FPI_RunOptions* options, int* out_fired, FPI_Error* error) {
    FPI_RunOptions defaults;
    const FPI_RunOptions* use_options;
    int i;
    int rc;
    int fired;
    unsigned char matched[FPI_MATCH_BYTES];
    const FPI_Rule* rule;
    if (out_fired) *out_fired = 0;
    if (!ast || !bindings) return FPI_ERR_ARGUMENT;
    defaults.stop_on_first_match = 0;
    defaults.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
    use_options = options ? options : &defaults;
    fired = 0;
    if (use_options->exec_mode == FPI_EXEC_SEQUENTIAL_IMMEDIATE) {
        for (i = 0; i < ast->rule_count; i++) {
            rule = &ast->rules[i];
            rc = rule_matches(user, ast, rule, bindings, error);
            if (rc < 0) return rc;
            if (rc == 0) continue;
            rc = execute_rule(user, ast, rule, bindings, error);
            if (rc != FPI_OK) return rc;
            fired++;
            if (use_options->stop_on_first_match) break;
        }
    } else if (use_options->exec_mode == FPI_EXEC_TWO_PHASE) {
        match_clear(matched);
        for (i = 0; i < ast->rule_count; i++) {
            rc = rule_matches(user, ast, &ast->rules[i], bindings, error);
            if (rc < 0) return rc;
            if (rc > 0) match_set(matched, i);
        }
        for (i = 0; i < ast->rule_count; i++) {
            if (!match_get(matched, i)) continue;
            rc = execute_rule(user, ast, &ast->rules[i], bindings, error);
            if (rc != FPI_OK) return rc;
            fired++;
            if (use_options->stop_on_first_match) break;
        }
    } else {
        fpi_error_set(error, FPI_ERR_ARGUMENT, ast->rule_count ? ast->rules[0].span : fpi_span_make(0UL, 0UL, 0, 0), "unknown execution mode");
        return FPI_ERR_ARGUMENT;
    }
    if (out_fired) *out_fired = fired;
    return FPI_OK;
}

int fpi_run_tick(void* user, const AST_Script* script, const FPI_Bindings* bindings, const FPI_RunOptions* options) {
    int fired;
    if (fpi_runtime_tick(user, script, bindings, options, &fired, 0) != FPI_OK) return 0;
    return fired;
}
