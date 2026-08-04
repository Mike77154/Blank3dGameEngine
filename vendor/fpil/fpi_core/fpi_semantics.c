#include "fpi_semantics.h"

int fpi_semantics_validate(const FPI_AST* ast, const FPI_Registry* registry, FPI_Error* error) {
    int i;
    int j;
    const FPI_Rule* rule;
    const FPI_Term* term;
    if (!ast || !registry) return FPI_ERR_ARGUMENT;
    for (i = 0; i < ast->rule_count; i++) {
        rule = &ast->rules[i];
        if (rule->condition_count == 0 || rule->action_count == 0) {
            fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "every rule requires conditions and actions");
            return FPI_ERR_SEMANTIC;
        }
        for (j = 0; j < (int)rule->condition_count; j++) {
            term = fpi_ast_condition_at(ast, rule, j);
            if (!term || term->symbol_id < 0 || term->symbol_id >= registry->condition_count) {
                fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "invalid condition symbol id");
                return FPI_ERR_SEMANTIC;
            }
        }
        for (j = 0; j < (int)rule->action_count; j++) {
            term = fpi_ast_action_at(ast, rule, j);
            if (!term || term->symbol_id < 0 || term->symbol_id >= registry->action_count) {
                fpi_error_set(error, FPI_ERR_SEMANTIC, rule->span, "invalid action symbol id");
                return FPI_ERR_SEMANTIC;
            }
        }
    }
    return FPI_OK;
}
