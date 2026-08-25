#include "blank3d_variables.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static void b3d_vars_status(Blank3DVariables *vars, const char *text)
{
    if (!vars) return;
    if (!text) text = "";
    strncpy(vars->status, text, sizeof(vars->status) - 1U);
    vars->status[sizeof(vars->status) - 1U] = '\0';
}

static long b3d_sat_add_long(long a, long b)
{
    if (b > 0L && a > LONG_MAX - b) return LONG_MAX;
    if (b < 0L && a < LONG_MIN - b) return LONG_MIN;
    return a + b;
}

static long b3d_q10_to_q16(long q10)
{
    if (q10 > LONG_MAX / 64L) return LONG_MAX;
    if (q10 < LONG_MIN / 64L) return LONG_MIN;
    return q10 * 64L;
}

static long b3d_q16_to_q10(long q16)
{
    return q16 / 64L;
}

static const char *b3d_num_alias(const Blank3DVariables *vars,
                                 unsigned long owner, const char *name)
{
    if (!vars || !name) return name;
    if (owner == vars->player_owner && vars->player_owner != 0UL) {
        if (strcmp(name, "health") == 0) return "player.health";
        if (strcmp(name, "damage_multiplier") == 0)
            return "weapon.damage_multiplier";
        if (strcmp(name, "speed_multiplier") == 0)
            return "weapon.speed_multiplier";
        if (strcmp(name, "recoil_multiplier") == 0)
            return "weapon.recoil_multiplier";
    }
    return name;
}

static const char *b3d_flag_alias(const Blank3DVariables *vars,
                                  unsigned long owner, const char *name)
{
    if (!vars || !name) return name;
    if (owner == vars->player_owner && vars->player_owner != 0UL) {
        if (strcmp(name, "can_fire") == 0) return "weapon.can_fire";
        if (strcmp(name, "can_reload") == 0) return "weapon.can_reload";
        if (strcmp(name, "weapon_enabled") == 0) return "weapon.enabled";
        if (strcmp(name, "active_reload") == 0)
            return "weapon.active_reload";
    }
    return name;
}

static int b3d_num_owner(vr89_scope scope, vr89_owner owner, ns_owner *out)
{
    if (!out) return 0;
    if (scope == VR89_SCOPE_GLOBAL) {
        *out = NS_OWNER_GLOBAL;
        return 1;
    }
    if (owner > (unsigned long)INT_MAX) return 0;
    *out = (ns_owner)owner;
    return 1;
}

static ns_id b3d_num_value(Blank3DVariables *vars,
                           vr89_scope scope, vr89_owner owner,
                           const char *name, int attach)
{
    const char *actual;
    ns_owner num_owner;
    ns_id type_id;
    ns_id value_id;
    if (!vars || !vars->systems || !name) return NS_INVALID_ID;
    actual = b3d_num_alias(vars, owner, name);
    type_id = ns_find_type(&vars->systems->numbers, actual);
    if (type_id < 0) return NS_INVALID_ID;
    if (!b3d_num_owner(scope, owner, &num_owner)) return NS_INVALID_ID;
    value_id = ns_find_value_by_type(&vars->systems->numbers,
                                     num_owner, type_id);
    if (value_id < 0 && attach)
        value_id = ns_attach_type(&vars->systems->numbers,
                                  num_owner, type_id);
    return value_id;
}

static int b3d_num_claim(void *user, vr89_scope scope, vr89_owner owner,
                         const char *name, vr89_operation operation,
                         const vm89_value *value)
{
    Blank3DVariables *vars;
    const char *actual;
    (void)operation;
    vars = (Blank3DVariables *)user;
    if (!vars || !vars->systems || !name || scope == VR89_SCOPE_LOCAL)
        return 0;
    if (value && value->type == VM89_VALUE_STRING) return 0;
    actual = b3d_num_alias(vars, owner, name);
    return ns_find_type(&vars->systems->numbers, actual) >= 0;
}

static int b3d_num_get(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, vm89_value *out_value)
{
    Blank3DVariables *vars;
    ns_id value_id;
    ns_fx value;
    ns_id type_id;
    const char *actual;
    vars = (Blank3DVariables *)user;
    if (!vars || !out_value) return 0;
    value_id = b3d_num_value(vars, scope, owner, name, 1);
    if (value_id < 0) return 0;
    if (ns_get_by_id(&vars->systems->numbers, value_id, &value) != NS_OK)
        return 0;
    actual = b3d_num_alias(vars, owner, name);
    type_id = ns_find_type(&vars->systems->numbers, actual);
    if (type_id < 0) return 0;
    if (vars->systems->numbers.types[type_id].kind == NS_KIND_BOOL)
        vm89_value_bool(out_value, value != NS_FX_ZERO);
    else
        vm89_value_fixed_raw(out_value, b3d_q10_to_q16(value));
    return 1;
}

