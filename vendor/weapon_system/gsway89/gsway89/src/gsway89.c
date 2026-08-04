#include "../include/gsway89.h"

#define GSW89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static const short gsw89_sin64_q15[64] = {
    0, 3212, 6393, 9512, 12539, 15446, 18205, 20787,
    23170, 25329, 27245, 28898, 30274, 31356, 32138, 32610,
    32767, 32610, 32138, 31356, 30274, 28898, 27245, 25329,
    23170, 20787, 18205, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18205, -20787,
    -23170, -25329, -27245, -28898, -30274, -31356, -32138, -32610,
    -32767, -32610, -32138, -31356, -30274, -28898, -27245, -25329,
    -23170, -20787, -18205, -15446, -12539, -9512, -6393, -3212
};

static long gsw89_advance_phase(long phase_x1000,
                                short cycle_frames,
                                short dt_frames)
{
    long inc;
    if (cycle_frames < 1) cycle_frames = 1;
    inc = (64000L * (long)dt_frames) / (long)cycle_frames;
    if (inc < 1) inc = 1;
    phase_x1000 += inc;
    while (phase_x1000 >= 64000L) phase_x1000 -= 64000L;
    while (phase_x1000 < 0L) phase_x1000 += 64000L;
    return phase_x1000;
}

static short gsw89_sin_phase(long phase_x1000)
{
    short index;
    index = (short)((phase_x1000 / 1000L) & 63L);
    return gsw89_sin64_q15[index];
}

static short gsw89_scale_wave(short amp_x1000, short wave_q15)
{
    return (short)(((long)amp_x1000 * (long)wave_q15) / 32767L);
}

static short gsw89_step_toward(short value, short target,
                               short total_frames, short dt_frames)
{
    short step;
    if (total_frames < 1) total_frames = 1;
    step = (short)((1000L * (long)dt_frames) / (long)total_frames);
    if (step < 1) step = 1;
    if (value < target) {
        value = (short)(value + step);
        if (value > target) value = target;
    } else if (value > target) {
        value = (short)(value - step);
        if (value < target) value = target;
    }
    return value;
}

void gsw89_init(gsw89_ctx *ctx, const gsw89_profile *profile)
{
    if (!ctx) return;
    if (profile) ctx->profile = *profile;
    else ctx->profile = gsw89_profile_sniper_default();
    ctx->hold_requested = 0;
    ctx->movement_pct = 0;
    ctx->stress_pct = 0;
    ctx->hold_blend_x1000 = 0;
    ctx->hold_used_frames = 0;
    ctx->recovery_left_frames = 0;
    ctx->exhausted = 0;
    ctx->hold_remaining_pct = 100;
    ctx->yaw_phase_x1000 = 0;
    ctx->pitch_phase_x1000 = 16000L;
    ctx->breath_phase_x1000 = 8000L;
    ctx->pulse_phase_x1000 = 0;
    ctx->breath_wave_x1000 = 0;
    ctx->pulse_wave_x1000 = 0;
    ctx->yaw_out_deg_x1000 = 0;
    ctx->pitch_out_deg_x1000 = 0;
}

void gsw89_set_profile(gsw89_ctx *ctx, const gsw89_profile *profile)
{
    if (!ctx || !profile) return;
    ctx->profile = *profile;
}

void gsw89_set_hold(gsw89_ctx *ctx, short hold_requested)
{
    if (!ctx) return;
    ctx->hold_requested = hold_requested ? 1 : 0;
}

void gsw89_set_movement_pct(gsw89_ctx *ctx, short movement_pct)
{
    if (!ctx) return;
    ctx->movement_pct = (short)GSW89_CLAMP(movement_pct, 0, 100);
}

void gsw89_set_stress_pct(gsw89_ctx *ctx, short stress_pct)
{
    if (!ctx) return;
    ctx->stress_pct = (short)GSW89_CLAMP(stress_pct, 0, 100);
}

void gsw89_reset_breath(gsw89_ctx *ctx)
{
    gsw89_profile p;
    if (!ctx) return;
    p = ctx->profile;
    gsw89_init(ctx, &p);
}

