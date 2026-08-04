#include <stdio.h>
#include "gsynthordnance89.h"

#define TEST_RATE 44100U
#define TEST_LOGICAL 32U

static gv89_voice voice_storage[TEST_LOGICAL];
static gsso89_bullet_voice bullet_storage[4];
static gsso89_grenade_voice grenade_storage[2];
static gsso89_rocket_voice rocket_storage[1];

int main(void)
{
    gwv89_context handler;
    gsso89_context ordnance;
    gsso89_bullet_params bullet;
    gsso89_grenade_params grenade;
    gsso89_rocket_params rocket;
    gv89_stats stats;
    gv89_u32 i;
    gv89_s16 left;
    gv89_s16 right;
    if (gwv89_init_ex(&handler, voice_storage, TEST_LOGICAL, 16U,
                      TEST_RATE) != GV89_OK) return 1;
    if (!gsso89_init(&ordnance, bullet_storage, 4U,
                     grenade_storage, 2U, rocket_storage, 1U,
                     TEST_RATE)) return 2;
    gsso89_bullet_defaults(&bullet);
    bullet.preset_id = GBA89_PRESET_SUPERSONIC_SNAP;
    bullet.common.instance_key = 100U;
    if (gsso89_play_bullet(&ordnance, &handler, &bullet, 1U, 0) != GV89_OK)
        return 3;
    gsso89_grenade_defaults(&grenade);
    grenade.common.instance_key = 200U;
    if (gsso89_play_grenade(&ordnance, &handler, &grenade, 2U, 0) != GV89_OK)
        return 4;
    gsso89_rocket_defaults(&rocket);
    rocket.synth_params.noise_level_q15[WSRB89_NOISE_RUMBLE] = 24000;
    rocket.synth_params.noise_level_q15[WSRB89_NOISE_CRACKLE] = 12000;
    rocket.synth_params.motion_lfo_depth_q15 = 4200U;
    gsso89_rocket_enable_custom(&rocket);
    rocket.common.instance_key = 300U;
    if (gsso89_play_rocket(&ordnance, &handler, &rocket, 3U, 0) != GV89_OK)
        return 5;
    for (i = 0U; i < TEST_RATE * 4U; ++i) {
        left = 0;
        right = 0;
        gwv89_process_stereo_sample(&handler, &left, &right);
    }
    gwv89_get_stats(&handler, &stats);
    if (stats.starts < 3U) return 6;
    if (stats.rejects != 0U) return 7;
    printf("gsynthordnance89 PASS: bullet=%lu grenade=%lu rocket=%lu context=%lu starts=%lu\n",
           (unsigned long)gsso89_bullet_voice_bytes(),
           (unsigned long)gsso89_grenade_voice_bytes(),
           (unsigned long)gsso89_rocket_voice_bytes(),
           (unsigned long)gsso89_context_bytes(),
           (unsigned long)stats.starts);
    return 0;
}
