#include "invariantSpecialoperations_89.h"
#include "invariantSpecialoperations_89_flags89.h"
#include <stdio.h>

int main(void)
{
    FlagStore store;
    FlagStoreEntry entries[16];
    char pool[512];
    const char *names[2];
    iso89_flags89_binding binding;
    iso89_provider provider;
    iso89_rule rules[4];
    iso89_context ctx;
    FlagsValue value;
    names[0] = "walk_forward";
    names[1] = "run_forward";
    flagstore_init(&store, entries, 16, pool, sizeof(pool));
    iso89_flags89_binding_init(&binding, &store, names, 2);
    iso89_flags89_make_provider(&binding, &provider);
    iso89_context_init(&ctx, rules, 4);
    if (!iso89_add_relation(&ctx, 1UL, 1L, 2UL, 0L,
                            ISO89_SPECOP_VICEVERSA)) return 1;
    if (!iso89_apply_event(&ctx, &provider, 1UL, 1L)) return 2;
    if (!iso89_apply_event(&ctx, &provider, 2UL, 1L)) return 3;
    if (!flagstore_get(&store, "walk_forward", &value) || value.as.i != 0L)
        return 4;
    if (!flagstore_get(&store, "run_forward", &value) || value.as.i != 1L)
        return 5;
    puts("invariantSpecialoperations_89 flags89 adapter: OK");
    return 0;
}
