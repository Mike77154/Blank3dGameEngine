#include "var_system_fpi.h"
#include "fpi_util.h"

#define VAR_TOKEN_MAX 160

static FPI_NamedVar* named_pool(FPI_VarSystem* vars, int scope) {
    return scope ? vars->l_named : vars->g_named;
}

static FPI_Fixed* indexed_ptr(FPI_VarSystem* vars, int scope, int index) {
    if (!vars || index < 0 || index >= FPI_STD_VARS) return 0;
    return scope ? &vars->lvars[index] : &vars->gvars[index];
}

static int named_find(FPI_NamedVar* pool, const char* name) {
    int i;
    if (!pool || !name || !*name) return -1;
    for (i = 0; i < FPI_MAX_NAMED_VARS; i++) {
        if (pool[i].used && fpi_str_ieq(pool[i].name, name)) return i;
    }
    return -1;
}

static int named_ensure(FPI_NamedVar* pool, const char* name) {
    int index;
    int i;
    index = named_find(pool, name);
    if (index >= 0) return index;
    for (i = 0; i < FPI_MAX_NAMED_VARS; i++) {
        if (!pool[i].used) {
            if (fpi_str_copy_lower(pool[i].name, FPI_VAR_NAME_MAX, name) < 0) return -1;
            pool[i].used = 1;
            pool[i].value = 0L;
            return i;
        }
    }
    return -1;
}

static const char* strip_percent(const char* name) {
    if (name && name[0] == '%') return name + 1;
    return name;
}

static int parse_index(const char* token, int* out_index) {
    FPI_Fixed fixed;
    long integer;
    if (!token || !out_index) return 0;
    if (fpi_fixed_parse(token, &fixed) != FPI_OK) return 0;
    if ((fixed & (FPI_FIXED_ONE - 1L)) != 0L) return 0;
    integer = fpi_fixed_to_int(fixed);
    if (integer < 0L || integer >= FPI_STD_VARS) return 0;
    *out_index = (int)integer;
    return 1;
}

static int find_named_any(FPI_VarSystem* vars, const char* name, FPI_VarRef* out_ref) {
    int index;
    index = named_find(vars->l_named, name);
    if (index >= 0) {
        out_ref->is_named = 1;
        out_ref->scope = 1;
        out_ref->index = index;
        return 1;
    }
    index = named_find(vars->g_named, name);
    if (index >= 0) {
        out_ref->is_named = 1;
        out_ref->scope = 0;
        out_ref->index = index;
        return 1;
    }
    return 0;
}

static int resolve_varref(FPI_VarSystem* vars, const char* token, int requested_scope, int create, FPI_VarRef* out_ref) {
    int index;
    int scope;
    const char* name;
    FPI_NamedVar* pool;
    if (!vars || !token || !*token || !out_ref) return 0;
    scope = requested_scope < 0 ? vars->current.scope : requested_scope;
    if (parse_index(token, &index)) {
        out_ref->is_named = 0;
        out_ref->scope = scope;
        out_ref->index = index;
        return 1;
    }
    name = strip_percent(token);
    if (!*name) return 0;
    if (requested_scope < 0 && find_named_any(vars, name, out_ref)) return 1;
    pool = named_pool(vars, scope);
    index = named_find(pool, name);
    if (index < 0 && create) index = named_ensure(pool, name);
    if (index < 0) return 0;
    out_ref->is_named = 1;
    out_ref->scope = scope;
    out_ref->index = index;
    return 1;
}

static FPI_Fixed* varref_ptr(FPI_VarSystem* vars, const FPI_VarRef* ref) {
    FPI_NamedVar* pool;
    if (!vars || !ref) return 0;
    if (!ref->is_named) return indexed_ptr(vars, ref->scope, ref->index);
    if (ref->index < 0 || ref->index >= FPI_MAX_NAMED_VARS) return 0;
    pool = named_pool(vars, ref->scope);
    if (!pool[ref->index].used) return 0;
    return &pool[ref->index].value;
}

static int split_value(const FPI_Value* value, char* first, char* second) {
    if (!value || !value->has) return 0;
    return fpi_split_2tokens_ws(value->s, first, VAR_TOKEN_MAX, second, VAR_TOKEN_MAX);
}

