#include "fpi_builtins.h"
#include "fpi_util.h"

static int append_suffix(char* out, int cap, const char* base, const char* suffix) {
    int i;
    int j;
    if (!out || cap <= 0 || !base || !suffix) return FPI_ERR_ARGUMENT;
    i = 0;
    while (base[i]) {
        if (i >= cap - 1) return FPI_ERR_IDENTIFIER_TOO_LONG;
        out[i] = fpi_ascii_tolower(base[i]);
        i++;
    }
    j = 0;
    while (suffix[j]) {
        if (i >= cap - 1) return FPI_ERR_IDENTIFIER_TOO_LONG;
        out[i++] = suffix[j++];
    }
    out[i] = '\0';
    return FPI_OK;
}

void fpi_state_words_default(FPI_StateWords* words) {
    if (!words) return;
    (void)fpi_str_copy(words->equal, FPI_IDENT_MAX + 1, "state");
    (void)fpi_str_copy(words->greater, FPI_IDENT_MAX + 1, "stategreater");
    (void)fpi_str_copy(words->lesser, FPI_IDENT_MAX + 1, "statelesser");
    (void)fpi_str_copy(words->set, FPI_IDENT_MAX + 1, "state");
    (void)fpi_str_copy(words->increment, FPI_IDENT_MAX + 1, "incstate");
}

int fpi_state_words_from_base(FPI_StateWords* words, const char* base_name) {
    int rc;
    if (!words || !base_name) return FPI_ERR_ARGUMENT;
    rc = append_suffix(words->equal, FPI_IDENT_MAX + 1, base_name, "");
    if (rc != FPI_OK) return rc;
    rc = append_suffix(words->greater, FPI_IDENT_MAX + 1, base_name, "greater");
    if (rc != FPI_OK) return rc;
    rc = append_suffix(words->lesser, FPI_IDENT_MAX + 1, base_name, "lesser");
    if (rc != FPI_OK) return rc;
    rc = append_suffix(words->set, FPI_IDENT_MAX + 1, base_name, "");
    if (rc != FPI_OK) return rc;
    return append_suffix(words->increment, FPI_IDENT_MAX + 1, "inc", base_name);
}

static int state_resolve(FPI_StateBuiltin* builtin, const FPI_Value* value, FPI_Fixed* out_value) {
    if (!builtin || !builtin->context) return 0;
    return fpi_resolve_value_fixed(builtin->context, value, out_value);
}

static int state_equal_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_StateBuiltin* builtin;
    FPI_Fixed rhs;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_StateBuiltin*)bind_user;
    if (!builtin || !builtin->state || !state_resolve(builtin, value, &rhs)) return 0;
    return *builtin->state == rhs;
}

static int state_greater_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_StateBuiltin* builtin;
    FPI_Fixed rhs;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_StateBuiltin*)bind_user;
    if (!builtin || !builtin->state || !state_resolve(builtin, value, &rhs)) return 0;
    return *builtin->state > rhs;
}

static int state_lesser_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_StateBuiltin* builtin;
    FPI_Fixed rhs;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_StateBuiltin*)bind_user;
    if (!builtin || !builtin->state || !state_resolve(builtin, value, &rhs)) return 0;
    return *builtin->state < rhs;
}

static void state_set_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_StateBuiltin* builtin;
    FPI_Fixed rhs;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_StateBuiltin*)bind_user;
    if (!builtin || !builtin->state || !state_resolve(builtin, value, &rhs)) return;
    if (builtin->single_transition_per_tick && builtin->transitioned && *builtin->transitioned) return;
    *builtin->state = rhs;
    if (builtin->transitioned) *builtin->transitioned = 1;
}

static void state_increment_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_StateBuiltin* builtin;
    FPI_Fixed rhs;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_StateBuiltin*)bind_user;
    if (!builtin || !builtin->state || !state_resolve(builtin, value, &rhs)) return;
    if (builtin->single_transition_per_tick && builtin->transitioned && *builtin->transitioned) return;
    *builtin->state = fpi_fixed_add_sat(*builtin->state, rhs);
    if (builtin->transitioned) *builtin->transitioned = 1;
}

int fpi_state_builtin_add_words(FPI_StateBuiltin* builtin, const FPI_StateWords* words) {
    int rc;
    if (!builtin || !builtin->context || !words) return FPI_ERR_ARGUMENT;
    rc = fpi_bind_cond(builtin->context, words->equal, state_equal_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_cond(builtin->context, words->greater, state_greater_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_cond(builtin->context, words->lesser, state_lesser_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_act(builtin->context, words->set, state_set_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_act(builtin->context, words->increment, state_increment_cb, builtin);
    if (rc < 0) return rc;
    return FPI_OK;
}

int fpi_state_builtin_init(FPI_StateBuiltin* builtin, FPI_Context* context, FPI_Fixed* state, int* transitioned, const FPI_StateWords* words) {
    FPI_StateWords defaults;
    if (!builtin || !context || !state) return FPI_ERR_ARGUMENT;
    builtin->context = context;
    builtin->state = state;
    builtin->transitioned = transitioned;
    builtin->single_transition_per_tick = 0;
    if (!words) {
        fpi_state_words_default(&defaults);
        words = &defaults;
    }
    return fpi_state_builtin_add_words(builtin, words);
}

int fpi_state_builtin_add_base(FPI_StateBuiltin* builtin, const char* base_name) {
    FPI_StateWords words;
    int rc;
    rc = fpi_state_words_from_base(&words, base_name);
    if (rc != FPI_OK) return rc;
    return fpi_state_builtin_add_words(builtin, &words);
}

void fpi_state_builtin_begin_tick(FPI_StateBuiltin* builtin) {
    if (builtin && builtin->transitioned) *builtin->transitioned = 0;
}

static FPI_U32 truth_next(FPI_TruthBuiltin* builtin) {
    builtin->random_state = builtin->random_state * 1664525UL + 1013904223UL;
    return builtin->random_state;
}

static int always_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(bind_user);
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    FPI_UNUSED(value);
    return 1;
}

static int never_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(bind_user);
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    FPI_UNUSED(value);
    return 0;
}

static int random_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_TruthBuiltin* builtin;
    FPI_Fixed threshold;
    FPI_U32 sample;
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    builtin = (FPI_TruthBuiltin*)bind_user;
    if (!builtin || !builtin->context || !fpi_resolve_value_fixed(builtin->context, value, &threshold)) return 0;
    if (threshold <= 0L) return 0;
    if (threshold >= 100L * FPI_FIXED_ONE) return 1;
    sample = truth_next(builtin) % 10000UL;
    return (FPI_Fixed)(sample * 65536UL / 100UL) < threshold;
}

static void none_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(bind_user);
    FPI_UNUSED(run_user);
    FPI_UNUSED(symbol_id);
    FPI_UNUSED(value);
}

int fpi_truth_builtin_init(FPI_TruthBuiltin* builtin, FPI_Context* context, FPI_U32 seed) {
    int rc;
    if (!builtin || !context) return FPI_ERR_ARGUMENT;
    builtin->context = context;
    builtin->random_state = seed ? seed : 1UL;
    rc = fpi_bind_cond(context, "always", always_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_cond(context, "never", never_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_cond(context, "random", random_cb, builtin);
    if (rc < 0) return rc;
    rc = fpi_bind_act(context, "none", none_cb, builtin);
    if (rc < 0) return rc;
    return FPI_OK;
}

void fpi_truth_builtin_seed(FPI_TruthBuiltin* builtin, FPI_U32 seed) {
    if (builtin) builtin->random_state = seed ? seed : 1UL;
}
