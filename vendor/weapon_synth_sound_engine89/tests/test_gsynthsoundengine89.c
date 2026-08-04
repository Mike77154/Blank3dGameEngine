#include "gsynthsoundengine89.h"

#define TEST_RATE 44100U
#define TEST_LOGICAL 64U
#define TEST_PHYSICAL 24U
#define TEST_CASINGS 24U
#define TEST_FIRES 4U

static gv89_voice test_voice_storage[TEST_LOGICAL];
static gsse89_casing_voice test_casing_storage[TEST_CASINGS];
static gsse89_fire_voice test_fire_storage[TEST_FIRES];

int main(void)
{
    gwv89_context handler;
    gsse89_context extras;
    gsse89_casing_params casing;
    gsse89_fire_params fire;
    gv89_handle handle;
    gv89_s16 left;
    gv89_s16 right;
    gv89_u32 i;
    gv89_u32 energy;
    if (gwv89_init_ex(&handler, test_voice_storage, TEST_LOGICAL,
                      TEST_PHYSICAL, TEST_RATE) != GV89_OK) return 1;
    if (!gsse89_init(&extras, test_casing_storage, TEST_CASINGS,
                     test_fire_storage, TEST_FIRES, TEST_RATE)) return 2;
    gsse89_casing_defaults(&casing);
    casing.shell = GT89_SHELL_RIFLE_BRASS;
    casing.surface = GT89_SURFACE_CONCRETE;
    casing.instance_key = 10U;
    for (i = 0U; i < 12U; ++i) {
        casing.pan_q15 = (gv89_s16)(-24000 + (gv89_s16)(i * 4200U));
        casing.variation = (gt89_u8)i;
        if (gsse89_play_casing(&extras, &handler, &casing,
                               100U + i, &handle) != GV89_OK) return 3;
    }
    gsse89_fire_defaults(&fire, GSSE89_FIRE_ROLE_FLAMETHROWER);
    fire.duration_ms = 900U;
    fire.instance_key = 20U;
    if (gsse89_play_fire(&extras, &handler, &fire, 991U, &handle) != GV89_OK)
        return 4;
    energy = 0U;
    for (i = 0U; i < TEST_RATE * 2U; ++i) {
        left = 0;
        right = 0;
        gwv89_process_stereo_sample(&handler, &left, &right);
        if (left < 0) energy += (gv89_u32)(-(gv89_s32)left);
        else energy += (gv89_u32)left;
        if (right < 0) energy += (gv89_u32)(-(gv89_s32)right);
        else energy += (gv89_u32)right;
    }
    if (energy == 0U) return 5;
    if (gsse89_active_fires(&extras) != 0U) return 6;
    return 0;
}
