#include "gguntuberotator89.h"

#define GGTR89_PHASE_ONE 65536U
#define GGTR89_PI2_Q15 205887

static const ggtr89_i16 ggtr89_sine_lut[256] = {
0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,
12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,
23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,
30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,
32767,32757,32728,32678,32609,32521,32412,32285,32137,31971,31785,31580,31356,31113,30852,30571,
30273,29956,29621,29268,28898,28510,28105,27683,27245,26790,26319,25832,25329,24811,24279,23731,
23170,22594,22005,21403,20787,20159,19519,18868,18204,17530,16846,16151,15446,14732,14010,13279,
12539,11793,11039,10278,9512,8739,7962,7179,6393,5602,4808,4011,3212,2410,1608,804,
0,-804,-1608,-2410,-3212,-4011,-4808,-5602,-6393,-7179,-7962,-8739,-9512,-10278,-11039,-11793,
-12539,-13279,-14010,-14732,-15446,-16151,-16846,-17530,-18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,
-23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790,-27245,-27683,-28105,-28510,-28898,-29268,-29621,-29956,
-30273,-30571,-30852,-31113,-31356,-31580,-31785,-31971,-32137,-32285,-32412,-32521,-32609,-32678,-32728,-32757,
-32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,-32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,
-30273,-29956,-29621,-29268,-28898,-28510,-28105,-27683,-27245,-26790,-26319,-25832,-25329,-24811,-24279,-23731,
-23170,-22594,-22005,-21403,-20787,-20159,-19519,-18868,-18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,
-12539,-11793,-11039,-10278,-9512,-8739,-7962,-7179,-6393,-5602,-4808,-4011,-3212,-2410,-1608,-804
};