static int resolve_target_rhs(FPI_VarSystem* vars, const FPI_Value* value, FPI_VarRef* target, FPI_Fixed* rhs) {
    char first[VAR_TOKEN_MAX];
    char second[VAR_TOKEN_MAX];
    if (!split_value(value, first, second)) return 0;
    if (!second[0]) {
        *target = vars->current;
        return fpi_resolve_fixed_token(&vars->resolver, first, rhs);
    }
    if (!resolve_varref(vars, first, -1, 0, target)) return 0;
    return fpi_resolve_fixed_token(&vars->resolver, second, rhs);
}

int fpi_eval_var_condition(FPI_VarSystem* vars, int condition_id, const FPI_Value* value) {
    FPI_VarRef target;
    FPI_Fixed rhs;
    FPI_Fixed* variable;
    if (!vars) return -1;
    if (condition_id != vars->ID_VAREQUAL && condition_id != vars->ID_VARNOTEQUAL &&
        condition_id != vars->ID_VARGREATER && condition_id != vars->ID_VARLESS) return -1;
    if (!resolve_target_rhs(vars, value, &target, &rhs)) return 0;
    variable = varref_ptr(vars, &target);
    if (!variable) return 0;
    if (condition_id == vars->ID_VAREQUAL) return *variable == rhs;
    if (condition_id == vars->ID_VARNOTEQUAL) return *variable != rhs;
    if (condition_id == vars->ID_VARGREATER) return *variable > rhs;
    return *variable < rhs;
}

static int selector_scope(const FPI_VarSystem* vars, int action_id, int* out_scope) {
    if (action_id == vars->ID_GLOBALVAR || action_id == vars->ID_GLOBVAR) { *out_scope = 0; return 1; }
    if (action_id == vars->ID_LOCALVAR || action_id == vars->ID_LOCVAR) { *out_scope = 1; return 1; }
    return 0;
}

static int apply_math(FPI_VarSystem* vars, int action_id, const FPI_Value* value) {
    FPI_VarRef target;
    FPI_Fixed rhs;
    FPI_Fixed* variable;
    int ok;
    FPI_Fixed mod;
    if (!resolve_target_rhs(vars, value, &target, &rhs)) return 0;
    variable = varref_ptr(vars, &target);
    if (!variable) return 0;
    if (action_id == vars->ID_ADDVAR) *variable = fpi_fixed_add_sat(*variable, rhs);
    else if (action_id == vars->ID_SUBVAR) *variable = fpi_fixed_sub_sat(*variable, rhs);
    else if (action_id == vars->ID_MULVAR) *variable = fpi_fixed_mul_sat(*variable, rhs);
    else if (action_id == vars->ID_DIVVAR) {
        *variable = fpi_fixed_div_sat(*variable, rhs, &ok);
        if (!ok) return 0;
    } else if (action_id == vars->ID_MODVAR) {
        *variable = fpi_fixed_mod(*variable, rhs, &ok);
        if (!ok) return 0;
    } else if (action_id == vars->ID_WRAPVAR) {
        mod = fpi_fixed_mod(*variable, rhs, &ok);
        if (!ok) return 0;
        if (mod < 0L) mod = fpi_fixed_add_sat(mod, rhs < 0L ? -rhs : rhs);
        *variable = mod;
    } else return 0;
    vars->current = target;
    return 1;
}

static FPI_U32 var_next_random(FPI_VarSystem* vars) {
    vars->random_state = vars->random_state * 1664525UL + 1013904223UL;
    return vars->random_state;
}