static int b3d_num_value_to_fx(const vm89_value *value, ns_fx *out)
{
    if (!value || !out) return 0;
    if (value->type == VM89_VALUE_FIXED) {
        *out = (ns_fx)b3d_q16_to_q10(value->fixed_q16);
        return 1;
    }
    if (value->type == VM89_VALUE_BOOL) {
        *out = value->boolean ? NS_FX_ONE : NS_FX_ZERO;
        return 1;
    }
    return 0;
}

static int b3d_num_set(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, const vm89_value *value)
{
    Blank3DVariables *vars;
    ns_id value_id;
    ns_fx numeric;
    vars = (Blank3DVariables *)user;
    if (!vars || !b3d_num_value_to_fx(value, &numeric)) return 0;
    value_id = b3d_num_value(vars, scope, owner, name, 1);
    return value_id >= 0 &&
           ns_set_by_id(&vars->systems->numbers, value_id, numeric) == NS_OK;
}

static int b3d_num_add(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, const vm89_value *value)
{
    Blank3DVariables *vars;
    ns_id value_id;
    ns_fx numeric;
    vars = (Blank3DVariables *)user;
    if (!vars || !b3d_num_value_to_fx(value, &numeric)) return 0;
    value_id = b3d_num_value(vars, scope, owner, name, 1);
    return value_id >= 0 &&
           ns_add_by_id(&vars->systems->numbers, value_id, numeric) == NS_OK;
}

static int b3d_num_sub(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, const vm89_value *value)
{
    Blank3DVariables *vars;
    ns_id value_id;
    ns_fx numeric;
    vars = (Blank3DVariables *)user;
    if (!vars || !b3d_num_value_to_fx(value, &numeric)) return 0;
    value_id = b3d_num_value(vars, scope, owner, name, 1);
    return value_id >= 0 &&
           ns_sub_by_id(&vars->systems->numbers, value_id, numeric) == NS_OK;
}

static int b3d_flag_instance_key(vr89_owner owner, const char *name,
                                 char *out, unsigned int out_cap)
{
    int n;
    if (!name || !out || out_cap < 2U) return 0;
    n = sprintf(out, "thing.%lu.%s", owner, name);
    return n > 0 && (unsigned int)n < out_cap;
}

static int b3d_flag_resolve_key(Blank3DVariables *vars,
                                vr89_scope scope, vr89_owner owner,
                                const char *name, vr89_operation operation,
                                const vm89_value *value,
                                char *out, unsigned int out_cap)
{
    const char *actual;
    FlagsValue existing;
    char instance_key[B3D_VAR_FLAG_KEY_CAP];
    if (!vars || !vars->systems || !name || !out) return 0;
    actual = b3d_flag_alias(vars, owner, name);
    if (flagstore_get(&vars->systems->flags, actual, &existing)) {
        if (strlen(actual) + 1U > out_cap) return 0;
        strcpy(out, actual);
        return 1;
    }
    if (scope == VR89_SCOPE_INSTANCE &&
        b3d_flag_instance_key(owner, actual, instance_key,
                              sizeof(instance_key)) &&
        flagstore_get(&vars->systems->flags, instance_key, &existing)) {
        if (strlen(instance_key) + 1U > out_cap) return 0;
        strcpy(out, instance_key);
        return 1;
    }
    if (operation == VR89_OP_SET && value &&
        value->type == VM89_VALUE_BOOL) {
        if (scope == VR89_SCOPE_GLOBAL) {
            if (strlen(actual) + 1U > out_cap) return 0;
            strcpy(out, actual);
            return 1;
        }
        if (scope == VR89_SCOPE_INSTANCE &&
            b3d_flag_instance_key(owner, actual, out, out_cap))
            return 1;
    }
    return 0;
}

static int b3d_flag_claim(void *user, vr89_scope scope, vr89_owner owner,
                          const char *name, vr89_operation operation,
                          const vm89_value *value)
{
    Blank3DVariables *vars;
    char key[B3D_VAR_FLAG_KEY_CAP];
    vars = (Blank3DVariables *)user;
    if (!vars || scope == VR89_SCOPE_LOCAL) return 0;
    return b3d_flag_resolve_key(vars, scope, owner, name,
                                operation, value, key, sizeof(key));
}

