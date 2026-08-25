#include "var_runtime89.h"

#include <stdio.h>
#include <string.h>

typedef struct TestNumericTag {
    long health_q16;
} TestNumeric;

typedef struct TestFlagsTag {
    int ready;
} TestFlags;

static int num_claim(void *user, vr89_scope scope, vr89_owner owner,
                     const char *name, vr89_operation op,
                     const vm89_value *value)
{
    (void)user; (void)scope; (void)owner; (void)op; (void)value;
    return strcmp(name, "health") == 0;
}

static int num_get(void *user, vr89_scope scope, vr89_owner owner,
                   const char *name, vm89_value *out)
{
    TestNumeric *n;
    (void)scope; (void)owner; (void)name;
    n = (TestNumeric *)user;
    vm89_value_fixed_raw(out, n->health_q16);
    return 1;
}

static int num_set(void *user, vr89_scope scope, vr89_owner owner,
                   const char *name, const vm89_value *value)
{
    TestNumeric *n;
    (void)scope; (void)owner; (void)name;
    if (value->type != VM89_VALUE_FIXED) return 0;
    n = (TestNumeric *)user;
    n->health_q16 = value->fixed_q16;
    return 1;
}

static int num_add(void *user, vr89_scope scope, vr89_owner owner,
                   const char *name, const vm89_value *value)
{
    TestNumeric *n;
    (void)scope; (void)owner; (void)name;
    if (value->type != VM89_VALUE_FIXED) return 0;
    n = (TestNumeric *)user;
    n->health_q16 += value->fixed_q16;
    return 1;
}

static int num_sub(void *user, vr89_scope scope, vr89_owner owner,
                   const char *name, const vm89_value *value)
{
    TestNumeric *n;
    (void)scope; (void)owner; (void)name;
    if (value->type != VM89_VALUE_FIXED) return 0;
    n = (TestNumeric *)user;
    n->health_q16 -= value->fixed_q16;
    return 1;
}

static int flag_claim(void *user, vr89_scope scope, vr89_owner owner,
                      const char *name, vr89_operation op,
                      const vm89_value *value)
{
    (void)user; (void)scope; (void)owner;
    if (strcmp(name, "ready") == 0) return 1;
    return op == VR89_OP_SET && value && value->type == VM89_VALUE_BOOL;
}

static int flag_get(void *user, vr89_scope scope, vr89_owner owner,
                    const char *name, vm89_value *out)
{
    TestFlags *f;
    (void)scope; (void)owner; (void)name;
    f = (TestFlags *)user;
    vm89_value_bool(out, f->ready);
    return 1;
}

static int flag_set(void *user, vr89_scope scope, vr89_owner owner,
                    const char *name, const vm89_value *value)
{
    TestFlags *f;
    (void)scope; (void)owner; (void)name;
    if (value->type != VM89_VALUE_BOOL) return 0;
    f = (TestFlags *)user;
    f->ready = value->boolean;
    return 1;
}

static int require_int(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    vr89_runtime runtime;
    vr89_provider p;
    TestNumeric numeric;
    TestFlags flags;
    vm89_value value;
    int statements;

    memset(&numeric, 0, sizeof(numeric));
    memset(&flags, 0, sizeof(flags));
    vr89_init(&runtime);

    memset(&p, 0, sizeof(p));
    p.name = "numeric"; p.priority = 200; p.user = &numeric;
    p.claim = num_claim; p.get = num_get; p.set = num_set;
    p.add = num_add; p.sub = num_sub;
    if (!require_int(vr89_add_provider(&runtime, &p) == VR89_OK,
                     "numeric provider")) return 1;

    memset(&p, 0, sizeof(p));
    p.name = "flags"; p.priority = 100; p.user = &flags;
    p.claim = flag_claim; p.get = flag_get; p.set = flag_set;
    if (!require_int(vr89_add_provider(&runtime, &p) == VR89_OK,
                     "flag provider")) return 1;

    if (!require_int(vr89_instance_create(&runtime, 7UL) == VR89_OK,
                     "instance create")) return 1;
    if (!require_int(vr89_begin_event(&runtime, 7UL, 2UL) == VR89_OK,
                     "begin event")) return 1;

    statements = 0;
    if (!require_int(vr89_execute_buffer(&runtime, 7UL,
        "health = 100; health -= 25; ready = true;\n"
        "foo = 3; var temp = 2; copy = foo; foo += temp;\n"
        "global.score = 10;", &statements) == VR89_OK,
        "execute buffer")) return 1;
    if (!require_int(statements == 8, "statement count")) return 1;
    if (!require_int(numeric.health_q16 == 75L * 65536L,
                     "numeric routed")) return 1;
    if (!require_int(flags.ready == 1, "bool routed")) return 1;
    if (!require_int(vr89_get(&runtime, VR89_SCOPE_INSTANCE, 7UL,
                              "foo", &value) == VR89_OK &&
                     value.fixed_q16 == 5L * 65536L,
                     "dynamic instance + local rhs")) return 1;
    if (!require_int(vr89_get(&runtime, VR89_SCOPE_INSTANCE, 7UL,
                              "copy", &value) == VR89_OK &&
                     value.fixed_q16 == 3L * 65536L,
                     "rhs variable copy")) return 1;
    if (!require_int(vr89_get(&runtime, VR89_SCOPE_GLOBAL, 0UL,
                              "score", &value) == VR89_OK &&
                     value.fixed_q16 == 10L * 65536L,
                     "explicit global")) return 1;
    if (!require_int(vr89_get(&runtime, VR89_SCOPE_LOCAL, 7UL,
                              "temp", &value) == VR89_OK,
                     "local exists in event")) return 1;

    if (!require_int(vr89_end_event(&runtime) == VR89_OK,
                     "end event")) return 1;
    if (!require_int(vr89_get(&runtime, VR89_SCOPE_LOCAL, 7UL,
                              "temp", &value) == VR89_ERR_NO_FRAME,
                     "local cleared")) return 1;

    printf("OK: var_runtime89 provider routing + dynamic vars + locals\n");
    return 0;
}
