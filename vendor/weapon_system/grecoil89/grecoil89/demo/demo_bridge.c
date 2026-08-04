#include <stdio.h>
#include "grecoil89.h"
#include "grecoil89_profiles.h"
#include "grecoil89_bridge.h"

typedef struct DemoEngine_s {
    grec_fp camera_pitch;
    grec_fp camera_yaw;
    grec_fp aim_pitch;
    grec_fp aim_yaw;
    grec_fp current_spread;
    grec_fp weapon_back;
    grec_fp weapon_up;
    int fire_events;
    int update_events;
} DemoEngine;

static int fp_to_milli(grec_fp v)
{
    return (int)((v * 1000) >> GREC_FP_SHIFT);
}

static void demo_camera(void *user, const GRecAngles *angles)
{
    DemoEngine *eng;
    eng = (DemoEngine *)user;
    eng->camera_pitch = angles->pitch;
    eng->camera_yaw = angles->yaw;
}

static void demo_aim(void *user, const GRecAngles *angles, grec_fp spread)
{
    DemoEngine *eng;
    eng = (DemoEngine *)user;
    eng->aim_pitch = angles->pitch;
    eng->aim_yaw = angles->yaw;
    eng->current_spread = spread;
}

static void demo_weapon(void *user, const GRecAngles *angles, const GRecVec3 *offset)
{
    DemoEngine *eng;
    (void)angles;
    eng = (DemoEngine *)user;
    eng->weapon_back = offset->back;
    eng->weapon_up = offset->up;
}

static void demo_on_fire(void *user, const GRecOutput *out)
{
    DemoEngine *eng;
    (void)out;
    eng = (DemoEngine *)user;
    eng->fire_events++;
}

static void demo_on_update(void *user, const GRecOutput *out)
{
    DemoEngine *eng;
    (void)out;
    eng = (DemoEngine *)user;
    eng->update_events++;
}

int main(void)
{
    GRecState st;
    GRecContext ctx;
    GRecBridge bridge;
    DemoEngine eng;
    const GRecProfile *profile;
    int i;

    profile = grec_get_profile(GREC_PROFILE_SHOTGUN_12G);
    if (profile == 0) return 1;

    eng.camera_pitch = 0;
    eng.camera_yaw = 0;
    eng.aim_pitch = 0;
    eng.aim_yaw = 0;
    eng.current_spread = 0;
    eng.weapon_back = 0;
    eng.weapon_up = 0;
    eng.fire_events = 0;
    eng.update_events = 0;

    grec_state_init(&st, 777u);
    grec_context_for_mode(&ctx, GREC_MODE_FIXED_CAM);
    grec_bridge_init(&bridge, &eng, demo_camera, demo_aim, demo_weapon, demo_on_fire, demo_on_update);

    grec_fire(&st, profile, &ctx);
    grec_bridge_after_fire(&bridge, &st, profile, &ctx);

    for (i = 0; i < 32; ++i) {
        grec_update(&st, profile);
        grec_bridge_after_update(&bridge, &st, profile, &ctx);
    }

    printf("bridge demo profile=%s\n", profile->name);
    printf("camera_pitch_milli=%d camera_yaw_milli=%d\n", fp_to_milli(eng.camera_pitch), fp_to_milli(eng.camera_yaw));
    printf("aim_pitch_milli=%d aim_yaw_milli=%d spread_milli=%d\n", fp_to_milli(eng.aim_pitch), fp_to_milli(eng.aim_yaw), fp_to_milli(eng.current_spread));
    printf("weapon_back_milli=%d weapon_up_milli=%d\n", fp_to_milli(eng.weapon_back), fp_to_milli(eng.weapon_up));
    printf("events fire=%d update=%d\n", eng.fire_events, eng.update_events);

    return 0;
}
