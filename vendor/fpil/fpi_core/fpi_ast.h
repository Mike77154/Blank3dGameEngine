#ifndef FPI_AST_H
#define FPI_AST_H

#include "fpi_registry.h"

typedef struct FPI_Term {
    int symbol_id;
    FPI_ValueRef value;
    FPI_Span span;
} FPI_Term;

typedef struct FPI_Rule {
    FPI_U16 condition_first;
    FPI_U16 condition_count;
    FPI_U16 action_first;
    FPI_U16 action_count;
    int priority;
    int flags;
    FPI_Span span;
} FPI_Rule;

typedef struct FPI_AST {
    FPI_Rule rules[FPI_MAX_RULES];
    int rule_count;
    FPI_Term terms[FPI_MAX_TERMS];
    int term_count;
    char value_pool[FPI_VALUE_POOL_BYTES];
    FPI_U32 value_pool_used;
} FPI_AST;

/* Compatibility aliases used by the previous release. */
typedef FPI_AST AST_Script;
typedef FPI_Rule AST_Rule;
typedef FPI_Term AST_Term;

void fpi_ast_init(FPI_AST* ast);
int fpi_ast_begin_rule(FPI_AST* ast, FPI_Span span, FPI_Error* error);
int fpi_ast_begin_actions(FPI_AST* ast, int rule_index, FPI_Error* error);
int fpi_ast_add_condition(FPI_AST* ast, int rule_index, int symbol_id, const FPI_ValueRef* value, FPI_Span span, FPI_Error* error);
int fpi_ast_add_action(FPI_AST* ast, int rule_index, int symbol_id, const FPI_ValueRef* value, FPI_Span span, FPI_Error* error);
int fpi_ast_pool_begin(const FPI_AST* ast, FPI_U32* mark);
int fpi_ast_pool_push(FPI_AST* ast, char c, FPI_Error* error, FPI_Span span);
int fpi_ast_pool_finish(FPI_AST* ast, FPI_U32 mark, FPI_ValueRef* out_ref, FPI_Error* error, FPI_Span span);
void fpi_ast_pool_rollback(FPI_AST* ast, FPI_U32 mark);
const char* fpi_ast_value_text(const FPI_AST* ast, const FPI_ValueRef* ref);
void fpi_ast_value_view(const FPI_AST* ast, const FPI_ValueRef* ref, FPI_Value* out_value);
const FPI_Term* fpi_ast_condition_at(const FPI_AST* ast, const FPI_Rule* rule, int index);
const FPI_Term* fpi_ast_action_at(const FPI_AST* ast, const FPI_Rule* rule, int index);
FPI_U32 fpi_ast_memory_used(const FPI_AST* ast);

/* Legacy helper names. */
void ast_init(AST_Script* ast);
AST_Rule* ast_add_rule(AST_Script* ast);
void ast_clear_rule(AST_Rule* rule);

#endif /* FPI_AST_H */