static ggtr89_i32 ggtr89_clamp_i32(ggtr89_i32 x, ggtr89_i32 lo,
                                    ggtr89_i32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static ggtr89_i16 ggtr89_clamp_i16(ggtr89_i32 x)
{
    if (x < -32768) return (ggtr89_i16)-32768;
    if (x > 32767) return (ggtr89_i16)32767;
    return (ggtr89_i16)x;
}

static ggtr89_i32 ggtr89_mul_q15(ggtr89_i32 a, ggtr89_i32 b)
{
    return (a * b) >> 15;
}

static ggtr89_i32 ggtr89_mul_q12(ggtr89_i32 a, ggtr89_i32 b)
{
    return (a * b) >> 12;
}

static ggtr89_u32 ggtr89_rand(ggtr89_context *ctx)
{
    ggtr89_u32 x;
    x = ctx->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ctx->rng = x;
    return x;
}

static ggtr89_i32 ggtr89_noise(ggtr89_context *ctx)
{
    return (ggtr89_i32)((ggtr89_rand(ctx) >> 16) & 65535U) - 32768;
}

static ggtr89_i32 ggtr89_decay_coeff(ggtr89_i32 sample_rate,
                                      ggtr89_i32 decay_ms)
{
    ggtr89_i32 den;
    ggtr89_i32 drop;
    if (decay_ms < 1) decay_ms = 1;
    den = (sample_rate / 1000) * decay_ms;
    if (den < 1) den = 1;
    drop = 32768 / den;
    if (drop < 1) drop = 1;
    if (drop > 8192) drop = 8192;
    return 32768 - drop;
}

static ggtr89_i32 ggtr89_alpha_for_fc(ggtr89_i32 sample_rate,
                                       ggtr89_i32 fc)
{
    ggtr89_i32 numerator;
    ggtr89_i32 denominator;
    ggtr89_i32 two_pi_fc;
    two_pi_fc = (fc * 710) / 113;
    numerator = fc * GGTR89_PI2_Q15;
    denominator = sample_rate + two_pi_fc;
    if (denominator < 1) denominator = 1;
    return ggtr89_clamp_i32(numerator / denominator, 1, 32767);
}

static void ggtr89_clear_runtime(ggtr89_context *ctx)
{
    ggtr89_i32 i;
    ctx->stage = GGTR89_STAGE_IDLE;
    ctx->stage_frame = 0;
    ctx->stage_total_frames = 1;
    ctx->rpm_q16 = 0;
    ctx->stop_start_rpm_q16 = 0;
    ctx->passage_phase_q16 = 0U;
    ctx->sine_phase_q16 = 0U;
    ctx->env_noise_q15 = 0;
    ctx->env_low_q15 = 0;
    ctx->env_sine_q15 = 0;
    ctx->rng = 0x6D2B79F5U;
    ctx->low_noise_hold = 0;
    ctx->low_noise_lp = 0;
    ctx->low_noise_div = 0;
    ctx->pulse_gain_q15 = 32767;
    for (i = 0; i < 5; ++i) ctx->eq_lp[i] = 0;
    for (i = 0; i < GGTR89_REVERB_CAPACITY; ++i) {
        ctx->reverb_a[i] = 0;
        ctx->reverb_b[i] = 0;
    }
    ctx->reverb_pos_a = 0;
    ctx->reverb_pos_b = 0;
    ctx->reverb_damp = 0;
}

void ggtr89_config_preset(ggtr89_config *cfg, ggtr89_i32 sample_rate,
                          ggtr89_i32 preset)
{
    ggtr89_i32 i;
    if (cfg == 0) return;
    cfg->sample_rate = sample_rate;
    cfg->barrel_count = 6;
    cfg->initial_rpm = 75;
    cfg->target_rpm = 560;
    cfg->start_ms = 620;
    cfg->stop_ms = 900;
    cfg->sine_hz = 65;
    cfg->noise_decay_ms = 18;
    cfg->low_noise_decay_ms = 38;
    cfg->sine_decay_ms = 55;
    cfg->noise_gain_q12 = 2700;
    cfg->low_noise_gain_q12 = 3800;
    cfg->sine_gain_q12 = 2500;
    cfg->pulse_jitter_q15 = 2600;
    cfg->drive_q12 = 4750;
    cfg->distortion_mix_q15 = 4800;
    cfg->reverb_wet_q15 = 2500;
    cfg->reverb_feedback_q15 = 7600;
    cfg->output_gain_q12 = 3300;
    for (i = 0; i < GGTR89_EQ_BANDS; ++i) cfg->eq_gain_q12[i] = 4096;
    cfg->eq_gain_q12[0] = 2400;
    cfg->eq_gain_q12[1] = 4300;
    cfg->eq_gain_q12[2] = 5600;
    cfg->eq_gain_q12[3] = 5200;
    cfg->eq_gain_q12[4] = 2400;
    cfg->eq_gain_q12[5] = 700;

    if (preset == GGTR89_PRESET_LIGHT) {
        cfg->barrel_count = 3;
        cfg->initial_rpm = 100;
        cfg->target_rpm = 720;
        cfg->start_ms = 430;
        cfg->stop_ms = 620;
        cfg->sine_hz = 98;
        cfg->noise_decay_ms = 13;
        cfg->low_noise_decay_ms = 28;
        cfg->sine_decay_ms = 38;
        cfg->noise_gain_q12 = 3300;
        cfg->low_noise_gain_q12 = 2500;
        cfg->sine_gain_q12 = 1800;
        cfg->eq_gain_q12[0] = 1600;
        cfg->eq_gain_q12[1] = 3500;
        cfg->eq_gain_q12[2] = 5200;
        cfg->eq_gain_q12[3] = 5600;
        cfg->eq_gain_q12[4] = 2800;
        cfg->eq_gain_q12[5] = 1000;
        cfg->output_gain_q12 = 3500;
    } else if (preset == GGTR89_PRESET_HEAVY) {
        cfg->barrel_count = 7;
        cfg->initial_rpm = 45;
        cfg->target_rpm = 430;
        cfg->start_ms = 820;
        cfg->stop_ms = 1250;
        cfg->sine_hz = 65;
        cfg->noise_decay_ms = 24;
        cfg->low_noise_decay_ms = 55;
        cfg->sine_decay_ms = 78;
        cfg->noise_gain_q12 = 2200;
        cfg->low_noise_gain_q12 = 4600;
        cfg->sine_gain_q12 = 3400;
        cfg->eq_gain_q12[0] = 3300;
        cfg->eq_gain_q12[1] = 5200;
        cfg->eq_gain_q12[2] = 5900;
        cfg->eq_gain_q12[3] = 4600;
        cfg->eq_gain_q12[4] = 1800;
        cfg->eq_gain_q12[5] = 400;
        cfg->drive_q12 = 5000;
        cfg->distortion_mix_q15 = 5600;
        cfg->output_gain_q12 = 3100;
    }
}

int ggtr89_init(ggtr89_context *ctx, const ggtr89_config *cfg)
{
    static const ggtr89_i32 crossovers[5] = { 90, 220, 520, 1200, 3000 };
    ggtr89_i32 i;
    if (ctx == 0 || cfg == 0) return 0;
    if (cfg->sample_rate < 8000 || cfg->sample_rate > 48000) return 0;
    if (cfg->barrel_count < 1 || cfg->barrel_count > 12) return 0;
    ctx->cfg = *cfg;
    ctx->decay_noise_q15 = ggtr89_decay_coeff(cfg->sample_rate,
                                               cfg->noise_decay_ms);
    ctx->decay_low_q15 = ggtr89_decay_coeff(cfg->sample_rate,
                                             cfg->low_noise_decay_ms);
    ctx->decay_sine_q15 = ggtr89_decay_coeff(cfg->sample_rate,
                                              cfg->sine_decay_ms);
    for (i = 0; i < 5; ++i) {
        ctx->eq_alpha_q15[i] = ggtr89_alpha_for_fc(cfg->sample_rate,
                                                   crossovers[i]);
    }
    ctx->reverb_len_a = (997 * cfg->sample_rate) / 44100;
    ctx->reverb_len_b = (1499 * cfg->sample_rate) / 44100;
    ctx->reverb_len_a = ggtr89_clamp_i32(ctx->reverb_len_a, 97,
                                         GGTR89_REVERB_CAPACITY - 1);
    ctx->reverb_len_b = ggtr89_clamp_i32(ctx->reverb_len_b, 131,
                                         GGTR89_REVERB_CAPACITY - 1);
    ggtr89_clear_runtime(ctx);
    return 1;
}

void ggtr89_reset(ggtr89_context *ctx)
{
    if (ctx == 0) return;
    ggtr89_clear_runtime(ctx);
}

void ggtr89_start(ggtr89_context *ctx)
{
    if (ctx == 0) return;
    ctx->stage = GGTR89_STAGE_START;
    ctx->stage_frame = 0;
    ctx->stage_total_frames = (ctx->cfg.start_ms * ctx->cfg.sample_rate) / 1000;
    if (ctx->stage_total_frames < 1) ctx->stage_total_frames = 1;
    ctx->rpm_q16 = ctx->cfg.initial_rpm << 16;
    ctx->passage_phase_q16 = 0U;
    ctx->env_noise_q15 = 22000;
    ctx->env_low_q15 = 18000;
    ctx->env_sine_q15 = 16000;
}

void ggtr89_stop(ggtr89_context *ctx)
{
    if (ctx == 0) return;
    if (ctx->stage == GGTR89_STAGE_IDLE || ctx->stage == GGTR89_STAGE_STOP) return;
    ctx->stage = GGTR89_STAGE_STOP;
    ctx->stage_frame = 0;
    ctx->stage_total_frames = (ctx->cfg.stop_ms * ctx->cfg.sample_rate) / 1000;
    if (ctx->stage_total_frames < 1) ctx->stage_total_frames = 1;
    ctx->stop_start_rpm_q16 = ctx->rpm_q16;
}

void ggtr89_force_loop(ggtr89_context *ctx)
{
    if (ctx == 0) return;
    ctx->stage = GGTR89_STAGE_LOOP;
    ctx->stage_frame = 0;
    ctx->stage_total_frames = 1;
    ctx->rpm_q16 = ctx->cfg.target_rpm << 16;
}

void ggtr89_set_eq_gain(ggtr89_context *ctx, ggtr89_i32 band,
                        ggtr89_i32 gain_q12)
{
    if (ctx == 0) return;
    if (band < 0 || band >= GGTR89_EQ_BANDS) return;
    ctx->cfg.eq_gain_q12[band] = ggtr89_clamp_i32(gain_q12, 0, 8192);
}

static void ggtr89_update_stage(ggtr89_context *ctx)
{
    ggtr89_i32 start_q16;
    ggtr89_i32 target_q16;
    ggtr89_i32 remain;
    start_q16 = ctx->cfg.initial_rpm << 16;
    target_q16 = ctx->cfg.target_rpm << 16;

    if (ctx->stage == GGTR89_STAGE_START) {
        remain = ctx->stage_total_frames;
        ctx->rpm_q16 = start_q16 +
            (ggtr89_i32)(((target_q16 - start_q16) / remain) * ctx->stage_frame);
        ctx->stage_frame += 1;
        if (ctx->stage_frame >= ctx->stage_total_frames) {
            ctx->stage = GGTR89_STAGE_LOOP;
            ctx->stage_frame = 0;
            ctx->rpm_q16 = target_q16;
        }
    } else if (ctx->stage == GGTR89_STAGE_LOOP) {
        ctx->rpm_q16 = target_q16;
    } else if (ctx->stage == GGTR89_STAGE_STOP) {
        remain = ctx->stage_total_frames;
        ctx->rpm_q16 = ctx->stop_start_rpm_q16 -
            (ggtr89_i32)((ctx->stop_start_rpm_q16 / remain) * ctx->stage_frame);
        if (ctx->rpm_q16 < 0) ctx->rpm_q16 = 0;
        ctx->stage_frame += 1;
        if (ctx->stage_frame >= ctx->stage_total_frames) {
            ctx->stage = GGTR89_STAGE_IDLE;
            ctx->stage_frame = 0;
            ctx->rpm_q16 = 0;
        }
    }
}

static void ggtr89_trigger_pulse(ggtr89_context *ctx)
{
    ggtr89_i32 jitter;
    ggtr89_i32 amp;
    jitter = (ggtr89_i32)((ggtr89_rand(ctx) >> 17) & 32767U) - 16384;
    jitter = ggtr89_mul_q15(jitter, ctx->cfg.pulse_jitter_q15);
    amp = 30000 + jitter;
    amp = ggtr89_clamp_i32(amp, 18000, 32767);
    ctx->pulse_gain_q15 = amp;
    ctx->env_noise_q15 = 32767;
    ctx->env_low_q15 = 30000;
    ctx->env_sine_q15 = 25000;
}

static ggtr89_i32 ggtr89_eq6(ggtr89_context *ctx, ggtr89_i32 x)
{
    ggtr89_i32 b[GGTR89_EQ_BANDS];
    ggtr89_i32 y;
    ggtr89_i32 i;
    for (i = 0; i < 5; ++i) {
        ctx->eq_lp[i] += ggtr89_mul_q15(x - ctx->eq_lp[i],
                                        ctx->eq_alpha_q15[i]);
    }
    b[0] = ctx->eq_lp[0];
    b[1] = ctx->eq_lp[1] - ctx->eq_lp[0];
    b[2] = ctx->eq_lp[2] - ctx->eq_lp[1];
    b[3] = ctx->eq_lp[3] - ctx->eq_lp[2];
    b[4] = ctx->eq_lp[4] - ctx->eq_lp[3];
    b[5] = x - ctx->eq_lp[4];
    y = 0;
    for (i = 0; i < GGTR89_EQ_BANDS; ++i) {
        y += ggtr89_mul_q12(b[i], ctx->cfg.eq_gain_q12[i]);
    }
    return ggtr89_clamp_i32(y, -65536, 65535);
}

static ggtr89_i32 ggtr89_distort(ggtr89_context *ctx, ggtr89_i32 x)
{
    ggtr89_i32 driven;
    ggtr89_i32 a;
    ggtr89_i32 shaped;
    ggtr89_i32 wet;
    ggtr89_i32 dry;
    driven = ggtr89_mul_q12(x, ctx->cfg.drive_q12);
    a = driven < 0 ? -driven : driven;
    if (a <= 18000) {
        shaped = driven;
    } else {
        a = 18000 + ((a - 18000) >> 2);
        if (a > 32767) a = 32767;
        shaped = driven < 0 ? -a : a;
    }
    wet = ggtr89_mul_q15(shaped, ctx->cfg.distortion_mix_q15);
    dry = ggtr89_mul_q15(x, 32767 - ctx->cfg.distortion_mix_q15);
    return ggtr89_clamp_i32(wet + dry, -32768, 32767);
}

static ggtr89_i32 ggtr89_reverb(ggtr89_context *ctx, ggtr89_i32 x)
{
    ggtr89_i32 da;
    ggtr89_i32 db;
    ggtr89_i32 taps;
    ggtr89_i32 feedback;
    ggtr89_i32 stored;
    ggtr89_i32 wet;
    ggtr89_i32 dry;

    da = ctx->reverb_a[ctx->reverb_pos_a];
    db = ctx->reverb_b[ctx->reverb_pos_b];
    taps = (da + db) >> 1;
    ctx->reverb_damp += ggtr89_mul_q15(taps - ctx->reverb_damp, 6000);
    feedback = ggtr89_mul_q15(ctx->reverb_damp,
                              ctx->cfg.reverb_feedback_q15);
    stored = ggtr89_clamp_i32(x + feedback, -32768, 32767);
    ctx->reverb_a[ctx->reverb_pos_a] = (ggtr89_i16)stored;
    ctx->reverb_b[ctx->reverb_pos_b] = (ggtr89_i16)
        ggtr89_clamp_i32(x + (feedback >> 1), -32768, 32767);

    ctx->reverb_pos_a += 1;
    ctx->reverb_pos_b += 1;
    if (ctx->reverb_pos_a >= ctx->reverb_len_a) ctx->reverb_pos_a = 0;
    if (ctx->reverb_pos_b >= ctx->reverb_len_b) ctx->reverb_pos_b = 0;

    wet = ggtr89_mul_q15(taps, ctx->cfg.reverb_wet_q15);
    dry = ggtr89_mul_q15(x, 32767 - ctx->cfg.reverb_wet_q15);
    return ggtr89_clamp_i32(dry + wet, -32768, 32767);
}

static ggtr89_i32 ggtr89_render_one(ggtr89_context *ctx)
{
    ggtr89_i32 rpm;
    ggtr89_i32 pulse_hz_q16;
    ggtr89_u32 passage_inc;
    ggtr89_u32 sine_inc;
    ggtr89_i32 n1;
    ggtr89_i32 n2;
    ggtr89_i32 s;
    ggtr89_i32 x;
    ggtr89_i32 master_q15;

    ggtr89_update_stage(ctx);
    rpm = ctx->rpm_q16 >> 16;
    pulse_hz_q16 = (rpm * ctx->cfg.barrel_count * 65536) / 60;
    passage_inc = (ggtr89_u32)(pulse_hz_q16 / ctx->cfg.sample_rate);
    ctx->passage_phase_q16 += passage_inc;
    while (ctx->passage_phase_q16 >= GGTR89_PHASE_ONE) {
        ctx->passage_phase_q16 -= GGTR89_PHASE_ONE;
        ggtr89_trigger_pulse(ctx);
    }

    n1 = ggtr89_noise(ctx);
    ctx->low_noise_div += 1;
    if (ctx->low_noise_div >= 2) {
        ctx->low_noise_div = 0;
        ctx->low_noise_hold = ggtr89_noise(ctx);
    }
    ctx->low_noise_lp += ggtr89_mul_q15(ctx->low_noise_hold -
                                        ctx->low_noise_lp, 6200);
    n2 = ctx->low_noise_lp;

    sine_inc = (ggtr89_u32)((ctx->cfg.sine_hz * 65536) /
                             ctx->cfg.sample_rate);
    ctx->sine_phase_q16 += sine_inc;
    ctx->sine_phase_q16 &= 65535U;
    s = ggtr89_sine_lut[(ctx->sine_phase_q16 >> 8) & 255U];

    n1 = ggtr89_mul_q15(n1, ctx->env_noise_q15);
    n1 = ggtr89_mul_q12(n1, ctx->cfg.noise_gain_q12);
    n2 = ggtr89_mul_q15(n2, ctx->env_low_q15);
    n2 = ggtr89_mul_q12(n2, ctx->cfg.low_noise_gain_q12);
    s = ggtr89_mul_q15(s, ctx->env_sine_q15);
    s = ggtr89_mul_q12(s, ctx->cfg.sine_gain_q12);

    x = n1 + n2 + s;
    x = ggtr89_mul_q15(x, ctx->pulse_gain_q15);

    if (ctx->stage == GGTR89_STAGE_STOP && ctx->stage_total_frames > 0) {
        master_q15 = 32767 -
            ((ctx->stage_frame * 24576) / ctx->stage_total_frames);
        master_q15 = ggtr89_clamp_i32(master_q15, 8191, 32767);
        x = ggtr89_mul_q15(x, master_q15);
    }

    ctx->env_noise_q15 = ggtr89_mul_q15(ctx->env_noise_q15,
                                        ctx->decay_noise_q15);
    ctx->env_low_q15 = ggtr89_mul_q15(ctx->env_low_q15,
                                      ctx->decay_low_q15);
    ctx->env_sine_q15 = ggtr89_mul_q15(ctx->env_sine_q15,
                                       ctx->decay_sine_q15);

    x = ggtr89_eq6(ctx, x);
    x = ggtr89_distort(ctx, x);
    x = ggtr89_reverb(ctx, x);
    x = ggtr89_mul_q12(x, ctx->cfg.output_gain_q12);
    return ggtr89_clamp_i32(x, -32768, 32767);
}

void ggtr89_render(ggtr89_context *ctx, ggtr89_i16 *output,
                   ggtr89_i32 frame_count)
{
    ggtr89_i32 i;
    if (ctx == 0 || output == 0 || frame_count <= 0) return;
    for (i = 0; i < frame_count; ++i) {
        output[i] = ggtr89_clamp_i16(ggtr89_render_one(ctx));
    }
}

int ggtr89_is_active(const ggtr89_context *ctx)
{
    if (ctx == 0) return 0;
    if (ctx->stage != GGTR89_STAGE_IDLE) return 1;
    if (ctx->env_noise_q15 > 8 || ctx->env_low_q15 > 8 ||
        ctx->env_sine_q15 > 8) return 1;
    if (ctx->reverb_damp > 8 || ctx->reverb_damp < -8) return 1;
    return 0;
}

int ggtr89_get_stage(const ggtr89_context *ctx)
{
    if (ctx == 0) return GGTR89_STAGE_IDLE;
    return ctx->stage;
}