static int b3d_flag_get(void *user, vr89_scope scope, vr89_owner owner,
                        const char *name, vm89_value *out_value)
{
    Blank3DVariables *vars;
    char key[B3D_VAR_FLAG_KEY_CAP];
    FlagsValue value;
    vars = (Blank3DVariables *)user;
    if (!vars || !out_value) return 0;
    if (!b3d_flag_resolve_key(vars, scope, owner, name, VR89_OP_READ,
                              (const vm89_value *)0, key, sizeof(key)))
        return 0;
    if (!flagstore_get(&vars->systems->flags, key, &value)) return 0;
    if (value.type == FLAGS_VAL_BOOL)
        vm89_value_bool(out_value, value.as.i != 0L);
    else if (value.type == FLAGS_VAL_INT) {
        if (value.as.i > LONG_MAX / 65536L)
            vm89_value_fixed_raw(out_value, LONG_MAX);
        else if (value.as.i < LONG_MIN / 65536L)
            vm89_value_fixed_raw(out_value, LONG_MIN);
        else
            vm89_value_fixed_raw(out_value, value.as.i * 65536L);
    } else if (value.type == FLAGS_VAL_FX)
        vm89_value_fixed_raw(out_value, value.as.fx);
    else if (value.type == FLAGS_VAL_STR)
        return vm89_value_string(out_value, value.as.s ? value.as.s : "") == VM89_OK;
    else
        return 0;
    return 1;
}

static int b3d_flag_set(void *user, vr89_scope scope, vr89_owner owner,
                        const char *name, const vm89_value *value)
{
    Blank3DVariables *vars;
    char key[B3D_VAR_FLAG_KEY_CAP];
    FlagsValue existing;
    vars = (Blank3DVariables *)user;
    if (!vars || !value) return 0;
    if (!b3d_flag_resolve_key(vars, scope, owner, name, VR89_OP_SET,
                              value, key, sizeof(key))) return 0;
    if (flagstore_get(&vars->systems->flags, key, &existing)) {
        if (existing.type == FLAGS_VAL_BOOL)
            return flagstore_set_bool(&vars->systems->flags, key,
                value->type == VM89_VALUE_BOOL ? value->boolean :
                (value->type == VM89_VALUE_FIXED && value->fixed_q16 != 0L));
        if (existing.type == FLAGS_VAL_INT && value->type == VM89_VALUE_FIXED)
            return flagstore_set_int(&vars->systems->flags, key,
                                     value->fixed_q16 / 65536L);
        if (existing.type == FLAGS_VAL_FX && value->type == VM89_VALUE_FIXED)
            return flagstore_set_fx(&vars->systems->flags, key,
                                    (flags_fx_t)value->fixed_q16);
        if (existing.type == FLAGS_VAL_STR && value->type == VM89_VALUE_STRING)
            return flagstore_set_str(&vars->systems->flags, key,
                                     value->string_value);
        return 0;
    }
    if (value->type == VM89_VALUE_BOOL)
        return flagstore_set_bool(&vars->systems->flags, key, value->boolean);
    return 0;
}

static int b3d_flag_delta(void *user, vr89_scope scope, vr89_owner owner,
                          const char *name, const vm89_value *value,
                          int subtract)
{
    Blank3DVariables *vars;
    char key[B3D_VAR_FLAG_KEY_CAP];
    FlagsValue existing;
    long delta;
    vars = (Blank3DVariables *)user;
    if (!vars || !value || value->type != VM89_VALUE_FIXED) return 0;
    if (!b3d_flag_resolve_key(vars, scope, owner, name,
                              subtract ? VR89_OP_SUB : VR89_OP_ADD,
                              value, key, sizeof(key))) return 0;
    if (!flagstore_get(&vars->systems->flags, key, &existing)) return 0;
    delta = subtract ? -value->fixed_q16 : value->fixed_q16;
    if (existing.type == FLAGS_VAL_FX)
        return flagstore_set_fx(&vars->systems->flags, key,
            (flags_fx_t)b3d_sat_add_long(existing.as.fx, delta));
    if (existing.type == FLAGS_VAL_INT) {
        long whole_delta;
        whole_delta = delta / 65536L;
        return flagstore_set_int(&vars->systems->flags, key,
            b3d_sat_add_long(existing.as.i, whole_delta));
    }
    return 0;
}

static int b3d_flag_add(void *user, vr89_scope scope, vr89_owner owner,
                        const char *name, const vm89_value *value)
{
    return b3d_flag_delta(user, scope, owner, name, value, 0);
}

static int b3d_flag_sub(void *user, vr89_scope scope, vr89_owner owner,
                        const char *name, const vm89_value *value)
{
    return b3d_flag_delta(user, scope, owner, name, value, 1);
}

static int b3d_flag_unset(void *user, vr89_scope scope, vr89_owner owner,
                          const char *name)
{
    (void)user;
    (void)scope;
    (void)owner;
    (void)name;
    return 0;
}

