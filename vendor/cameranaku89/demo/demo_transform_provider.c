/*
    demo_transform_provider.c
    C89 receive-provider example for engine/external 3D transform math.
*/
#include <stdio.h>
#include "cameranaku89_all.h"

typedef struct demo_transform_stats_s {
    int move_calls;
    int scale_calls;
    int rotate_calls;
} demo_transform_stats;

static int demo_move(void *user, cnk_vec3 position, cnk_vec3 delta, cnk_vec3 *out_position)
{
    demo_transform_stats *stats;
    stats = (demo_transform_stats *)user;
    stats->move_calls += 1;
    out_position->x = position.x + delta.x;
    out_position->y = position.y + delta.y;
    out_position->z = position.z + delta.z;
    return 1;
}

static int demo_scale(void *user, cnk_vec3 value, cnk_vec3 scale, cnk_vec3 *out_value)
{
    demo_transform_stats *stats;
    stats = (demo_transform_stats *)user;
    stats->scale_calls += 1;
    out_value->x = cnk_fx_mul(value.x, scale.x);
    out_value->y = cnk_fx_mul(value.y, scale.y);
    out_value->z = cnk_fx_mul(value.z, scale.z);
    return 1;
}

static int demo_rotate(void *user, cnk_vec3 value, cnk_vec3 euler_deg, cnk_vec3 *out_value)
{
    demo_transform_stats *stats;
    stats = (demo_transform_stats *)user;
    stats->rotate_calls += 1;
    *out_value = cnk_transform_rotate(0, value, euler_deg);
    return 1;
}

int main(void)
{
    demo_transform_stats stats;
    cnk_transform_provider provider;
    cnk_camera cam;
    cnk_profile profile;
    cnk_solver_bind bind;
    cnk_vec3 target_pos;
    cnk_vec3 target_vel;
    cnk_vec3 test;

    stats.move_calls = 0;
    stats.scale_calls = 0;
    stats.rotate_calls = 0;

    cnk_transform_provider_clear(&provider);
    cnk_transform_provider_set(&provider, &stats, demo_move, demo_scale, demo_rotate);

    cnk_camera_reset(&cam);
    cnk_profile_default(&profile);
    cnk_profile_style_ots(&profile);
    cnk_camera_apply_profile(&cam, &profile);

    bind.camera = &cam;
    bind.target_id = 7;
    bind.enabled = 1;
    cnk_adapter_receive_transform_provider(&bind, &provider);

    target_pos = cnk_vec3_make(0, 0, 0);
    target_vel = cnk_vec3_make(0, 0, 0);
    cnk_camera_set_target(&cam, target_pos, target_vel, CNK_DEG(25), 0, 0);
    cnk_camera_snap(&cam);
    cnk_camera_update(&cam, 1, 0, 0);

    test = cnk_transform_apply_trs(
        &provider,
        cnk_vec3_make(CNK_ONE, CNK_ONE, CNK_ONE),
        cnk_vec3_make(CNK_FX_FROM_INT(2), CNK_ONE, CNK_ONE),
        cnk_vec3_make(CNK_DEG(45), 0, 0),
        cnk_vec3_make(CNK_FX_FROM_INT(3), 0, 0));

    printf("mode=%d move=%d scale=%d rotate=%d camera=(%d,%d,%d) test=(%d,%d,%d)\n",
        cam.transform_provider.mode,
        stats.move_calls,
        stats.scale_calls,
        stats.rotate_calls,
        CNK_FX_TO_INT(cam.state.pose.pos.x),
        CNK_FX_TO_INT(cam.state.pose.pos.y),
        CNK_FX_TO_INT(cam.state.pose.pos.z),
        CNK_FX_TO_INT(test.x),
        CNK_FX_TO_INT(test.y),
        CNK_FX_TO_INT(test.z));

    if (stats.move_calls <= 0 || stats.scale_calls <= 0 || stats.rotate_calls <= 0) {
        return 1;
    }
    return 0;
}
