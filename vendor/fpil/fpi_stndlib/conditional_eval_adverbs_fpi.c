#include "conditional_eval_adverbs_fpi.h"

static FPI_U32 adverb_next(FPI_CondAdverbs* adverbs) {
    adverbs->random_state = adverbs->random_state * 1664525UL + 1013904223UL;
    return adverbs->random_state;
}

static int adverb_condition_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    return fpi_eval_cond_adverbs((FPI_CondAdverbs*)bind_user, symbol_id, value);
}

int fpi_cond_adverbs_init(FPI_CondAdverbs* adverbs, FPI_Context* context) {
    return fpi_cond_adverbs_init2(adverbs, context, 0, 0, 0);
}

int fpi_cond_adverbs_init2(FPI_CondAdverbs* adverbs, FPI_Context* context,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user) {
    int rc;
    if (!adverbs || !context) return FPI_ERR_ARGUMENT;
    adverbs->context = context;
    adverbs->random_state = 1UL;
    fpi_value_resolver_init(&adverbs->resolver, context, vars, internal_fn, internal_user);
    rc = fpi_bind_cond(context, "always", adverb_condition_cb, adverbs);
    if (rc < 0) return rc;
    adverbs->ID_ALWAYS = rc;
    rc = fpi_bind_cond(context, "never", adverb_condition_cb, adverbs);
    if (rc < 0) return rc;
    adverbs->ID_NEVER = rc;
    rc = fpi_bind_cond(context, "random", adverb_condition_cb, adverbs);
    if (rc < 0) return rc;
    adverbs->ID_RANDOM = rc;
    return FPI_OK;
}

int fpi_eval_cond_adverbs(FPI_CondAdverbs* adverbs, int condition_id,
    const FPI_Value* value) {
    FPI_Fixed chance;
    FPI_U32 sample;
    if (!adverbs) return -1;
    if (condition_id == adverbs->ID_ALWAYS) return 1;
    if (condition_id == adverbs->ID_NEVER) return 0;
    if (condition_id != adverbs->ID_RANDOM) return -1;
    if (!fpi_resolve_fixed_value(&adverbs->resolver, value, &chance)) return 0;
    if (chance <= 0L) return 0;
    if (chance >= 100L * FPI_FIXED_ONE) return 1;
    sample = adverb_next(adverbs) % 10000UL;
    return (FPI_Fixed)(sample * 65536UL / 100UL) < chance;
}

void fpi_cond_adverbs_seed(FPI_CondAdverbs* adverbs, FPI_U32 seed) {
    if (adverbs) adverbs->random_state = seed ? seed : 1UL;
}