void blank3d_variables_init(Blank3DVariables *vars, Blank3DSystems *systems)
{
    vr89_provider provider;
    if (!vars) return;
    memset(vars, 0, sizeof(*vars));
    vars->systems = systems;
    vr89_init(&vars->runtime);

    memset(&provider, 0, sizeof(provider));
    provider.name = "numsys89";
    provider.priority = 200;
    provider.user = vars;
    provider.claim = b3d_num_claim;
    provider.get = b3d_num_get;
    provider.set = b3d_num_set;
    provider.add = b3d_num_add;
    provider.sub = b3d_num_sub;
    (void)vr89_add_provider(&vars->runtime, &provider);

    memset(&provider, 0, sizeof(provider));
    provider.name = "flags89";
    provider.priority = 100;
    provider.user = vars;
    provider.claim = b3d_flag_claim;
    provider.get = b3d_flag_get;
    provider.set = b3d_flag_set;
    provider.add = b3d_flag_add;
    provider.sub = b3d_flag_sub;
    provider.unset = b3d_flag_unset;
    (void)vr89_add_provider(&vars->runtime, &provider);
    b3d_vars_status(vars, "VarDSL -> NumSys/Flags/VarStore ready");
}

void blank3d_variables_reset_instances(Blank3DVariables *vars)
{
    if (!vars) return;
    vr89_reset_instances(&vars->runtime);
    vars->player_owner = 0UL;
}

void blank3d_variables_set_player_owner(Blank3DVariables *vars,
                                        unsigned long owner)
{
    if (vars) vars->player_owner = owner;
}

int blank3d_variables_instance_create(Blank3DVariables *vars,
                                      unsigned long owner)
{
    if (!vars) return 0;
    return vr89_instance_create(&vars->runtime, owner) == VR89_OK;
}

int blank3d_variables_instance_destroy(Blank3DVariables *vars,
                                       unsigned long owner)
{
    if (!vars) return 0;
    return vr89_instance_destroy(&vars->runtime, owner) == VR89_OK;
}

int blank3d_variables_begin_event(Blank3DVariables *vars,
                                  unsigned long owner,
                                  unsigned long event_id)
{
    int r;
    if (!vars) return 0;
    r = vr89_begin_event(&vars->runtime, owner, event_id);
    if (r != VR89_OK) b3d_vars_status(vars, vr89_result_string(r));
    return r == VR89_OK;
}

int blank3d_variables_end_event(Blank3DVariables *vars)
{
    int r;
    if (!vars) return 0;
    r = vr89_end_event(&vars->runtime);
    if (r != VR89_OK) b3d_vars_status(vars, vr89_result_string(r));
    return r == VR89_OK;
}

int blank3d_variables_execute(Blank3DVariables *vars,
                              unsigned long owner,
                              const char *text)
{
    int r;
    int count;
    if (!vars || !text) return 0;
    count = 0;
    r = vr89_execute_buffer(&vars->runtime, owner, text, &count);
    if (r != VR89_OK) {
        b3d_vars_status(vars, vr89_result_string(r));
        return 0;
    }
    sprintf(vars->status, "VarDSL executed %d statement(s)", count);
    return 1;
}

int blank3d_variables_execute_slice(Blank3DVariables *vars,
                                    unsigned long owner,
                                    const char *text,
                                    unsigned int text_len)
{
    unsigned int n;
    if (!vars || !text) return 0;
    n = text_len;
    if (n + 1U > sizeof(vars->script_buffer)) {
        b3d_vars_status(vars, "VarDSL script block exceeds fixed buffer");
        return 0;
    }
    memcpy(vars->script_buffer, text, n);
    vars->script_buffer[n] = '\0';
    return blank3d_variables_execute(vars, owner, vars->script_buffer);
}

int blank3d_variables_set_q16(Blank3DVariables *vars,
                              vr89_scope scope,
                              unsigned long owner,
                              const char *name,
                              long value_q16)
{
    vm89_value value;
    int r;
    if (!vars) return 0;
    vm89_value_fixed_raw(&value, value_q16);
    r = vr89_set(&vars->runtime, scope, owner, name, &value);
    return r == VR89_OK;
}

int blank3d_variables_set_bool(Blank3DVariables *vars,
                               vr89_scope scope,
                               unsigned long owner,
                               const char *name,
                               int value)
{
    vm89_value v;
    int r;
    if (!vars) return 0;
    vm89_value_bool(&v, value);
    r = vr89_set(&vars->runtime, scope, owner, name, &v);
    return r == VR89_OK;
}

int blank3d_variables_get(Blank3DVariables *vars,
                          vr89_scope scope,
                          unsigned long owner,
                          const char *name,
                          vm89_value *out_value)
{
    if (!vars) return 0;
    return vr89_get(&vars->runtime, scope, owner, name, out_value) == VR89_OK;
}

const char *blank3d_variables_status(const Blank3DVariables *vars)
{
    return vars ? vars->status : "VarDSL unavailable";
}
