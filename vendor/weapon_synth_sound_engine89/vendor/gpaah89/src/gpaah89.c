#include "gpaah89.h"

#define GPAAH89_ENV_OFF 0
#define GPAAH89_ENV_DECAY 1
#define GPAAH89_ENV_SUSTAIN 2
#define GPAAH89_ENV_RELEASE 3

static gpaah89_s32 gpaah89_clamp_s32(gpaah89_s32 v,
                                     gpaah89_s32 lo,
                                     gpaah89_s32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static gpaah89_s16 gpaah89_clamp_s16(gpaah89_s32 v)
{
    if (v < -32768) return (gpaah89_s16)-32768;
    if (v > 32767) return (gpaah89_s16)32767;
    return (gpaah89_s16)v;
}

static gpaah89_s32 gpaah89_mul_q15(gpaah89_s32 a, gpaah89_s32 b)
{
    a = gpaah89_clamp_s32(a, -65535, 65535);
    b = gpaah89_clamp_s32(b, -32767, 32767);
    return (a * b) >> 15;
}

static gpaah89_s32 gpaah89_mul_q14(gpaah89_s32 a, gpaah89_s32 b)
{
    a = gpaah89_clamp_s32(a, -65535, 65535);
    b = gpaah89_clamp_s32(b, -32767, 32767);
    return (a * b) >> 14;
}

static gpaah89_u32 gpaah89_xorshift32(gpaah89_u32 *s)
{
    gpaah89_u32 x;
    x = *s;
    if (x == 0U) x = 0x6d2b79f5U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static gpaah89_s32 gpaah89_noise_next(gpaah89_noise_osc *osc)
{
    gpaah89_u32 r;
    gpaah89_s32 n;

    if (osc->counter == 0U) {
        r = gpaah89_xorshift32(&osc->rng);
        n = (gpaah89_s32)((r >> 16) & 65535U) - 32768;
        osc->target = n;
        osc->counter = osc->period;
    }
    osc->counter--;
    osc->value += (osc->target - osc->value) >> osc->smooth_shift;
    return osc->value;
}

/* Integer approximation for a non-resonant one-pole low-pass coefficient. */
static gpaah89_s16 gpaah89_alpha_from_hz(gpaah89_u32 sample_rate,
                                         gpaah89_u16 hz)
{
    gpaah89_u32 denom;
    gpaah89_u32 numer;
    gpaah89_u32 a;

    if (hz == 0U) return 0;
    denom = (gpaah89_u32)hz + (sample_rate / 6U);
    numer = (gpaah89_u32)hz * 32767U;
    if (denom == 0U) return 0;
    a = numer / denom;
    if (a > 32767U) a = 32767U;
    return (gpaah89_s16)a;
}

static gpaah89_s32 gpaah89_env_next(gpaah89_envelope_state *e,
                                    const gpaah89_noise_envelope *p)
{
    gpaah89_s32 start;
    gpaah89_s32 end;
    gpaah89_s32 diff;
    gpaah89_u32 total;
    gpaah89_u32 remain;

    if (e->stage == GPAAH89_ENV_OFF) return 0;

    if (e->stage == GPAAH89_ENV_DECAY) {
        total = e->decay_samples;
        if (total == 0U) total = 1U;
        remain = total - e->pos;
        start = 32767;
        end = p->sustain_q15;
        diff = start - end;
        e->level_q15 = end +
            (gpaah89_s32)((diff * (gpaah89_s32)remain) /
                          (gpaah89_s32)total);
        e->pos++;
        if (e->pos >= total) {
            e->stage = GPAAH89_ENV_SUSTAIN;
            e->pos = 0U;
            e->level_q15 = end;
        }
    } else if (e->stage == GPAAH89_ENV_SUSTAIN) {
        e->level_q15 = p->sustain_q15;
        e->pos++;
        if (e->pos >= e->sustain_samples) {
            e->stage = GPAAH89_ENV_RELEASE;
            e->pos = 0U;
        }
    } else if (e->stage == GPAAH89_ENV_RELEASE) {
        total = e->release_samples;
        if (total == 0U) total = 1U;
        remain = total - e->pos;
        start = p->sustain_q15;
        e->level_q15 =
            (gpaah89_s32)((start * (gpaah89_s32)remain) /
                          (gpaah89_s32)total);
        e->pos++;
        if (e->pos >= total) {
            e->stage = GPAAH89_ENV_OFF;
            e->pos = 0U;
            e->level_q15 = 0;
        }
    }

    return e->level_q15;
}

static int gpaah89_any_envelope_active(const gpaah89_state *s)
{
    int i;
    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i) {
        if (s->env[i].stage != GPAAH89_ENV_OFF) return 1;
    }
    return 0;
}

static gpaah89_s32 gpaah89_eq_process(gpaah89_state *s, gpaah89_s32 x)
{
    gpaah89_s32 sum;
    gpaah89_s32 band;
    gpaah89_s32 delta;
    int i;

    x = gpaah89_clamp_s32(x, -65535, 65535);
    sum = 0;
    for (i = 0; i < GPAAH89_EQ_BANDS; ++i) {
        delta = x - s->eq_state[i].high_lp;
        delta = gpaah89_clamp_s32(delta, -65535, 65535);
        s->eq_state[i].high_lp +=
            gpaah89_mul_q15(delta, s->eq_state[i].high_alpha_q15);

        delta = x - s->eq_state[i].low_lp;
        delta = gpaah89_clamp_s32(delta, -65535, 65535);
        s->eq_state[i].low_lp +=
            gpaah89_mul_q15(delta, s->eq_state[i].low_alpha_q15);

        band = s->eq_state[i].high_lp - s->eq_state[i].low_lp;
        band = gpaah89_clamp_s32(band, -65535, 65535);
        sum += gpaah89_mul_q14(band, s->preset.eq[i].gain_q14);
        sum = gpaah89_clamp_s32(sum, -131071, 131071);
    }
    return sum;
}

static gpaah89_s32 gpaah89_soft_clip(gpaah89_s32 x)
{
    gpaah89_s32 a;
    gpaah89_s32 y;
    int neg;

    x = gpaah89_clamp_s32(x, -131071, 131071);
    neg = 0;
    if (x < 0) {
        neg = 1;
        a = -x;
    } else {
        a = x;
    }

    if (a <= 8192) {
        y = a;
    } else if (a <= 24576) {
        y = 8192 + ((a - 8192) >> 1);
    } else if (a <= 65535) {
        y = 16384 + ((a - 24576) >> 3);
    } else {
        y = 21504 + ((a - 65535) >> 5);
    }
    if (y > 32767) y = 32767;
    return neg ? -y : y;
}

static gpaah89_s32 gpaah89_distort(gpaah89_state *s,
                                    gpaah89_s32 dry,
                                    gpaah89_s32 env_peak_q15)
{
    gpaah89_s32 drive_start;
    gpaah89_s32 drive_end;
    gpaah89_s32 drive;
    gpaah89_s32 driven;
    gpaah89_s32 wet;
    gpaah89_s32 mix;

    drive_start = s->preset.distortion_drive_start_q8;
    drive_end = s->preset.distortion_drive_end_q8;
    drive = drive_end +
        ((drive_start - drive_end) * env_peak_q15 >> 15);
    driven = (gpaah89_clamp_s32(dry, -65535, 65535) * drive) >> 8;
    wet = gpaah89_soft_clip(driven);
    mix = s->preset.distortion_mix_q15;
    return gpaah89_mul_q15(dry, 32767 - mix) +
           gpaah89_mul_q15(wet, mix);
}

static gpaah89_s32 gpaah89_triangle_q15(gpaah89_u32 phase)
{
    gpaah89_u32 x;
    gpaah89_s32 t;

    x = phase >> 16;
    if (x < 32768U) {
        t = (gpaah89_s32)(x << 1) - 32768;
    } else {
        t = 32767 - (gpaah89_s32)((x - 32768U) << 1);
    }
    return t;
}

static void gpaah89_chorus(gpaah89_state *s,
                           gpaah89_s32 dry,
                           gpaah89_s32 env_peak_q15,
                           gpaah89_s32 *left,
                           gpaah89_s32 *right)
{
    gpaah89_u32 left_samples;
    gpaah89_u32 right_samples;
    gpaah89_u32 depth_samples;
    gpaah89_s32 tri;
    gpaah89_s32 mod;
    gpaah89_s32 delay_l;
    gpaah89_s32 delay_r;
    gpaah89_u16 read_l;
    gpaah89_u16 read_r;
    gpaah89_s32 wet_l;
    gpaah89_s32 wet_r;
    gpaah89_s32 mix_start;
    gpaah89_s32 mix_end;
    gpaah89_s32 mix;

    left_samples =
        ((gpaah89_u32)s->preset.chorus_left_ms_x10 * s->sample_rate) /
        10000U;
    right_samples =
        ((gpaah89_u32)s->preset.chorus_right_ms_x10 * s->sample_rate) /
        10000U;
    depth_samples =
        ((gpaah89_u32)s->preset.chorus_depth_ms_x10 * s->sample_rate) /
        10000U;

    if (left_samples < 1U) left_samples = 1U;
    if (right_samples < 1U) right_samples = 1U;
    if (left_samples >= GPAAH89_CHORUS_BUFFER)
        left_samples = GPAAH89_CHORUS_BUFFER - 1U;
    if (right_samples >= GPAAH89_CHORUS_BUFFER)
        right_samples = GPAAH89_CHORUS_BUFFER - 1U;

    tri = gpaah89_triangle_q15(s->chorus_phase);
    s->chorus_phase += s->chorus_phase_inc;
    mod = (tri * (gpaah89_s32)depth_samples) >> 15;

    delay_l = (gpaah89_s32)left_samples + mod;
    delay_r = (gpaah89_s32)right_samples - mod;
    if (delay_l < 1) delay_l = 1;
    if (delay_r < 1) delay_r = 1;
    if (delay_l >= GPAAH89_CHORUS_BUFFER)
        delay_l = GPAAH89_CHORUS_BUFFER - 1;
    if (delay_r >= GPAAH89_CHORUS_BUFFER)
        delay_r = GPAAH89_CHORUS_BUFFER - 1;

    read_l = (gpaah89_u16)((s->chorus_write + GPAAH89_CHORUS_BUFFER -
                            (gpaah89_u16)delay_l) %
                           GPAAH89_CHORUS_BUFFER);
    read_r = (gpaah89_u16)((s->chorus_write + GPAAH89_CHORUS_BUFFER -
                            (gpaah89_u16)delay_r) %
                           GPAAH89_CHORUS_BUFFER);
    wet_l = s->chorus_buffer[read_l];
    wet_r = s->chorus_buffer[read_r];

    s->chorus_buffer[s->chorus_write] = gpaah89_clamp_s16(dry);
    s->chorus_write++;
    if (s->chorus_write >= GPAAH89_CHORUS_BUFFER) s->chorus_write = 0U;

    mix_start = s->preset.chorus_mix_start_q15;
    mix_end = s->preset.chorus_mix_end_q15;
    mix = mix_end + ((mix_start - mix_end) * env_peak_q15 >> 15);
    mix = gpaah89_clamp_s32(mix, 0, 32767);

    *left = gpaah89_mul_q15(dry, 32767 - mix) +
            gpaah89_mul_q15(wet_l, mix);
    *right = gpaah89_mul_q15(dry, 32767 - mix) +
             gpaah89_mul_q15(wet_r, mix);
}

static gpaah89_u16 gpaah89_reverb_index(gpaah89_state *s,
                                        gpaah89_u16 delay_ms)
{
    gpaah89_u32 delay_samples;

    delay_samples = ((gpaah89_u32)delay_ms * s->sample_rate) / 1000U;
    if (delay_samples < 1U) delay_samples = 1U;
    if (delay_samples >= GPAAH89_REVERB_BUFFER)
        delay_samples = GPAAH89_REVERB_BUFFER - 1U;
    return (gpaah89_u16)((s->reverb_write + GPAAH89_REVERB_BUFFER -
                          (gpaah89_u16)delay_samples) %
                         GPAAH89_REVERB_BUFFER);
}

static void gpaah89_reverb(gpaah89_state *s,
                           gpaah89_s32 in_l,
                           gpaah89_s32 in_r,
                           gpaah89_s32 *out_l,
                           gpaah89_s32 *out_r)
{
    gpaah89_s32 wet_l;
    gpaah89_s32 wet_r;
    gpaah89_s32 tap;
    gpaah89_s32 weighted;
    gpaah89_s32 mix;
    gpaah89_u16 index;
    int i;

    wet_l = 0;
    wet_r = 0;
    for (i = 0; i < GPAAH89_REVERB_TAPS; ++i) {
        index = gpaah89_reverb_index(s, s->preset.reverb[i].delay_ms);
        tap = s->reverb_buffer[index];
        weighted = gpaah89_mul_q15(tap,
                                    s->preset.reverb[i].gain_q15);
        if ((i & 1) == 0) {
            wet_l += weighted;
            wet_r += weighted >> 1;
        } else {
            wet_l += weighted >> 1;
            wet_r += weighted;
        }
        wet_l = gpaah89_clamp_s32(wet_l, -65535, 65535);
        wet_r = gpaah89_clamp_s32(wet_r, -65535, 65535);
    }

    s->reverb_buffer[s->reverb_write] =
        gpaah89_clamp_s16((in_l + in_r) >> 1);
    s->reverb_write++;
    if (s->reverb_write >= GPAAH89_REVERB_BUFFER) s->reverb_write = 0U;

    mix = s->preset.reverb_mix_q15;
    *out_l = gpaah89_mul_q15(in_l, 32767 - mix) +
             gpaah89_mul_q15(wet_l, mix);
    *out_r = gpaah89_mul_q15(in_r, 32767 - mix) +
             gpaah89_mul_q15(wet_r, mix);
}

static void gpaah89_clear_state_buffers(gpaah89_state *s)
{
    int i;

    for (i = 0; i < GPAAH89_CHORUS_BUFFER; ++i)
        s->chorus_buffer[i] = 0;
    for (i = 0; i < GPAAH89_REVERB_BUFFER; ++i)
        s->reverb_buffer[i] = 0;
    for (i = 0; i < GPAAH89_EQ_BANDS; ++i) {
        s->eq_state[i].low_lp = 0;
        s->eq_state[i].high_lp = 0;
    }
}

static void gpaah89_set_eq(gpaah89_preset *p,
                           int i,
                           gpaah89_u16 low_hz,
                           gpaah89_u16 high_hz,
                           gpaah89_s16 gain_q14)
{
    p->eq[i].low_hz = low_hz;
    p->eq[i].high_hz = high_hz;
    p->eq[i].gain_q14 = gain_q14;
}

static void gpaah89_set_env(gpaah89_preset *p,
                            int i,
                            gpaah89_u16 decay_ms,
                            gpaah89_u16 sustain_ms,
                            gpaah89_u16 release_ms,
                            gpaah89_s16 sustain_q15)
{
    p->envelope[i].decay_ms = decay_ms;
    p->envelope[i].sustain_ms = sustain_ms;
    p->envelope[i].release_ms = release_ms;
    p->envelope[i].sustain_q15 = sustain_q15;
}

static void gpaah89_set_tap(gpaah89_preset *p,
                            int i,
                            gpaah89_u16 delay_ms,
                            gpaah89_s16 gain_q15)
{
    p->reverb[i].delay_ms = delay_ms;
    p->reverb[i].gain_q15 = gain_q15;
}

static void gpaah89_common_preset(gpaah89_preset *p)
{
    p->noise_mix_q15[GPAAH89_NOISE_LOW] = 25000;
    p->noise_mix_q15[GPAAH89_NOISE_MID] = 32767;
    p->noise_mix_q15[GPAAH89_NOISE_HIGH] = 28000;

    p->transient_correlation_ms_x10 = 0U;
    p->transient_correlation_mix_q15 = 0;

    gpaah89_set_env(p, GPAAH89_NOISE_LOW, 145, 20, 220, 4259);
    gpaah89_set_env(p, GPAAH89_NOISE_MID, 52, 10, 86, 2294);
    gpaah89_set_env(p, GPAAH89_NOISE_HIGH, 8, 0, 14, 0);

    gpaah89_set_eq(p, 0, 35, 100, 19333);
    gpaah89_set_eq(p, 1, 100, 260, 20972);
    gpaah89_set_eq(p, 2, 260, 700, 18350);
    gpaah89_set_eq(p, 3, 700, 1800, 17695);
    gpaah89_set_eq(p, 4, 1800, 4500, 19988);
    gpaah89_set_eq(p, 5, 4500, 11000, 18022);

    p->distortion_drive_start_q8 = 614;
    p->distortion_drive_end_q8 = 294;
    p->distortion_mix_q15 = 13762;

    p->chorus_left_ms_x10 = 17;
    p->chorus_right_ms_x10 = 34;
    p->chorus_depth_ms_x10 = 5;
    p->chorus_rate_millihz = 550;
    p->chorus_mix_start_q15 = 5898;
    p->chorus_mix_end_q15 = 1966;

    gpaah89_set_tap(p, 0, 11, 7864);
    gpaah89_set_tap(p, 1, 17, 5898);
    gpaah89_set_tap(p, 2, 29, 4260);
    gpaah89_set_tap(p, 3, 43, 2949);
    gpaah89_set_tap(p, 4, 67, 1966);
    gpaah89_set_tap(p, 5, 91, 983);
    p->reverb_mix_q15 = 4915;

    p->output_gain_q15 = 22000;
}

int gpaah89_platform_ok(void)
{
    if (sizeof(gpaah89_s16) != 2U) return 0;
    if (sizeof(gpaah89_s32) != 4U) return 0;
    if (sizeof(gpaah89_u32) != 4U) return 0;
    return 1;
}

int gpaah89_get_preset(gpaah89_preset_id id, gpaah89_preset *p)
{
    if (p == 0) return 0;
    gpaah89_common_preset(p);

    switch (id) {
    case GPAAH89_PRESET_PISTOL:
        /* BASE: lower, wider and less dry while keeping a hard front. */
        p->noise_mix_q15[0] = 27500;
        p->noise_mix_q15[1] = 32767;
        p->noise_mix_q15[2] = 27000;
        p->transient_correlation_ms_x10 = 28U;
        p->transient_correlation_mix_q15 = 18500;
        gpaah89_set_env(p, 0, 145, 16, 240, 3000);
        gpaah89_set_env(p, 1, 62, 10, 115, 1900);
        gpaah89_set_env(p, 2, 7, 0, 13, 0);
        p->eq[0].gain_q14 = 20500;
        p->eq[1].gain_q14 = 24500;
        p->eq[2].gain_q14 = 23800;
        p->eq[3].gain_q14 = 22000;
        p->eq[4].gain_q14 = 20500;
        p->eq[5].gain_q14 = 15000;
        p->distortion_drive_start_q8 = 700;
        p->distortion_drive_end_q8 = 305;
        p->distortion_mix_q15 = 14500;
        p->chorus_left_ms_x10 = 18;
        p->chorus_right_ms_x10 = 38;
        p->chorus_depth_ms_x10 = 7;
        p->chorus_mix_start_q15 = 6800;
        p->chorus_mix_end_q15 = 2600;
        gpaah89_set_tap(p, 0, 13, 8200);
        gpaah89_set_tap(p, 1, 23, 6200);
        gpaah89_set_tap(p, 2, 37, 4400);
        gpaah89_set_tap(p, 3, 58, 3000);
        gpaah89_set_tap(p, 4, 86, 1800);
        gpaah89_set_tap(p, 5, 118, 900);
        p->reverb_mix_q15 = 5200;
        p->output_gain_q15 = 23000;
        break;

    case GPAAH89_PRESET_MAGNUM:
        p->noise_mix_q15[0] = 31000;
        p->noise_mix_q15[1] = 32767;
        p->noise_mix_q15[2] = 31500;
        p->transient_correlation_ms_x10 = 32U;
        p->transient_correlation_mix_q15 = 21500;
        gpaah89_set_env(p, 0, 175, 18, 305, 4800);
        gpaah89_set_env(p, 1, 70, 10, 125, 2700);
        gpaah89_set_env(p, 2, 7, 0, 15, 0);
        p->eq[0].gain_q14 = 22500;
        p->eq[1].gain_q14 = 25500;
        p->eq[2].gain_q14 = 22500;
        p->eq[3].gain_q14 = 23800;
        p->eq[4].gain_q14 = 26000;
        p->eq[5].gain_q14 = 20000;
        p->distortion_drive_start_q8 = 845;
        p->distortion_drive_end_q8 = 340;
        p->distortion_mix_q15 = 17039;
        p->chorus_mix_start_q15 = 7209;
        p->chorus_mix_end_q15 = 1802;
        p->reverb_mix_q15 = 4915;
        p->output_gain_q15 = 19500;
        break;

    case GPAAH89_PRESET_SHOTGUN:
        /* BASE v5: deeper blast body with less upper-band dryness. */
        p->noise_mix_q15[0] = 32767;
        p->noise_mix_q15[1] = 31000;
        p->noise_mix_q15[2] = 21000;
        p->transient_correlation_ms_x10 = 30U;
        p->transient_correlation_mix_q15 = 18000;
        gpaah89_set_env(p, 0, 230, 35, 390, 6500);
        gpaah89_set_env(p, 1, 95, 18, 175, 3200);
        gpaah89_set_env(p, 2, 10, 0, 18, 0);
        gpaah89_set_eq(p, 0, 25, 85, 28000);
        gpaah89_set_eq(p, 1, 85, 220, 28000);
        gpaah89_set_eq(p, 2, 220, 600, 22500);
        gpaah89_set_eq(p, 3, 600, 1600, 17500);
        gpaah89_set_eq(p, 4, 1600, 4200, 13500);
        gpaah89_set_eq(p, 5, 4200, 9500, 9000);
        p->distortion_drive_start_q8 = 720;
        p->distortion_drive_end_q8 = 320;
        p->distortion_mix_q15 = 15500;
        p->chorus_depth_ms_x10 = 9;
        p->chorus_mix_start_q15 = 7000;
        p->chorus_mix_end_q15 = 2600;
        gpaah89_set_tap(p, 0, 14, 8500);
        gpaah89_set_tap(p, 1, 25, 6500);
        gpaah89_set_tap(p, 2, 42, 4600);
        gpaah89_set_tap(p, 3, 66, 3200);
        gpaah89_set_tap(p, 4, 98, 1900);
        gpaah89_set_tap(p, 5, 140, 950);
        p->reverb_mix_q15 = 6000;
        p->output_gain_q15 = 19500;
        break;

    case GPAAH89_PRESET_METRALLA:
        /* BASE: fuller low-mid body with a short readable crack. */
        p->noise_mix_q15[0] = 27000;
        p->noise_mix_q15[1] = 32767;
        p->noise_mix_q15[2] = 27500;
        p->transient_correlation_ms_x10 = 24U;
        p->transient_correlation_mix_q15 = 16000;
        gpaah89_set_env(p, 0, 92, 8, 140, 1800);
        gpaah89_set_env(p, 1, 40, 4, 62, 950);
        gpaah89_set_env(p, 2, 5, 0, 9, 0);
        p->eq[0].gain_q14 = 19000;
        p->eq[1].gain_q14 = 23000;
        p->eq[2].gain_q14 = 24000;
        p->eq[3].gain_q14 = 23500;
        p->eq[4].gain_q14 = 21000;
        p->eq[5].gain_q14 = 14500;
        p->distortion_drive_start_q8 = 690;
        p->distortion_drive_end_q8 = 300;
        p->distortion_mix_q15 = 14000;
        p->chorus_left_ms_x10 = 15;
        p->chorus_right_ms_x10 = 30;
        p->chorus_depth_ms_x10 = 5;
        p->chorus_mix_start_q15 = 5200;
        p->chorus_mix_end_q15 = 1800;
        gpaah89_set_tap(p, 0, 10, 6500);
        gpaah89_set_tap(p, 1, 19, 4800);
        gpaah89_set_tap(p, 2, 31, 3400);
        gpaah89_set_tap(p, 3, 49, 2300);
        gpaah89_set_tap(p, 4, 72, 1500);
        gpaah89_set_tap(p, 5, 105, 750);
        p->reverb_mix_q15 = 3600;
        p->output_gain_q15 = 23500;
        break;

    case GPAAH89_PRESET_GATLING:
        /* BASE: lower pressure tunnel; 20 ms retrigger stays intact. */
        p->noise_mix_q15[0] = 29000;
        p->noise_mix_q15[1] = 32767;
        p->noise_mix_q15[2] = 26000;
        p->transient_correlation_ms_x10 = 20U;
        p->transient_correlation_mix_q15 = 15000;
        gpaah89_set_env(p, 0, 72, 5, 95, 1500);
        gpaah89_set_env(p, 1, 30, 2, 45, 650);
        gpaah89_set_env(p, 2, 4, 0, 7, 0);
        p->eq[0].gain_q14 = 20500;
        p->eq[1].gain_q14 = 24200;
        p->eq[2].gain_q14 = 25000;
        p->eq[3].gain_q14 = 23800;
        p->eq[4].gain_q14 = 19500;
        p->eq[5].gain_q14 = 12500;
        p->distortion_drive_start_q8 = 700;
        p->distortion_drive_end_q8 = 305;
        p->distortion_mix_q15 = 14500;
        p->chorus_left_ms_x10 = 14;
        p->chorus_right_ms_x10 = 28;
        p->chorus_depth_ms_x10 = 4;
        p->chorus_mix_start_q15 = 4800;
        p->chorus_mix_end_q15 = 1500;
        gpaah89_set_tap(p, 0, 9, 5600);
        gpaah89_set_tap(p, 1, 16, 4300);
        gpaah89_set_tap(p, 2, 27, 3100);
        gpaah89_set_tap(p, 3, 42, 2100);
        gpaah89_set_tap(p, 4, 63, 1300);
        gpaah89_set_tap(p, 5, 88, 650);
        p->reverb_mix_q15 = 3000;
        p->output_gain_q15 = 21500;
        break;

    case GPAAH89_PRESET_ROCKET_LAUNCHER:
        /* BASE v5: lower pressure mass and a longer dark expansion. */
        p->noise_mix_q15[0] = 32767;
        p->noise_mix_q15[1] = 30000;
        p->noise_mix_q15[2] = 21000;
        p->transient_correlation_ms_x10 = 45U;
        p->transient_correlation_mix_q15 = 24500;
        gpaah89_set_env(p, 0, 380, 110, 680, 11000);
        gpaah89_set_env(p, 1, 165, 45, 330, 5500);
        gpaah89_set_env(p, 2, 8, 0, 21, 0);
        gpaah89_set_eq(p, 0, 20, 70, 31000);
        gpaah89_set_eq(p, 1, 70, 180, 31000);
        gpaah89_set_eq(p, 2, 180, 500, 25500);
        gpaah89_set_eq(p, 3, 500, 1400, 18000);
        gpaah89_set_eq(p, 4, 1400, 4000, 12000);
        gpaah89_set_eq(p, 5, 4000, 9000, 7500);
        p->distortion_drive_start_q8 = 950;
        p->distortion_drive_end_q8 = 375;
        p->distortion_mix_q15 = 18500;
        p->chorus_left_ms_x10 = 22;
        p->chorus_right_ms_x10 = 55;
        p->chorus_depth_ms_x10 = 18;
        p->chorus_rate_millihz = 360;
        p->chorus_mix_start_q15 = 8500;
        p->chorus_mix_end_q15 = 3300;
        gpaah89_set_tap(p, 0, 18, 11000);
        gpaah89_set_tap(p, 1, 39, 8200);
        gpaah89_set_tap(p, 2, 72, 6000);
        gpaah89_set_tap(p, 3, 118, 4200);
        gpaah89_set_tap(p, 4, 158, 2800);
        gpaah89_set_tap(p, 5, 180, 1700);
        p->reverb_mix_q15 = 9800;
        p->output_gain_q15 = 20000;
        break;

    case GPAAH89_PRESET_SNIPER_RIFLE:
        p->noise_mix_q15[0] = 26000;
        p->noise_mix_q15[1] = 32767;
        p->noise_mix_q15[2] = 32767;
        gpaah89_set_env(p, 0, 160, 16, 260, 4000);
        gpaah89_set_env(p, 1, 65, 8, 110, 1800);
        gpaah89_set_env(p, 2, 7, 0, 14, 0);
        p->eq[0].gain_q14 = 18000;
        p->eq[1].gain_q14 = 22000;
        p->eq[2].gain_q14 = 22000;
        p->eq[3].gain_q14 = 19500;
        p->eq[4].gain_q14 = 22000;
        p->eq[5].gain_q14 = 21000;
        p->distortion_drive_start_q8 = 700;
        p->distortion_drive_end_q8 = 320;
        p->distortion_mix_q15 = 15000;
        p->chorus_mix_start_q15 = 6553;
        p->chorus_mix_end_q15 = 1966;
        p->reverb_mix_q15 = 5243;
        p->output_gain_q15 = 19000;
        break;

    default:
        return 0;
    }
    return 1;
}

int gpaah89_init(gpaah89_state *s,
                 gpaah89_u32 sample_rate,
                 const gpaah89_preset *p,
                 gpaah89_u32 seed)
{
    int i;

    if (s == 0 || p == 0) return 0;
    if (!gpaah89_platform_ok()) return 0;
    if (sample_rate < 8000U || sample_rate > GPAAH89_MAX_SAMPLE_RATE)
        return 0;

    s->sample_rate = sample_rate;
    s->preset = *p;
    s->chorus_write = 0U;
    s->reverb_write = 0U;
    s->chorus_phase = 0U;
    s->chorus_phase_inc =
        ((gpaah89_u32)p->chorus_rate_millihz * 4294967U) / sample_rate;
    s->tail_pos = 0U;
    s->trigger_pos = 0U;
    s->correlation_samples =
        ((gpaah89_u32)p->transient_correlation_ms_x10 * sample_rate) /
        10000U;
    s->active = 0U;

    s->noise[0].period = 4U;
    s->noise[0].smooth_shift = 3U;
    s->noise[1].period = 2U;
    s->noise[1].smooth_shift = 2U;
    s->noise[2].period = 1U;
    s->noise[2].smooth_shift = 1U;

    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i) {
        s->noise[i].rng = seed ^
            (0x9e3779b9U * (gpaah89_u32)(i + 1));
        s->noise[i].target = 0;
        s->noise[i].value = 0;
        s->noise[i].counter = 0U;
        s->env[i].pos = 0U;
        s->env[i].stage = GPAAH89_ENV_OFF;
        s->env[i].level_q15 = 0;
    }

    for (i = 0; i < GPAAH89_EQ_BANDS; ++i) {
        s->eq_state[i].low_alpha_q15 =
            gpaah89_alpha_from_hz(sample_rate, p->eq[i].low_hz);
        s->eq_state[i].high_alpha_q15 =
            gpaah89_alpha_from_hz(sample_rate, p->eq[i].high_hz);
    }

    gpaah89_clear_state_buffers(s);
    gpaah89_trigger(s, seed);
    return 1;
}

