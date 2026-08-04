#include "blank3d_numbar.h"
#include "blank3d_hud.h"

#include <stdio.h>
#include <string.h>

static int draw_calls;
static int last_anchor;
static int last_scale_x;
static int last_scale_y;

/* The orchestration test does not need OpenGL; this stub verifies dispatch. */
void blank3d_hud_draw_layout_scaled(Blank3DHud *hud,
                                    Blank3DBigHud *layout,
                                    const Blank3DBigHudTelemetry *telemetry,
                                    unsigned int frame_ms,
                                    int anchor,
                                    int offset_x,
                                    int offset_y,
                                    int scale_x_q8,
                                    int scale_y_q8,
                                    int canvas_width,
                                    int canvas_height)
{
    (void)hud;
    (void)layout;
    (void)telemetry;
    (void)frame_ms;
    (void)offset_x;
    (void)offset_y;
    if (canvas_width < 1 || canvas_height < 1) return;
    ++draw_calls;
    last_anchor = anchor;
    last_scale_x = scale_x_q8;
    last_scale_y = scale_y_q8;
}

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static long resolve(void *user,
                    unsigned long entity_id,
                    void *native_entity,
                    const char *binding,
                    long fallback,
                    int *resolved)
{
    (void)user;
    (void)entity_id;
    (void)native_entity;
    if (resolved) *resolved = 1;
    if (strcmp(binding, "player.health") == 0) return 73L;
    if (strcmp(binding, "player.health_max") == 0) return 100L;
    if (strcmp(binding, "player.alive") == 0) return 1L;
    if (strcmp(binding, "gameplay.threat") == 0) return 16L;
    if (strcmp(binding, "weapon.loaded") == 0) return 13L;
    if (strcmp(binding, "weapon.capacity") == 0) return 15L;
    if (strcmp(binding, "weapon.reserve") == 0) return 179L;
    if (resolved) *resolved = 0;
    return fallback;
}

static Blank3DNumbarInstance *find_instance(Blank3DNumbarSystem *system,
                                             const char *name)
{
    int i;
    for (i = 0; i < system->instance_count; ++i) {
        if (system->instances[i].alive &&
            strcmp(system->instances[i].orchestrator.name, name) == 0)
            return &system->instances[i];
    }
    return (Blank3DNumbarInstance *)0;
}

int main(void)
{
    static Blank3DNumbarSystem system;
    static Blank3DHud hud;
    Blank3DNumbarInstance *instance;
    int loaded;
    int cached;
    int showcase;
    int all_presets;

    blank3d_numbar_init(&system, resolve, (void *)0);
    blank3d_numbar_begin_frame(&system);
    loaded = blank3d_numbar_request(&system, 1UL, (void *)0,
                                    "config/hud/gameplay.ini");
    if (loaded != 3) return fail(blank3d_numbar_status(&system));
    if (system.instance_count != 3) return fail("gameplay instance count");

    instance = find_instance(&system, "player_health");
    if (!instance) return fail("player_health orchestrator missing");
    if (instance->layout.canvas_width != 108 ||
        instance->layout.canvas_height != 108)
        return fail("preset natural canvas missing");
    if (strcmp(instance->orchestrator.value_bind, "player.health") != 0 ||
        strcmp(instance->orchestrator.max_bind, "player.health_max") != 0)
        return fail("health INI bindings missing");

    instance = find_instance(&system, "player_ammo");
    if (!instance) return fail("player_ammo orchestrator missing");
    if (strcmp(instance->orchestrator.value2_bind, "weapon.reserve") != 0 ||
        strcmp(instance->orchestrator.units_bind, "weapon.capacity") != 0)
        return fail("ammo secondary/dynamic-unit bindings missing");

    cached = blank3d_numbar_request(&system, 1UL, (void *)0,
                                    "config/hud/gameplay.ini");
    if (cached != 3 || system.instance_count != 3)
        return fail("orchestrator cache duplicated instances");

    blank3d_numbar_begin_frame(&system);
    if (blank3d_numbar_request(&system, 1UL, (void *)0,
                               "config/hud/gameplay.ini") != 3)
        return fail("cached gameplay request failed");
    draw_calls = 0;
    blank3d_numbar_draw(&system, &hud, 16u);
    if (draw_calls != 3) return fail("requested instances were not dispatched");
    if (last_scale_x != 256 || last_scale_y != 256)
        return fail("orchestrator scale was not propagated");
    if (last_anchor != B3D_BIGHUD_ANCHOR_BOTTOM_RIGHT)
        return fail("z ordering or anchor propagation failed");

    showcase = blank3d_numbar_request(&system, 2UL, (void *)0,
                         "config/hud/examples/showcase.ini");
    if (showcase != 7) return fail(blank3d_numbar_status(&system));
    if (system.instance_count != 10)
        return fail("showcase did not load every showcase preset");

    all_presets = blank3d_numbar_request(&system, 3UL, (void *)0,
                         "config/hud/examples/all_presets.ini");
    if (all_presets != 20) return fail(blank3d_numbar_status(&system));
    if (system.instance_count != 30)
        return fail("not every shipped preset loaded");

    blank3d_numbar_begin_frame(&system);
    if (blank3d_numbar_request(&system, 1UL, (void *)0,
                               "config/hud/gameplay.ini") != 3)
        return fail("gameplay request after full catalog failed");
    blank3d_numbar_draw(&system, &hud, 16u);
    if (system.instance_count != 3)
        return fail("stale actor/preset slots were not released");

    printf("NumBar orchestration OK: gameplay=%d showcase=%d all=%d cached=%d active=%d\n",
           loaded, showcase, all_presets, cached, system.instance_count);
    return 0;
}
