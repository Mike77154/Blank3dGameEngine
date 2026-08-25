#include "blank3d_sniper.h"

#include <stdio.h>
#include <string.h>

typedef struct TestCountersTag {
    int raycasts;
    int hud_commands;
    int preset_commands;
    int provider_paint_commands;
    int provider_animation_calls;
} TestCounters;

static int test_raycast(void *user,
                        const g89_vec3 *from,
                        const g89_vec3 *dir,
                        g89_fx max_dist,
                        gaq89_hit *out_hit)
{
    TestCounters *counters;
    counters = (TestCounters *)user;
    if (!from || !dir || !out_hit || max_dist <= 0L) return 0;
    counters->raycasts += 1;
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->valid = 1;
    out_hit->target_id = 42;
    out_hit->target_kind = GAQ89_TARGET_ACTOR;
    out_hit->target_part = 1;
    out_hit->distance = G89_FX_FROM_INT(75);
    out_hit->position.x = from->x + (dir->x * 75L);
    out_hit->position.y = from->y + (dir->y * 75L);
    out_hit->position.z = from->z + (dir->z * 75L);
    out_hit->normal.y = G89_FX_ONE;
    return 1;
}

static void test_hud_emit(void *user, const gsh89_cmd *cmd)
{
    TestCounters *counters;
    counters = (TestCounters *)user;
    if (cmd && cmd->kind != 0) counters->hud_commands += 1;
}

static void test_preset_emit(void *user, const gsp89_draw_cmd *cmd)
{
    TestCounters *counters;
    counters = (TestCounters *)user;
    if (cmd && cmd->kind != GSP89_CMD_NONE) counters->preset_commands += 1;
}


static int test_provider_animation(void *user,
                                   const gsa89_pose *pose,
                                   const gsv89_shape *src,
                                   short shape_count,
                                   gsv89_shape *dst,
                                   short dst_capacity)
{
    TestCounters *counters;
    (void)pose;
    (void)src;
    (void)shape_count;
    (void)dst;
    (void)dst_capacity;
    counters = (TestCounters *)user;
    counters->provider_animation_calls += 1;
    return GPR89_FALLBACK;
}

static int test_provider_paint(void *user, const gsp89_draw_cmd *cmd)
{
    TestCounters *counters;
    counters = (TestCounters *)user;
    if (cmd && cmd->kind != GSP89_CMD_NONE)
        counters->provider_paint_commands += 1;
    return GPR89_FALLBACK;
}

int main(void)
{
    Blank3DSniper sniper;
    TestCounters counters;
    g89_camera camera;
    gpr89_provider host_provider;
    int frame;

    memset(&counters, 0, sizeof(counters));
    memset(&camera, 0, sizeof(camera));
    camera.pos.y = G89_FX_FROM_INT(2);
    camera.forward.z = G89_FX_ONE;
    camera.right.x = G89_FX_ONE;
    camera.up.y = G89_FX_ONE;
    camera.base_fov_deg_x100 = 7000;
    camera.current_fov_deg_x100 = 7000;

    blank3d_sniper_init(&sniper, 1280, 720, 7000,
                        test_raycast, &counters,
                        "config/scope/hud/sniper_re5_psg1.ini");
    if (!sniper.preset) return 1;
    if (!blank3d_sniper_scope_recipe_loaded(&sniper)) return 10;
    if (strcmp(sniper.preset->name, "re5_psg1_game_scope") != 0) return 11;
    gpr89_provider_init(&host_provider, "host");
    host_provider.capabilities = GPR89_CAP_PAINT | GPR89_CAP_ANIMATION;
    host_provider.user = &counters;
    host_provider.emit_draw_cmd = test_provider_paint;
    host_provider.animate_shapes = test_provider_animation;
    if (!blank3d_sniper_register_scope_provider(&sniper, &host_provider)) return 12;
    if (!blank3d_sniper_scope_animation_loaded(&sniper)) return 14;

    blank3d_sniper_update(&sniper, 1, 1, 0,
                          15, 10, 1, 16, &camera);
    if (sniper.animation_pose.root.alpha_x1000 >= 1000L) return 15;
    for (frame = 1; frame < 24; ++frame) {
        blank3d_sniper_update(&sniper, 1, 1, 0,
                              15, 10, 1, 16, &camera);
    }

    if (!blank3d_sniper_is_scoped(&sniper)) return 2;
    if (blank3d_sniper_fov_deg_x100(&sniper) >= 7000) return 3;
    if (blank3d_sniper_sensitivity_pct(&sniper) >= 100) return 4;
    if (counters.raycasts < 1) return 5;
    if (!sniper.telemetry.target_valid ||
        sniper.telemetry.target_id != 42) return 6;

    blank3d_sniper_emit(&sniper,
                        test_hud_emit, &counters,
                        test_preset_emit, &counters);
    if (counters.hud_commands < 1) return 7;
    if (counters.preset_commands < 1) return 8;
    if (counters.provider_paint_commands < 1) return 13;
    if (counters.provider_animation_calls < 1) return 16;

    if (blank3d_sniper_trigger_event(&sniper, "fire") < 1) return 17;
    blank3d_sniper_update(&sniper, 1, 1, 0, 15, 10, 1, 20, &camera);
    if (sniper.animation_pose.root.scale_x1000 <= 1000L) return 18;

    for (frame = 0; frame < 24; ++frame) {
        blank3d_sniper_update(&sniper, 0, 0, 0,
                              0, 0, 1, 16, &camera);
    }
    if (blank3d_sniper_is_scoped(&sniper)) return 9;

    printf("Blank3D native sniper stack test: OK (%d HUD, %d preset)\n",
           counters.hud_commands, counters.preset_commands);
    return 0;
}
