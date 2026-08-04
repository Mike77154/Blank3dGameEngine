#include <stdio.h>

#include "gweaponvoice89.h"

#define TEST_VOICES 16U
#define TEST_RATE 44100U

static gv89_voice voices[TEST_VOICES];

static int test_order(const gwv89_context *ctx)
{
    if (ctx->classes[GWV89_EVENT_REPORT].gain_q15 <=
        ctx->classes[GWV89_EVENT_MECHANISM].gain_q15) return 0;
    if (ctx->classes[GWV89_EVENT_EXPLOSION].gain_q15 <=
        ctx->classes[GWV89_EVENT_IMPACT].gain_q15) return 0;
    if (ctx->classes[GWV89_EVENT_MECHANISM].gain_q15 <=
        ctx->classes[GWV89_EVENT_AMBIENCE].gain_q15) return 0;
    if (ctx->classes[GWV89_EVENT_CASING].gain_q15 <= 0) return 0;
    return 1;
}

int main(void)
{
    gwv89_context ctx;
    gv89_s16 realistic_mech;
    gv89_s16 hybrid_mech;
    gv89_s16 cinematic_mech;

    if (gwv89_init_ex(&ctx, voices, TEST_VOICES, 8U, TEST_RATE) != GV89_OK)
        return 1;
    if (!test_order(&ctx)) return 2;

    if (gwv89_apply_mix_profile(&ctx, GWV89_MIX_REALISTIC) != GV89_OK)
        return 3;
    if (!test_order(&ctx)) return 4;
    realistic_mech = ctx.classes[GWV89_EVENT_MECHANISM].gain_q15;

    if (gwv89_apply_mix_profile(&ctx, GWV89_MIX_HYBRID) != GV89_OK)
        return 5;
    if (!test_order(&ctx)) return 6;
    hybrid_mech = ctx.classes[GWV89_EVENT_MECHANISM].gain_q15;

    if (gwv89_apply_mix_profile(&ctx, GWV89_MIX_CINEMATIC) != GV89_OK)
        return 7;
    if (!test_order(&ctx)) return 8;
    cinematic_mech = ctx.classes[GWV89_EVENT_MECHANISM].gain_q15;

    if (!(realistic_mech < hybrid_mech && hybrid_mech < cinematic_mech))
        return 9;
    if (gwv89_apply_mix_profile(&ctx,
            (gwv89_mix_profile)GWV89_MIX_PROFILE_COUNT) != GV89_BAD_ARGUMENT)
        return 10;

    printf("weapon mix hierarchy PASS: mech=%d/%d/%d report=%d explosion=%d\n",
           (int)realistic_mech, (int)hybrid_mech, (int)cinematic_mech,
           (int)ctx.classes[GWV89_EVENT_REPORT].gain_q15,
           (int)ctx.classes[GWV89_EVENT_EXPLOSION].gain_q15);
    return 0;
}
