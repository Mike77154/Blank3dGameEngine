#include "gshotguneq89.h"

/* 1-pole coefficients for 120, 320, 900, 2400 and 6000 Hz at 44.1 kHz. */
static const gsgeq89_s16 gsgeq89_alpha_q15[5] = {
    555, 1460, 3943, 9490, 18830
};

static const gsgeq89_preset gsgeq89_presets[GSGEQ89_PROFILE_COUNT] = {
    {
        "balanced_shotgun_master",
        { 4300, 4550, 4550, 4450, 4050, 3300 },
        3000, 2100, 29200
    },
    {
        "remington_870_steel",
        { 4550, 4800, 4650, 4250, 3550, 2850 },
        3400, 1800, 29400
    },
    {
        "mossberg_500_590_alloy",
        { 4050, 4250, 4450, 4800, 5050, 4300 },
        2700, 2700, 28200
    },
    {
        "benelli_nova_polymer",
        { 4450, 4700, 4500, 3950, 3250, 2600 },
        2200, 2250, 29600
    },
    {
        "winchester_sxp_rotary",
        { 3950, 4200, 4450, 5050, 4800, 3550 },
        3200, 3500, 28200
    }
};

static gsgeq89_s32 gsgeq89_mul_q15(gsgeq89_s32 a, gsgeq89_s32 b)
{
    return (a * b) / 32768L;
}

static gsgeq89_s32 gsgeq89_mul_q12(gsgeq89_s32 a, gsgeq89_s32 b)
{
    return (a * b) / 4096L;
}

static gsgeq89_s16 gsgeq89_sat16(gsgeq89_s32 x)
{
    if (x > 32767L) return 32767;
    if (x < -32768L) return -32768;
    return (gsgeq89_s16)x;
}

static gsgeq89_s32 gsgeq89_soft_clip(gsgeq89_s32 x)
{
    gsgeq89_s32 sign;
    gsgeq89_s32 a;
    sign = 1L;
    if (x < 0L) {
        sign = -1L;
        a = -x;
    } else {
        a = x;
    }
    if (a > 22528L) a = 22528L + (a - 22528L) / 5L;
    if (a > 32767L) a = 32767L;
    return sign < 0L ? -a : a;
}

int gsgeq89_get_preset(gsgeq89_profile_id profile, gsgeq89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)profile < 0 || profile >= GSGEQ89_PROFILE_COUNT) return 0;
    *out_preset = gsgeq89_presets[(int)profile];
    return 1;
}

const char *gsgeq89_profile_name(gsgeq89_profile_id profile)
{
    if ((int)profile < 0 || profile >= GSGEQ89_PROFILE_COUNT) return "invalid";
    return gsgeq89_presets[(int)profile].name;
}

void gsgeq89_reset(gsgeq89_context *ctx)
{
    int i;
    if (ctx == 0) return;
    for (i = 0; i < 5; ++i) ctx->lowpass[i] = 0L;
    ctx->transient_lp = 0L;
}

void gsgeq89_init(gsgeq89_context *ctx, gsgeq89_profile_id profile)
{
    if (ctx == 0) return;
    if (!gsgeq89_get_preset(profile, &ctx->preset)) {
        gsgeq89_get_preset(GSGEQ89_PROFILE_BALANCED, &ctx->preset);
    }
    gsgeq89_reset(ctx);
}

void gsgeq89_set_preset(gsgeq89_context *ctx, const gsgeq89_preset *preset)
{
    if (ctx == 0 || preset == 0) return;
    ctx->preset = *preset;
    gsgeq89_reset(ctx);
}

gsgeq89_s16 gsgeq89_process_sample(gsgeq89_context *ctx, gsgeq89_s16 input)
{
    gsgeq89_s32 x;
    gsgeq89_s32 bands[GSGEQ89_BANDS];
    gsgeq89_s32 sum;
    gsgeq89_s32 transient;
    gsgeq89_s32 driven;
    int i;

    if (ctx == 0) return input;
    x = (gsgeq89_s32)input;
    for (i = 0; i < 5; ++i) {
        ctx->lowpass[i] += gsgeq89_mul_q15(
            x - ctx->lowpass[i], (gsgeq89_s32)gsgeq89_alpha_q15[i]);
    }

    bands[0] = ctx->lowpass[0];
    bands[1] = ctx->lowpass[1] - ctx->lowpass[0];
    bands[2] = ctx->lowpass[2] - ctx->lowpass[1];
    bands[3] = ctx->lowpass[3] - ctx->lowpass[2];
    bands[4] = ctx->lowpass[4] - ctx->lowpass[3];
    bands[5] = x - ctx->lowpass[4];

    sum = 0L;
    for (i = 0; i < GSGEQ89_BANDS; ++i) {
        sum += gsgeq89_mul_q12(bands[i],
            (gsgeq89_s32)ctx->preset.band_gain_q12[i]);
    }

    /* Preserve the short metal contacts without turning rail noise into hiss. */
    ctx->transient_lp += gsgeq89_mul_q15(x - ctx->transient_lp, 6200L);
    transient = x - ctx->transient_lp;
    sum += gsgeq89_mul_q15(transient,
        (gsgeq89_s32)ctx->preset.transient_q15);

    driven = sum + gsgeq89_mul_q15(sum,
        (gsgeq89_s32)ctx->preset.drive_q15);
    driven = gsgeq89_soft_clip(driven);
    driven = gsgeq89_mul_q15(driven,
        (gsgeq89_s32)ctx->preset.output_q15);
    return gsgeq89_sat16(driven);
}

gsgeq89_u32 gsgeq89_context_bytes(void)
{
    return (gsgeq89_u32)sizeof(gsgeq89_context);
}
