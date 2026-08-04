#ifndef FPI_BUILTINS_H
#define FPI_BUILTINS_H

#include "fpi.h"

typedef struct FPI_StateWords {
    char equal[FPI_IDENT_MAX + 1];
    char greater[FPI_IDENT_MAX + 1];
    char lesser[FPI_IDENT_MAX + 1];
    char set[FPI_IDENT_MAX + 1];
    char increment[FPI_IDENT_MAX + 1];
} FPI_StateWords;

typedef struct FPI_StateBuiltin {
    FPI_Context* context;
    FPI_Fixed* state;
    int* transitioned;
    int single_transition_per_tick;
} FPI_StateBuiltin;

typedef struct FPI_TruthBuiltin {
    FPI_Context* context;
    FPI_U32 random_state;
} FPI_TruthBuiltin;

void fpi_state_words_default(FPI_StateWords* words);
int fpi_state_words_from_base(FPI_StateWords* words, const char* base_name);
int fpi_state_builtin_init(FPI_StateBuiltin* builtin, FPI_Context* context, FPI_Fixed* state, int* transitioned, const FPI_StateWords* words);
int fpi_state_builtin_add_words(FPI_StateBuiltin* builtin, const FPI_StateWords* words);
int fpi_state_builtin_add_base(FPI_StateBuiltin* builtin, const char* base_name);
void fpi_state_builtin_begin_tick(FPI_StateBuiltin* builtin);

int fpi_truth_builtin_init(FPI_TruthBuiltin* builtin, FPI_Context* context, FPI_U32 seed);
void fpi_truth_builtin_seed(FPI_TruthBuiltin* builtin, FPI_U32 seed);

#endif /* FPI_BUILTINS_H */