void gpaah89_trigger(gpaah89_state *s, gpaah89_u32 seed)
{
    gpaah89_u32 samples;
    int i;

    if (s == 0) return;

    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i) {
        samples = ((gpaah89_u32)s->preset.envelope[i].decay_ms *
                   s->sample_rate) / 1000U;
        if (samples == 0U) samples = 1U;
        s->env[i].decay_samples = samples;

        s->env[i].sustain_samples =
            ((gpaah89_u32)s->preset.envelope[i].sustain_ms *
             s->sample_rate) / 1000U;

        samples = ((gpaah89_u32)s->preset.envelope[i].release_ms *
                   s->sample_rate) / 1000U;
        if (samples == 0U) samples = 1U;
        s->env[i].release_samples = samples;

        s->env[i].stage = GPAAH89_ENV_DECAY;
        s->env[i].pos = 0U;
        s->env[i].level_q15 = 32767;

        s->noise[i].rng = seed ^
            (0x85ebca6bU * (gpaah89_u32)(i + 1));
        s->noise[i].counter = 0U;
        s->noise[i].target = 0;
        s->noise[i].value = 0;
    }

    s->tail_pos = 0U;
    s->trigger_pos = 0U;
    s->active = 1U;
}

static void gpaah89_render_one(gpaah89_state *s,
                               gpaah89_s16 *out_l,
                               gpaah89_s16 *out_r)
{
    gpaah89_s32 source;
    gpaah89_s32 noise;
    gpaah89_s32 raw_noise[GPAAH89_NOISE_OSCS];
    gpaah89_s32 common_noise;
    gpaah89_s32 correlation_mix;
    gpaah89_u32 correlation_remain;
    gpaah89_s32 env;
    gpaah89_s32 env_peak;
    gpaah89_s32 shaped;
    gpaah89_s32 distorted;
    gpaah89_s32 chorus_l;
    gpaah89_s32 chorus_r;
    gpaah89_s32 reverb_l;
    gpaah89_s32 reverb_r;
    int i;

    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i)
        raw_noise[i] = gpaah89_noise_next(&s->noise[i]);

    correlation_mix = 0;
    if (s->correlation_samples > 0U &&
        s->trigger_pos < s->correlation_samples) {
        correlation_remain = s->correlation_samples - s->trigger_pos;
        correlation_mix = (gpaah89_s32)
            (((gpaah89_u32)s->preset.transient_correlation_mix_q15 *
              correlation_remain) / s->correlation_samples);
    }
    common_noise = raw_noise[GPAAH89_NOISE_MID];

    source = 0;
    env_peak = 0;
    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i) {
        noise = raw_noise[i];
        if (correlation_mix > 0) {
            noise = gpaah89_mul_q15(noise, 32767 - correlation_mix) +
                    gpaah89_mul_q15(common_noise, correlation_mix);
        }
        env = gpaah89_env_next(&s->env[i], &s->preset.envelope[i]);
        if (env > env_peak) env_peak = env;
        noise = gpaah89_mul_q15(noise, s->preset.noise_mix_q15[i]);
        noise = gpaah89_mul_q15(noise, env);
        source += noise;
        source = gpaah89_clamp_s32(source, -65535, 65535);
    }
    s->trigger_pos++;

    if (gpaah89_any_envelope_active(s)) {
        s->tail_pos = 0U;
    } else {
        s->tail_pos++;
        if (s->tail_pos >= s->sample_rate / 2U) s->active = 0U;
    }

    shaped = gpaah89_eq_process(s, source);
    distorted = gpaah89_distort(s, shaped, env_peak);
    distorted = gpaah89_mul_q15(distorted, s->preset.output_gain_q15);

    gpaah89_chorus(s, distorted, env_peak, &chorus_l, &chorus_r);
    gpaah89_reverb(s, chorus_l, chorus_r, &reverb_l, &reverb_r);

    *out_l = gpaah89_clamp_s16(reverb_l);
    *out_r = gpaah89_clamp_s16(reverb_r);
}

gpaah89_u32 gpaah89_render_stereo(gpaah89_state *s,
                                  gpaah89_s16 *out,
                                  gpaah89_u32 frames)
{
    gpaah89_u32 i;
    gpaah89_s16 l;
    gpaah89_s16 r;

    if (s == 0 || out == 0) return 0U;
    for (i = 0U; i < frames; ++i) {
        gpaah89_render_one(s, &l, &r);
        out[i * 2U] = l;
        out[i * 2U + 1U] = r;
    }
    return frames;
}

gpaah89_u32 gpaah89_render_mono(gpaah89_state *s,
                                gpaah89_s16 *out,
                                gpaah89_u32 frames)
{
    gpaah89_u32 i;
    gpaah89_s16 l;
    gpaah89_s16 r;

    if (s == 0 || out == 0) return 0U;
    for (i = 0U; i < frames; ++i) {
        gpaah89_render_one(s, &l, &r);
        out[i] = (gpaah89_s16)(((gpaah89_s32)l +
                                (gpaah89_s32)r) >> 1);
    }
    return frames;
}

int gpaah89_is_active(const gpaah89_state *s)
{
    if (s == 0) return 0;
    return s->active ? 1 : 0;
}
