#include <stdio.h>
#include <string.h>

#include "blank3d_display_stack89.h"

#define B3D_PRE_DISPLAY_VERB_COUNT 97
#define B3D_DISPLAY_SEXTET_VERB_COUNT 47
#define B3D_REQUIRED_VERB_COUNT \
    (B3D_PRE_DISPLAY_VERB_COUNT + B3D_DISPLAY_SEXTET_VERB_COUNT)

static int dummy_action(void *user, const gverb89_call *call)
{
    (void)user;
    (void)call;
    return GVERB89_HANDLED;
}

int main(void)
{
    Blank3DDisplayStack89 display;
    gverb89_registry registry;
    char name[32];
    int i;

    if (GVERB89_MAX_ENTRIES < B3D_REQUIRED_VERB_COUNT) {
        printf("FAIL: GameVerbs89 capacity %d < runtime requirement %d\n",
               GVERB89_MAX_ENTRIES, B3D_REQUIRED_VERB_COUNT);
        return 1;
    }

    gverb89_init(&registry);
    for (i = 0; i < B3D_PRE_DISPLAY_VERB_COUNT; ++i) {
        sprintf(name, "pre_display_%03d", i);
        if (!gverb89_register_action(&registry, name, dummy_action, 0)) {
            printf("FAIL: could not prefill verb %d\n", i);
            return 2;
        }
    }

    blank3d_display_stack89_init(&display);
    if (!blank3d_display_stack89_register_verbs(&display, &registry)) {
        printf("FAIL: sextet registration rejected at %d/%d entries: %s\n",
               registry.count, GVERB89_MAX_ENTRIES,
               blank3d_display_stack89_status(&display));
        return 3;
    }

    if (registry.count != B3D_REQUIRED_VERB_COUNT) {
        printf("FAIL: expected %d verbs, got %d\n",
               B3D_REQUIRED_VERB_COUNT, registry.count);
        return 4;
    }

    printf("Blank3D GameVerbs runtime capacity: PASS (%d/%d used, %d spare)\n",
           registry.count, GVERB89_MAX_ENTRIES,
           GVERB89_MAX_ENTRIES - registry.count);
    return 0;
}
