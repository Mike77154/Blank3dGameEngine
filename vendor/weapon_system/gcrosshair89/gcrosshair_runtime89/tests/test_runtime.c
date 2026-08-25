#include <stdio.h>
#include <string.h>
#include "gcrosshair_runtime89.h"

static int semantic_seen;

static int semantic_vector(void *user,
                           const GC89P_VectorCommand *command,
                           int *out_emitted)
{
    int target;
    target = *(int *)user;
    if (!command || !command->meta) return GC89P_UNHANDLED;
    semantic_seen = command->meta->semantic_id;
    if (command->meta->semantic_id != target) return GC89P_UNHANDLED;
    *out_emitted = 5;
    return GC89P_HANDLED;
}

int main(void)
{
    GC89R_Runtime runtime;
    GC89P_VectorProvider vector;
    GC89P_SoftwareSurface surface;
    GC89P_DrawReport report;
    GC89_InputState input;
    unsigned char rgba[128 * 128 * 4];
    int target;

    memset(&input, 0, sizeof(input));
    if (!gc89r_init(&runtime, "../gcrosshair_base89/recipes/gcrosshair.ini")) {
        puts("runtime init failed");
        return 1;
    }

    /* A native ABI3 shape must work with only bundled primitives. */
    if (!gc89r_set_preset(&runtime, 144)) return 1;
    gc89p_surface_init(&surface, rgba, 128, 128, 128 * 4);
    gc89p_surface_clear(&surface, GC89_RGBA(0, 0, 0, 0));
    if (gc89r_draw_rgba(&runtime, &input, &surface, &report) <= 0) {
        puts("standalone native vector failed");
        return 1;
    }
    if (!report.internal_vector_used) return 1;

    /* Legacy semantic stub can be delegated by preset identity. */
    if (!gc89r_set_preset(&runtime, 39)) return 1;
    memset(&vector, 0, sizeof(vector));
    vector.draw_vector = semantic_vector;
    target = 39;
    semantic_seen = -1;
    gc89p_runtime_set_vector_provider(gc89r_providers(&runtime),
                                      &vector, &target);
    if (gc89r_draw(&runtime, 128, 128, &input, &report) != 5) {
        puts("semantic provider failed");
        return 1;
    }
    if (semantic_seen != 39 || !report.vector_provider_used) return 1;

    if (!runtime.animation_recipe.events[GCB89_ANIM_EVENT_FIRE].enabled) return 20;
    if (!gc89r_trigger_event(&runtime, GCB89_ANIM_EVENT_CUSTOM1)) return 21;
    gc89r_update(&runtime, 1UL);
    if (!gc89r_animation_modifier(&runtime)) return 22;
    puts("runtime89 tests: OK");
    return 0;
}
