#ifndef FPI_IR_H
#define FPI_IR_H

#include "fpi_program.h"

typedef struct FPI_IRTerm {
    int name_space;
    int symbol_id;
    FPI_Value value;
} FPI_IRTerm;

typedef struct FPI_IRRule {
    int condition_first;
    int condition_count;
    int action_first;
    int action_count;
} FPI_IRRule;

typedef struct FPI_IR {
    FPI_IRRule* rules;
    int rule_count;
    FPI_IRTerm* terms;
    int term_count;
} FPI_IR;

void fpi_ir_init(FPI_IR* ir);
int fpi_ir_build(const FPI_AST* ast, FPI_Arena* arena, FPI_IR* out_ir, FPI_Error* error);

#endif /* FPI_IR_H */