int fpi_exec_var_action(FPI_VarSystem* vars, int action_id, const FPI_Value* value) {
    int scope;
    char first[VAR_TOKEN_MAX];
    char second[VAR_TOKEN_MAX];
    FPI_VarRef target;
    FPI_Fixed* variable;
    FPI_Fixed rhs;
    int max_value;
    int flag;
    if (!vars) return 0;
    if (selector_scope(vars, action_id, &scope)) {
        if (!split_value(value, first, second) || second[0]) return 0;
        if (!resolve_varref(vars, first, scope, 1, &target)) return 0;
        target.scope = scope;
        vars->current = target;
        return 1;
    }
    if (action_id == vars->ID_DIMVAR || action_id == vars->ID_DIMLOCALVAR) {
        scope = action_id == vars->ID_DIMLOCALVAR ? 1 : 0;
        if (!split_value(value, first, second) || second[0]) return 0;
        if (!resolve_varref(vars, first, scope, 1, &target) || !target.is_named) return 0;
        vars->current = target;
        return 1;
    }
    if (action_id == vars->ID_RESETGLOBALSONRELOAD) {
        flag = 0;
        if (value && value->has && !fpi_resolve_int_value(&vars->resolver, value, &flag)) return 0;
        vars->reset_globals_on_reload = flag != 0;
        return 1;
    }
    if (action_id == vars->ID_SETVAR) {
        if (!value || !value->has) {
            variable = varref_ptr(vars, &vars->current);
            if (!variable) return 0;
            *variable = 0L;
            return 1;
        }
        if (!split_value(value, first, second)) return 0;
        if (!second[0]) {
            if (fpi_resolve_fixed_token(&vars->resolver, first, &rhs)) {
                variable = varref_ptr(vars, &vars->current);
                if (!variable) return 0;
                *variable = rhs;
                return 1;
            }
            if (!resolve_varref(vars, first, -1, 1, &target) || !target.is_named) return 0;
            vars->current = target;
            return 1;
        }
        if (!resolve_varref(vars, first, -1, 1, &target)) return 0;
        if (!fpi_resolve_fixed_token(&vars->resolver, second, &rhs)) return 0;
        variable = varref_ptr(vars, &target);
        if (!variable) return 0;
        *variable = rhs;
        vars->current = target;
        return 1;
    }
    if (action_id == vars->ID_INCVAR || action_id == vars->ID_DECVAR) {
        rhs = FPI_FIXED_ONE;
        if (value && value->has && !fpi_resolve_fixed_value(&vars->resolver, value, &rhs)) return 0;
        variable = varref_ptr(vars, &vars->current);
        if (!variable) return 0;
        if (action_id == vars->ID_INCVAR) *variable = fpi_fixed_add_sat(*variable, rhs);
        else *variable = fpi_fixed_sub_sat(*variable, rhs);
        return 1;
    }
    if (action_id == vars->ID_ADDVAR || action_id == vars->ID_SUBVAR ||
        action_id == vars->ID_MULVAR || action_id == vars->ID_DIVVAR ||
        action_id == vars->ID_MODVAR || action_id == vars->ID_WRAPVAR)
        return apply_math(vars, action_id, value);
    if (action_id == vars->ID_SETVARRND) {
        if (!resolve_target_rhs(vars, value, &target, &rhs)) return 0;
        max_value = (int)fpi_fixed_to_int(rhs);
        if (max_value < 0) max_value = 0;
        variable = varref_ptr(vars, &target);
        if (!variable) return 0;
        *variable = fpi_fixed_from_int((long)(var_next_random(vars) % (FPI_U32)(max_value + 1)));
        vars->current = target;
        return 1;
    }
    if (action_id == vars->ID_SIN || action_id == vars->ID_COS) {
        if (!resolve_target_rhs(vars, value, &target, &rhs)) return 0;
        variable = varref_ptr(vars, &target);
        if (!variable) return 0;
        *variable = action_id == vars->ID_SIN ? fpi_fixed_sin_deg(rhs) : fpi_fixed_cos_deg(rhs);
        vars->current = target;
        return 1;
    }
    return 0;
}

static int var_condition_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    return fpi_eval_var_condition((FPI_VarSystem*)bind_user, symbol_id, value);
}

static void var_action_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    (void)fpi_exec_var_action((FPI_VarSystem*)bind_user, symbol_id, value);
}

static int bind_condition(FPI_VarSystem* vars, const char* name) {
    return fpi_bind_cond(vars->context, name, var_condition_cb, vars);
}

static int bind_action(FPI_VarSystem* vars, const char* name) {
    return fpi_bind_act(vars->context, name, var_action_cb, vars);
}

void fpi_var_system_init(FPI_VarSystem* vars, FPI_Context* context) {
    fpi_var_system_init2(vars, context, 0, 0);
}

