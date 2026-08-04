#include "stdlib/stdlib.h"

int ddsl_stdlib_set_usage_flag(ddsl_store *st, const char *name) {
    if (!st || !name) return 0;
    return ddsl_store_set(st, name, "true");
}

int ddsl_stdlib_load_defaults(ddsl_store *st) {
    int ok;
    if (!st) return 0;
    ok = 1;
    ok = ddsl_store_set(st, "dsl_c89", "true") && ok;
    ok = ddsl_store_set(st, "fixed_point", "true") && ok;
    return ok;
}
