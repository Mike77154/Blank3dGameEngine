#ifndef CONDITIONAL_EVAL_ADVERBS_FPI_H
#define CONDITIONAL_EVAL_ADVERBS_FPI_H

#include "value_resolver_fpi.h"

typedef struct FPI_CondAdverbs {
    FPI_Context* context;
    int ID_ALWAYS;
    int ID_NEVER;
    int ID_RANDOM;
    FPI_U32 random_state;
    FPI_ValueResolver resolver;
} FPI_CondAdverbs;

int fpi_cond_adverbs_init(FPI_CondAdverbs* adverbs, FPI_Context* context);
int fpi_cond_adverbs_init2(FPI_CondAdverbs* adverbs, FPI_Context* context,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user);
int fpi_eval_cond_adverbs(FPI_CondAdverbs* adverbs, int condition_id,
    const FPI_Value* value);
void fpi_cond_adverbs_seed(FPI_CondAdverbs* adverbs, FPI_U32 seed);

#endif /* CONDITIONAL_EVAL_ADVERBS_FPI_H */