void fpi_var_system_init2(FPI_VarSystem* vars, FPI_Context* context, FPI_InternalVarFn internal_fn, void* internal_user) {
    int i;
    if (!vars) return;
    vars->context = context;
    for (i = 0; i < FPI_STD_VARS; i++) { vars->gvars[i] = 0L; vars->lvars[i] = 0L; }
    for (i = 0; i < FPI_MAX_NAMED_VARS; i++) {
        vars->g_named[i].used = 0; vars->g_named[i].name[0] = '\0'; vars->g_named[i].value = 0L;
        vars->l_named[i].used = 0; vars->l_named[i].name[0] = '\0'; vars->l_named[i].value = 0L;
    }
    vars->current.is_named = 0;
    vars->current.scope = 0;
    vars->current.index = 0;
    vars->reset_globals_on_reload = 0;
    vars->random_state = 1UL;
    fpi_value_resolver_init(&vars->resolver, context, vars, internal_fn, internal_user);
    if (!context) return;
    vars->ID_VAREQUAL = bind_condition(vars, "varequal");
    vars->ID_VARNOTEQUAL = bind_condition(vars, "varnotequal");
    vars->ID_VARGREATER = bind_condition(vars, "vargreater");
    vars->ID_VARLESS = bind_condition(vars, "varless");
    vars->ID_GLOBALVAR = bind_action(vars, "globalvar");
    vars->ID_GLOBVAR = bind_action(vars, "globvar");
    vars->ID_LOCALVAR = bind_action(vars, "localvar");
    vars->ID_LOCVAR = bind_action(vars, "locvar");
    vars->ID_SETVAR = bind_action(vars, "setvar");
    vars->ID_INCVAR = bind_action(vars, "incvar");
    vars->ID_DECVAR = bind_action(vars, "decvar");
    vars->ID_ADDVAR = bind_action(vars, "addvar");
    vars->ID_SUBVAR = bind_action(vars, "subvar");
    vars->ID_MULVAR = bind_action(vars, "mulvar");
    vars->ID_DIVVAR = bind_action(vars, "divvar");
    vars->ID_MODVAR = bind_action(vars, "modvar");
    vars->ID_WRAPVAR = bind_action(vars, "wrapvar");
    vars->ID_SETVARRND = bind_action(vars, "setvarrnd");
    vars->ID_SIN = bind_action(vars, "sin");
    vars->ID_COS = bind_action(vars, "cos");
    vars->ID_DIMVAR = bind_action(vars, "dimvar");
    vars->ID_DIMLOCALVAR = bind_action(vars, "dimlocalvar");
    vars->ID_RESETGLOBALSONRELOAD = bind_action(vars, "resetglobalsonreload");
}

void fpi_var_system_reset_globals(FPI_VarSystem* vars) {
    int i;
    if (!vars) return;
    for (i = 0; i < FPI_STD_VARS; i++) vars->gvars[i] = 0L;
    for (i = 0; i < FPI_MAX_NAMED_VARS; i++) if (vars->g_named[i].used) vars->g_named[i].value = 0L;
}

int fpi_var_try_get(FPI_VarSystem* vars, const char* name, FPI_Fixed* out_value) {
    int index;
    const char* normalized;
    if (!vars || !name || !out_value) return 0;
    if (parse_index(name, &index)) { *out_value = vars->gvars[index]; return 1; }
    normalized = strip_percent(name);
    index = named_find(vars->l_named, normalized);
    if (index >= 0) { *out_value = vars->l_named[index].value; return 1; }
    index = named_find(vars->g_named, normalized);
    if (index >= 0) { *out_value = vars->g_named[index].value; return 1; }
    return 0;
}

int fpi_var_try_set(FPI_VarSystem* vars, const char* name, FPI_Fixed value) {
    int index;
    const char* normalized;
    if (!vars || !name) return 0;
    if (parse_index(name, &index)) { vars->gvars[index] = value; return 1; }
    normalized = strip_percent(name);
    index = named_find(vars->l_named, normalized);
    if (index >= 0) { vars->l_named[index].value = value; return 1; }
    index = named_find(vars->g_named, normalized);
    if (index < 0) index = named_ensure(vars->g_named, normalized);
    if (index < 0) return 0;
    vars->g_named[index].value = value;
    return 1;
}

void fpi_var_seed(FPI_VarSystem* vars, FPI_U32 seed) {
    if (vars) vars->random_state = seed ? seed : 1UL;
}