void gsw89_update(gsw89_ctx *ctx, short dt_frames)
{
    short can_hold;
    short target_hold_blend;
    short recover_step;
    long scale_pct;
    long hold_reduce_pct;
    long effective_pct;
    short yaw_wave;
    short pitch_wave;
    short breath_wave;
    short pulse_wave;
    long yaw_value;
    long pitch_value;
    long breath_scale_pct;

    if (!ctx) return;
    if (dt_frames < 1) dt_frames = 1;

    if (ctx->exhausted) {
        ctx->recovery_left_frames = (short)(ctx->recovery_left_frames - dt_frames);
        if (ctx->recovery_left_frames <= 0) {
            ctx->recovery_left_frames = 0;
            ctx->exhausted = 0;
            ctx->hold_used_frames = 0;
        }
    }

    can_hold = (short)(ctx->hold_requested && !ctx->exhausted);
    if (can_hold) {
        ctx->hold_used_frames = (short)(ctx->hold_used_frames + dt_frames);
        if (ctx->profile.hold_max_frames < 1) ctx->profile.hold_max_frames = 1;
        if (ctx->hold_used_frames >= ctx->profile.hold_max_frames) {
            ctx->hold_used_frames = ctx->profile.hold_max_frames;
            ctx->exhausted = 1;
            ctx->recovery_left_frames = ctx->profile.recovery_frames;
            can_hold = 0;
        }
    } else if (!ctx->exhausted && ctx->hold_used_frames > 0) {
        if (ctx->profile.recovery_frames < 1) ctx->profile.recovery_frames = 1;
        recover_step = (short)(((long)ctx->profile.hold_max_frames *
                                (long)dt_frames) /
                               (long)ctx->profile.recovery_frames);
        if (recover_step < 1) recover_step = 1;
        ctx->hold_used_frames = (short)(ctx->hold_used_frames - recover_step);
        if (ctx->hold_used_frames < 0) ctx->hold_used_frames = 0;
    }

    target_hold_blend = can_hold ? 1000 : 0;
    ctx->hold_blend_x1000 = gsw89_step_toward(
        ctx->hold_blend_x1000,
        target_hold_blend,
        ctx->profile.hold_enter_frames,
        dt_frames);

    if (ctx->profile.hold_max_frames < 1) ctx->profile.hold_max_frames = 1;
    ctx->hold_remaining_pct = (short)(100L -
        ((long)ctx->hold_used_frames * 100L) /
        (long)ctx->profile.hold_max_frames);
    ctx->hold_remaining_pct = (short)GSW89_CLAMP(
        ctx->hold_remaining_pct, 0, 100);

    ctx->yaw_phase_x1000 = gsw89_advance_phase(
        ctx->yaw_phase_x1000,
        ctx->profile.drift_cycle_frames,
        dt_frames);
    ctx->pitch_phase_x1000 = gsw89_advance_phase(
        ctx->pitch_phase_x1000,
        (short)(ctx->profile.drift_cycle_frames + 17),
        dt_frames);
    ctx->breath_phase_x1000 = gsw89_advance_phase(
        ctx->breath_phase_x1000,
        ctx->profile.breath_cycle_frames,
        dt_frames);
    ctx->pulse_phase_x1000 = gsw89_advance_phase(
        ctx->pulse_phase_x1000,
        ctx->profile.pulse_cycle_frames,
        dt_frames);

    yaw_wave = gsw89_sin_phase(ctx->yaw_phase_x1000);
    pitch_wave = gsw89_sin_phase(ctx->pitch_phase_x1000);
    breath_wave = gsw89_sin_phase(ctx->breath_phase_x1000);
    pulse_wave = gsw89_sin_phase(ctx->pulse_phase_x1000);

    scale_pct = 100L;
    scale_pct += ((long)ctx->movement_pct *
                  (long)ctx->profile.movement_gain_pct) / 100L;
    scale_pct += ((long)ctx->stress_pct *
                  (long)ctx->profile.stress_gain_pct) / 100L;

    hold_reduce_pct = ((long)ctx->profile.hold_reduction_pct *
                       (long)ctx->hold_blend_x1000) / 1000L;
    effective_pct = (scale_pct * (100L - hold_reduce_pct)) / 100L;
    breath_scale_pct = 100L -
        ((95L * (long)ctx->hold_blend_x1000) / 1000L);

    ctx->breath_wave_x1000 = (short)(
        ((long)gsw89_scale_wave(ctx->profile.breath_amp_x1000,
                               breath_wave) *
         breath_scale_pct) / 100L);
    ctx->pulse_wave_x1000 = gsw89_scale_wave(
        (short)(ctx->profile.pulse_amp_x1000 +
                ((long)ctx->profile.pulse_amp_x1000 *
                 (long)ctx->stress_pct) / 100L),
        pulse_wave);

    yaw_value = (long)gsw89_scale_wave(ctx->profile.yaw_amp_x1000,
                                      yaw_wave);
    yaw_value = (yaw_value * effective_pct) / 100L;
    yaw_value += (long)ctx->breath_wave_x1000 / 4L;
    yaw_value += (long)ctx->pulse_wave_x1000 / 5L;

    pitch_value = (long)gsw89_scale_wave(ctx->profile.pitch_amp_x1000,
                                        pitch_wave);
    pitch_value = (pitch_value * effective_pct) / 100L;
    pitch_value += (long)ctx->breath_wave_x1000;
    pitch_value += (long)ctx->pulse_wave_x1000;

    ctx->yaw_out_deg_x1000 = (short)GSW89_CLAMP(
        yaw_value, -32767L, 32767L);
    ctx->pitch_out_deg_x1000 = (short)GSW89_CLAMP(
        pitch_value, -32767L, 32767L);
}

gsw89_profile gsw89_profile_sniper_default(void)
{
    gsw89_profile p;
    p.yaw_amp_x1000 = 110;
    p.pitch_amp_x1000 = 90;
    p.drift_cycle_frames = 115;
    p.breath_amp_x1000 = 75;
    p.breath_cycle_frames = 210;
    p.pulse_amp_x1000 = 12;
    p.pulse_cycle_frames = 52;
    p.hold_reduction_pct = 88;
    p.hold_enter_frames = 12;
    p.hold_max_frames = 180;
    p.recovery_frames = 150;
    p.movement_gain_pct = 140;
    p.stress_gain_pct = 70;
    return p;
}
