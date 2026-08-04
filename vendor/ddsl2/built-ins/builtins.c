#include "built-ins/builtins.h"

int ddsl_builtin_truthy(void *user, const ddsl_value *args, int argc, ddsl_value *out) {
    (void)user;
    if (!out) return 0;
    if (!args || argc <= 0) {
        *out = ddsl_v_bool(0);
        return 1;
    }
    *out = ddsl_v_bool(ddsl_value_truthy(args[0]));
    return 1;
}

int ddsl_builtins_register_core(ddsl_registry *r) {
    if (!r) return 0;
    return ddsl_registry_add(r, "truthy", ddsl_builtin_truthy, 0);
}
