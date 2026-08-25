#include "invariantSpecialoperations_89.h"
#include <stdio.h>

typedef struct TestState {
    long values[4];
    int present[4];
} TestState;

static int tget(void *user, iso89_subject subject, iso89_value *out)
{
    TestState *s;
    int i;
    s = (TestState *)user;
    if (!s || subject == 0UL || subject > 4UL || !out) return 0;
    i = (int)subject - 1;
    if (!s->present[i]) { *out = 0L; return 1; }
    *out = s->values[i];
    return 1;
}

static int tset(void *user, iso89_subject subject, iso89_value value)
{
    TestState *s;
    int i;
    s = (TestState *)user;
    if (!s || subject == 0UL || subject > 4UL) return 0;
    i = (int)subject - 1;
    s->values[i] = value;
    s->present[i] = 1;
    return 1;
}

int main(void)
{
    iso89_rule rules[8];
    iso89_context ctx;
    iso89_provider provider;
    TestState state;
    int i;
    for (i = 0; i < 4; ++i) { state.values[i] = 0L; state.present[i] = 0; }
    iso89_context_init(&ctx, rules, 8);
    if (!iso89_add_relation(&ctx, 1UL, 1L, 2UL, 0L,
                            ISO89_SPECOP_VICEVERSA)) return 1;
    provider.get_value = tget;
    provider.set_value = tset;
    provider.user = &state;
    if (!iso89_apply_event(&ctx, &provider, 1UL, 1L)) return 2;
    if (state.values[0] != 1L || state.values[1] != 0L) return 3;
    if (!iso89_apply_event(&ctx, &provider, 2UL, 1L)) return 4;
    if (state.values[0] != 0L || state.values[1] != 1L) return 5;
    puts("invariantSpecialoperations_89 core: OK");
    return 0;
}
