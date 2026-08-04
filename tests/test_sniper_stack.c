#include "blank3d_sniper.h"

#include <stdio.h>
#include <string.h>

typedef struct TestCountersTag {
    int raycasts;
    int hud_commands;
    int preset_commands;
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

int main(void)
{
    Blank3DSniper sniper;
    TestCounters counters;
    g89_camera camera;
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
                        "re5_psg1_game_scope");
    if (!sniper.preset) return 1;

    for (frame = 0; frame < 24; ++frame) {
        blank3d_sniper_update(&sniper, 1, 1, 0,
                              15, 10, 1, &camera);
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

    for (frame = 0; frame < 24; ++frame) {
        blank3d_sniper_update(&sniper, 0, 0, 0,
                              0, 0, 1, &camera);
    }
    if (blank3d_sniper_is_scoped(&sniper)) return 9;

    printf("Blank3D native sniper stack test: OK (%d HUD, %d preset)\n",
           counters.hud_commands, counters.preset_commands);
    return 0;
}
