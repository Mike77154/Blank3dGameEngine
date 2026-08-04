#include <stdio.h>
#include "grecoil89.h"
#include "grecoil89_profiles.h"

static int milli(grec_fp v)
{
    return (int)((v * 1000) >> GREC_FP_SHIFT);
}

int main(void)
{
    GRecState st;
    GRecContext ctx;
    GRecOutput out;
    const GRecProfile *profile;
    int i;

    profile = grec_get_profile(GREC_PROFILE_SMG_9MM);
    if (profile == 0) return 1;

    grec_state_init(&st, 991u);
    grec_context_for_mode(&ctx, GREC_MODE_ADS);

    printf("tick,fire,aim_pitch_milli,aim_yaw_milli,camera_pitch_milli,camera_yaw_milli,weapon_back_milli,weapon_up_milli,spread_milli,shot_index\n");

    for (i = 0; i < 90; ++i) {
        int fire;
        fire = 0;
        if (i < 36 && (i % 3) == 0) {
            grec_fire(&st, profile, &ctx);
            fire = 1;
        }
        grec_update(&st, profile);
        grec_sample(&st, profile, &ctx, &out);
        printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n",
               i,
               fire,
               milli(out.aim_angles.pitch),
               milli(out.aim_angles.yaw),
               milli(out.camera_angles.pitch),
               milli(out.camera_angles.yaw),
               milli(out.weapon_offset.back),
               milli(out.weapon_offset.up),
               milli(out.spread),
               (unsigned)out.shot_index);
    }

    return 0;
}
