#include "gweaponfoley89.h"

#define GWF89_U32_MASK 0xFFFFFFFFUL

static gwf89_s16 gwf89_sat16(gwf89_s32 x)
{
    if (x > 32767L) return (gwf89_s16)32767;
    if (x < -32768L) return (gwf89_s16)-32768;
    return (gwf89_s16)x;
}

static gwf89_s16 gwf89_mul_q15(gwf89_s16 a, gwf89_s16 b)
{
    gwf89_s32 p;
    p = (gwf89_s32)a * (gwf89_s32)b;
    return gwf89_sat16(p >> 15);
}

static gwf89_s16 gwf89_mul_q12(gwf89_s16 a, signed short b)
{
    gwf89_s32 p;
    p = (gwf89_s32)a * (gwf89_s32)b;
    return gwf89_sat16(p >> 12);
}

static gwf89_u32 gwf89_rng(gwf89_u32 *state)
{
    gwf89_u32 x;
    x = *state & GWF89_U32_MASK;
    if (x == 0UL) x = 0x6D2B79F5UL;
    x ^= (x << 13) & GWF89_U32_MASK;
    x ^= (x >> 17);
    x ^= (x << 5) & GWF89_U32_MASK;
    x &= GWF89_U32_MASK;
    *state = x;
    return x;
}

static gwf89_s16 gwf89_noise(gwf89_u32 *state)
{
    gwf89_u32 x;
    x = gwf89_rng(state);
    return (gwf89_s16)((signed long)((x >> 16) & 65535UL) - 32768L);
}

static void gwf89_zero16(gwf89_s16 *p, unsigned long count)
{
    unsigned long i;
    for (i = 0UL; i < count; ++i) p[i] = 0;
}

static gwf89_s16 gwf89_envelope(const gwf89_hit *h, gwf89_u32 local)
{
    gwf89_u32 a;
    gwf89_u32 d;
    gwf89_u32 hold;
    gwf89_u32 r;
    gwf89_s32 v;
    gwf89_s32 span;

    a = (gwf89_u32)h->attack_samples;
    d = (gwf89_u32)h->decay_samples;
    hold = (gwf89_u32)h->hold_samples;
    r = (gwf89_u32)h->release_samples;

    if (a != 0UL && local < a) {
        v = ((gwf89_s32)h->peak_q15 * (gwf89_s32)(local + 1UL)) / (gwf89_s32)a;
        return gwf89_sat16(v);
    }
    if (local >= a) local -= a;

    if (d != 0UL && local < d) {
        span = (gwf89_s32)h->peak_q15 - (gwf89_s32)h->sustain_q15;
        v = (gwf89_s32)h->peak_q15 - (span * (gwf89_s32)local) / (gwf89_s32)d;
        return gwf89_sat16(v);
    }
    if (local >= d) local -= d;

    if (local < hold) return h->sustain_q15;
    local -= hold;

    if (r != 0UL && local < r) {
        v = ((gwf89_s32)h->sustain_q15 * (gwf89_s32)(r - local)) / (gwf89_s32)r;
        return gwf89_sat16(v);
    }
    return 0;
}


static gwf89_s16 gwf89_tone_envelope(const gwf89_tone_shape *t,
                                      gwf89_u32 local)
{
    gwf89_u32 a;
    gwf89_u32 d;
    gwf89_u32 hold;
    gwf89_u32 r;
    gwf89_s32 v;
    gwf89_s32 span;

    a = (gwf89_u32)t->attack_samples;
    d = (gwf89_u32)t->decay_samples;
    hold = (gwf89_u32)t->hold_samples;
    r = (gwf89_u32)t->release_samples;

    if (a != 0UL && local < a) {
        v = (32767L * (gwf89_s32)(local + 1UL)) / (gwf89_s32)a;
        return gwf89_sat16(v);
    }
    if (local >= a) local -= a;

    if (d != 0UL && local < d) {
        span = 32767L - (gwf89_s32)t->sustain_q15;
        v = 32767L - (span * (gwf89_s32)local) / (gwf89_s32)d;
        return gwf89_sat16(v);
    }
    if (local >= d) local -= d;

    if (local < hold) return t->sustain_q15;
    local -= hold;

    if (r != 0UL && local < r) {
        v = ((gwf89_s32)t->sustain_q15 * (gwf89_s32)(r - local)) /
            (gwf89_s32)r;
        return gwf89_sat16(v);
    }
    return 0;
}

static gwf89_s16 gwf89_particle_filter(gwf89_context *ctx,
                                        unsigned int hit_index,
                                        gwf89_s16 input,
                                        gwf89_s16 tone_env,
                                        const gwf89_tone_shape *t)
{
    gwf89_s32 alpha;
    gwf89_s16 delta;
    gwf89_s16 low;
    gwf89_s16 band;
    gwf89_s16 high;
    gwf89_s32 out;

    /*
       SimSynth-inspired particle stage:
       - amplitude and tone envelopes are independent;
       - the tone envelope opens a two-pole multimode filter for the tick;
       - as it closes, a darker low/band body remains under the transient.
       This turns the source into a short metal particle instead of merely
       chopping broadband noise.
    */
    alpha = (gwf89_s32)t->cutoff_base_q15 +
            (gwf89_s32)gwf89_mul_q15(t->cutoff_env_q15, tone_env);
    if (alpha < 384L) alpha = 384L;
    if (alpha > 29200L) alpha = 29200L;

    delta = gwf89_sat16((gwf89_s32)input -
                        (gwf89_s32)ctx->hit_tone_lp1[hit_index]);
    ctx->hit_tone_lp1[hit_index] = gwf89_sat16(
        (gwf89_s32)ctx->hit_tone_lp1[hit_index] +
        (gwf89_s32)gwf89_mul_q15((gwf89_s16)alpha, delta));

    delta = gwf89_sat16((gwf89_s32)ctx->hit_tone_lp1[hit_index] -
                        (gwf89_s32)ctx->hit_tone_lp2[hit_index]);
    ctx->hit_tone_lp2[hit_index] = gwf89_sat16(
        (gwf89_s32)ctx->hit_tone_lp2[hit_index] +
        (gwf89_s32)gwf89_mul_q15((gwf89_s16)alpha, delta));

    low = ctx->hit_tone_lp2[hit_index];
    band = gwf89_sat16((gwf89_s32)ctx->hit_tone_lp1[hit_index] -
                       (gwf89_s32)ctx->hit_tone_lp2[hit_index]);
    high = gwf89_sat16((gwf89_s32)input -
                       (gwf89_s32)ctx->hit_tone_lp1[hit_index]);

    out = (gwf89_s32)gwf89_mul_q15(low, 24576);
    out += (gwf89_s32)gwf89_mul_q15(
        band, gwf89_sat16((gwf89_s32)t->band_mix_q15 +
                          (gwf89_s32)t->emphasis_q15));
    out += (gwf89_s32)gwf89_mul_q15(high, t->high_mix_q15);
    return gwf89_sat16(out);
}

static gwf89_s16 gwf89_filterbank(gwf89_s16 state[5], gwf89_s16 input,
                                     const signed short gain_q12[6])
{
    static const gwf89_s16 alpha[5] = { 1147, 3530, 8817, 16696, 24885 };
    gwf89_s16 band[6];
    gwf89_s16 previous;
    gwf89_s16 delta;
    gwf89_s32 sum;
    int i;

    for (i = 0; i < 5; ++i) {
        delta = gwf89_sat16((gwf89_s32)input - (gwf89_s32)state[i]);
        state[i] = gwf89_sat16((gwf89_s32)state[i] +
                   (gwf89_s32)gwf89_mul_q15(alpha[i], delta));
    }

    band[0] = state[0];
    previous = state[0];
    for (i = 1; i < 5; ++i) {
        band[i] = gwf89_sat16((gwf89_s32)state[i] - (gwf89_s32)previous);
        previous = state[i];
    }
    band[5] = gwf89_sat16((gwf89_s32)input - (gwf89_s32)state[4]);

    sum = 0L;
    for (i = 0; i < 6; ++i) {
        sum += (gwf89_s32)gwf89_mul_q12(band[i], gain_q12[i]);
    }
    return gwf89_sat16(sum);
}

static gwf89_s16 gwf89_distortion(gwf89_s16 x, gwf89_s16 amount_q15)
{
    gwf89_s32 drive_q12;
    gwf89_s32 threshold;
    gwf89_s32 wet_q15;
    gwf89_s32 y;
    gwf89_s32 ay;
    gwf89_s32 over;
    gwf89_s32 span;
    gwf89_s32 curved;
    gwf89_s32 signed_curve;
    gwf89_s32 out;

    if (amount_q15 < 0) amount_q15 = 0;

    /*
       True fixed-point soft clipping. The amount controls three parts of
       the same distortion stage: pre-gain, knee and wet mix.

       0      -> about 1.0x, high knee, mostly dry
       32767  -> about 3.5x, low knee, mostly saturated

       The rational knee approaches full scale smoothly and avoids the
       brittle flat top of hard clipping. No lookup table or allocator.
    */
    drive_q12 = 4096L + (((gwf89_s32)amount_q15 * 10240L) >> 15);
    threshold = 27000L - (((gwf89_s32)amount_q15 * 12500L) >> 15);
    wet_q15 = 4096L + (((gwf89_s32)amount_q15 * 24576L) >> 15);

    if (threshold < 12000L) threshold = 12000L;
    if (wet_q15 > 32767L) wet_q15 = 32767L;

    y = ((gwf89_s32)x * drive_q12) >> 12;
    if (y > 65535L) y = 65535L;
    if (y < -65535L) y = -65535L;

    ay = (y < 0L) ? -y : y;
    if (ay <= threshold) {
        curved = ay;
    } else {
        over = ay - threshold;
        span = 32767L - threshold;
        curved = threshold + (over * span) / (over + span);
    }

    signed_curve = (y < 0L) ? -curved : curved;
    out = ((gwf89_s32)x * (32767L - wet_q15) +
           signed_curve * wet_q15) >> 15;
    return gwf89_sat16(out);
}

static gwf89_s16 gwf89_contact_body(gwf89_context *ctx,
                                         unsigned int hit_index,
                                         gwf89_s16 input,
                                         const gwf89_hit *hit)
{
    static const gwf89_s16 mode_weight_q15[GWF89_METAL_MODES] = {
        10500, -9200, 7900, -6600
    };
    gwf89_s32 mode_sum;
    gwf89_s32 write_value;
    gwf89_s32 out;
    gwf89_s32 wet_long;
    gwf89_s32 feedback_long;
    gwf89_s32 damping_long;
    gwf89_s16 delayed;
    gwf89_s16 damped;
    gwf89_s16 delta;
    gwf89_s16 feedback;
    gwf89_s16 excite;
    gwf89_s16 wet;
    gwf89_s16 damping;
    unsigned short delay;
    unsigned short read_pos;
    unsigned short write_pos;
    unsigned int i;

    /*
       One four-mode rigid-body bank per physical contact. This keeps a
       striker, spring, rail, hammer and tubular latch from sharing the same
       damping law. White noise remains the only excitation source.
    */
    if (hit_index >= GWF89_MAX_HITS) return input;
    excite = (gwf89_s16)(input >> 2);
    mode_sum = 0L;
    write_pos = ctx->hit_metal_write[hit_index];

    wet_long = ((gwf89_s32)ctx->preset->metal_wet_q15 * 3L +
                (gwf89_s32)hit->body_wet_q15) >> 2;
    feedback_long = ((gwf89_s32)ctx->preset->metal_feedback_q15 * 3L +
                     (gwf89_s32)hit->body_feedback_q15) >> 2;
    damping_long = ((gwf89_s32)ctx->preset->metal_damping_q15 * 3L +
                    (gwf89_s32)hit->body_damping_q15) >> 2;
    if (wet_long < 0L) wet_long = 0L;
    if (wet_long > 20000L) wet_long = 20000L;
    if (feedback_long < 0L) feedback_long = 0L;
    if (feedback_long > 26000L) feedback_long = 26000L;
    if (damping_long < 3000L) damping_long = 3000L;
    if (damping_long > 29000L) damping_long = 29000L;
    wet = (gwf89_s16)wet_long;
    feedback = (gwf89_s16)feedback_long;
    damping = (gwf89_s16)damping_long;

    for (i = 0U; i < GWF89_METAL_MODES; ++i) {
        delay = (unsigned short)ctx->preset->metal_delay[i];
        if (delay < 2U) delay = 2U;
        if (delay >= GWF89_METAL_BUFFER) delay = GWF89_METAL_BUFFER - 1U;

        if (write_pos >= delay)
            read_pos = (unsigned short)(write_pos - delay);
        else
            read_pos = (unsigned short)(GWF89_METAL_BUFFER + write_pos - delay);

        delayed = ctx->hit_metal_buffer[hit_index][i][read_pos];
        delta = gwf89_sat16((gwf89_s32)delayed -
                            (gwf89_s32)ctx->hit_metal_lp[hit_index][i]);
        ctx->hit_metal_lp[hit_index][i] = gwf89_sat16(
            (gwf89_s32)ctx->hit_metal_lp[hit_index][i] +
            (gwf89_s32)gwf89_mul_q15(damping, delta));
        damped = ctx->hit_metal_lp[hit_index][i];

        if ((i & 1U) != 0U)
            write_value = -(gwf89_s32)excite +
                          (gwf89_s32)gwf89_mul_q15(damped, feedback);
        else
            write_value = (gwf89_s32)excite +
                          (gwf89_s32)gwf89_mul_q15(damped, feedback);

        ctx->hit_metal_buffer[hit_index][i][write_pos] =
            gwf89_sat16(write_value);
        mode_sum += (gwf89_s32)gwf89_mul_q15(damped,
                                              mode_weight_q15[i]);
    }

    ctx->hit_metal_write[hit_index] =
        (unsigned short)((write_pos + 1U) & (GWF89_METAL_BUFFER - 1U));

    out = (gwf89_s32)input + ((mode_sum * (gwf89_s32)wet) >> 15);
    return gwf89_sat16(out);
}

static gwf89_s16 gwf89_chorus(gwf89_context *ctx, gwf89_s16 input)
{
    unsigned short phase;
    unsigned short tri;
    unsigned short delay;
    unsigned short read_pos;
    gwf89_s16 delayed;
    gwf89_s32 out;
    gwf89_s16 wet;

    phase = ctx->chorus_phase;
    if (phase < 32768U) tri = phase;
    else tri = (unsigned short)(65535U - phase);

    delay = (unsigned short)(ctx->preset->chorus_base_delay +
            (unsigned short)(((unsigned long)ctx->preset->chorus_depth * (unsigned long)tri) >> 15));
    if (delay >= GWF89_CHORUS_BUFFER) delay = GWF89_CHORUS_BUFFER - 1;

    if (ctx->chorus_write >= delay) read_pos = (unsigned short)(ctx->chorus_write - delay);
    else read_pos = (unsigned short)(GWF89_CHORUS_BUFFER + ctx->chorus_write - delay);

    delayed = ctx->chorus_buffer[read_pos];
    ctx->chorus_buffer[ctx->chorus_write] = input;
    ctx->chorus_write = (unsigned short)((ctx->chorus_write + 1U) & (GWF89_CHORUS_BUFFER - 1U));
    ctx->chorus_phase = (unsigned short)(ctx->chorus_phase + ctx->preset->chorus_phase_step);

    wet = ctx->preset->chorus_wet_q15;
    out = ((gwf89_s32)input * (gwf89_s32)(32767 - wet) +
           (gwf89_s32)delayed * (gwf89_s32)wet) >> 15;
    return gwf89_sat16(out);
}

static gwf89_s16 gwf89_reverb_room(gwf89_context *ctx, gwf89_s16 input)
{
    gwf89_s16 da;
    gwf89_s16 db;
    gwf89_s16 apd;
    gwf89_s16 feedback;
    gwf89_s16 wet;
    gwf89_s16 comb;
    gwf89_s16 ap;
    gwf89_s32 write_value;
    unsigned short delay_a;
    unsigned short delay_b;
    unsigned short delay_ap;
    unsigned short read_a;
    unsigned short read_b;
    unsigned short read_ap;

    delay_a = ctx->preset->reverb_delay_a;
    delay_b = ctx->preset->reverb_delay_b;
    delay_ap = ctx->preset->allpass_delay;
    if (delay_a < 2U) delay_a = 2U;
    if (delay_b < 2U) delay_b = 2U;
    if (delay_ap < 2U) delay_ap = 2U;
    if (delay_a >= GWF89_REVERB_BUFFER)
        delay_a = GWF89_REVERB_BUFFER - 1U;
    if (delay_b >= GWF89_REVERB_BUFFER)
        delay_b = GWF89_REVERB_BUFFER - 1U;
    if (delay_ap >= GWF89_ALLPASS_BUFFER)
        delay_ap = GWF89_ALLPASS_BUFFER - 1U;

    if (ctx->reverb_a_pos >= delay_a)
        read_a = (unsigned short)(ctx->reverb_a_pos - delay_a);
    else
        read_a = (unsigned short)(GWF89_REVERB_BUFFER +
                                  ctx->reverb_a_pos - delay_a);
    if (ctx->reverb_b_pos >= delay_b)
        read_b = (unsigned short)(ctx->reverb_b_pos - delay_b);
    else
        read_b = (unsigned short)(GWF89_REVERB_BUFFER +
                                  ctx->reverb_b_pos - delay_b);

    da = ctx->reverb_a[read_a];
    db = ctx->reverb_b[read_b];
    feedback = ctx->preset->reverb_feedback_q15;

    write_value = (gwf89_s32)input +
                  (gwf89_s32)gwf89_mul_q15(da, feedback);
    ctx->reverb_a[ctx->reverb_a_pos] = gwf89_sat16(write_value);
    write_value = (gwf89_s32)input +
                  (gwf89_s32)gwf89_mul_q15(
                      db, (gwf89_s16)(feedback - 1200));
    ctx->reverb_b[ctx->reverb_b_pos] = gwf89_sat16(write_value);

    ctx->reverb_a_pos =
        (unsigned short)((ctx->reverb_a_pos + 1U) &
                         (GWF89_REVERB_BUFFER - 1U));
    ctx->reverb_b_pos =
        (unsigned short)((ctx->reverb_b_pos + 1U) &
                         (GWF89_REVERB_BUFFER - 1U));

    comb = gwf89_sat16(((gwf89_s32)da + (gwf89_s32)db) >> 1);

    if (ctx->allpass_pos >= delay_ap)
        read_ap = (unsigned short)(ctx->allpass_pos - delay_ap);
    else
        read_ap = (unsigned short)(GWF89_ALLPASS_BUFFER +
                                   ctx->allpass_pos - delay_ap);
    apd = ctx->allpass[read_ap];
    ap = gwf89_sat16((gwf89_s32)apd -
                     (gwf89_s32)gwf89_mul_q15(comb, 16384));
    ctx->allpass[ctx->allpass_pos] =
        gwf89_sat16((gwf89_s32)comb +
                    (gwf89_s32)gwf89_mul_q15(ap, 16384));
    ctx->allpass_pos =
        (unsigned short)((ctx->allpass_pos + 1U) &
                         (GWF89_ALLPASS_BUFFER - 1U));

    wet = ctx->preset->reverb_wet_q15;
    return gwf89_mul_q15(ap, wet);
}

#define EQ_PISTOL_RELEASE { 1200, 2800, 5200, 8200, 8400, 4200 }
#define EQ_PISTOL_STOP    { 2000, 4000, 6500, 8600, 7400, 3600 }
#define EQ_SPRING_EDGE    { 700, 1800, 3900, 7600, 9800, 5200 }
#define EQ_FRAME_TAP      { 3300, 5700, 7000, 6500, 4800, 2600 }
#define EQ_DETENT         { 900, 2400, 5200, 8600, 9500, 4200 }
#define EQ_MAGNUM_RATCHET { 2300, 5000, 7300, 7600, 5400, 2800 }
#define EQ_MAGNUM_HAMMER  { 4400, 7000, 7600, 6400, 4700, 2400 }
#define EQ_BOLT_RAIL      { 4300, 6800, 6900, 5300, 4200, 2500 }
#define EQ_BOLT_STOP      { 5400, 7600, 7300, 5500, 4000, 2200 }
#define EQ_BOLT_LOCK      { 3600, 6500, 7700, 7400, 5600, 2800 }
#define EQ_SMG_HAMMER     { 2500, 5300, 7100, 7600, 5800, 2900 }
#define EQ_TUBE_CONTACT   { 6700, 8200, 6400, 4300, 3000, 1800 }
#define EQ_SHOT_HAMMER    { 4900, 7400, 7200, 6100, 4400, 2200 }
#define EQ_SAFETY_CLICK   { 1400, 3300, 6000, 8500, 8500, 3800 }

#define TONE_EDGE    { 1, 105, 14, 281, 1265, 4300, 17600, 2600, 9600, 15000 }
#define TONE_STRIKER { 1, 145, 22, 379, 1725, 3600, 16500, 3600, 7600, 17800 }
#define TONE_SPRING  { 1, 82, 8, 231, 920, 5200, 18400, 1800, 12400, 13200 }
#define TONE_FRAME   { 1, 225, 48, 611, 2645, 2600, 12600, 3000, 4800, 19000 }
#define TONE_DETENT  { 1, 118, 16, 306, 1380, 3900, 17100, 3900, 9800, 16900 }
#define TONE_RATCHET { 1, 170, 24, 420, 1725, 3200, 15000, 4300, 7200, 19000 }
#define TONE_HEAVY   { 1, 285, 72, 858, 2990, 2100, 11200, 4200, 3900, 20600 }
#define TONE_RAIL    { 2, 380, 105, 1073, 3795, 1650, 9200, 1800, 3000, 18400 }
#define TONE_STOP    { 1, 245, 58, 775, 2760, 2350, 12600, 3900, 4300, 20700 }
#define TONE_LOCK    { 1, 210, 44, 594, 2185, 2850, 14300, 4500, 6100, 20100 }
#define TONE_MID     { 1, 210, 42, 585, 2185, 3000, 14500, 3700, 6100, 19500 }
#define TONE_TUBE    { 2, 430, 118, 1353, 4600, 1250, 7800, 2500, 2300, 18600 }
#define TONE_SAFETY  { 1, 138, 18, 354, 1495, 3700, 16600, 4300, 8300, 18100 }
#define TONE_NONE    { 0, 0, 0, 0, 0, 32767, 0, 0, 0, 0 }

#define BODY_EDGE    4200,  9200, 25200
#define BODY_STRIKER 7200, 14200, 22400
#define BODY_SPRING  3500,  7600, 26500
#define BODY_FRAME   9000, 15600, 19000
#define BODY_DETENT  4700, 10200, 24600
#define BODY_RATCHET 6600, 12600, 22000
#define BODY_HEAVY  11800, 19800, 16400
#define BODY_RAIL    5200,  9800, 17800
#define BODY_STOP    9800, 17400, 18200
#define BODY_LOCK    8600, 15800, 19600
#define BODY_MID     7600, 14400, 20500
#define BODY_TUBE   13600, 21400, 14800
#define BODY_SAFETY  4500,  9800, 25000
#define BODY_NONE       0,     0, 32767

#define HIT(off,a,d,h,r,p,s,l,w,hi,eq,drv,tone,body) \
    { off,a,d,h,r,p,s,l,w,hi,eq,drv,tone,body }
#define ZHIT HIT(0,0,0,0,0,0,0,0,0,0,EQ_PISTOL_RELEASE,0,TONE_NONE,BODY_NONE)

/*
   v1.6 real-reference calibration: each micro-contact keeps its own
   amplitude/tone envelope, six-band color, drive and rigid-body damping.
   Runtime variants add tiny deterministic timing/material differences, while
   slow/normal/fast timing preserves the same mechanism order. White noise
   remains the only source.
*/
static const gwf89_preset gwf89_presets[GWF89_PRESET_COUNT] = {
    {
        "pistol_empty", 3,
        {
            HIT(0,1,28,12,280,15500,3488,2500,21500,28500,EQ_PISTOL_RELEASE,12500,TONE_EDGE,BODY_EDGE),
            HIT(176,1,65,22,479,30500,9150,9500,25000,18000,EQ_PISTOL_STOP,27200,TONE_STRIKER,BODY_STRIKER),
            HIT(397,1,32,8,276,9000,1575,2500,18500,28500,EQ_SPRING_EDGE,9000,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 2000, 3800, 6200, 7600, 6500, 3300 }, 11800,
        7200, 13500, 21800, { 7, 11, 17, 23 },
        1550, 168, 22, 1, 1650, 5600, 251, 421, 137, 1700
    },
    {
        "pistol_handling", 4,
        {
            HIT(0,2,74,48,630,20500,7688,12000,23000,12500,EQ_FRAME_TAP,19000,TONE_FRAME,BODY_FRAME),
            HIT(574,1,40,16,339,12500,2969,3500,23000,25000,EQ_DETENT,13500,TONE_DETENT,BODY_DETENT),
            HIT(1190,2,89,48,737,23000,8625,14500,22000,9500,EQ_FRAME_TAP,21200,TONE_FRAME,BODY_FRAME),
            HIT(2058,1,50,18,417,10500,2756,4500,21500,24000,EQ_SAFETY_CLICK,12000,TONE_SAFETY,BODY_SAFETY),
            ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 2600, 4500, 6400, 6800, 5200, 2800 }, 10500,
        7800, 14200, 21000, { 9, 14, 23, 31 },
        1900, 182, 25, 1, 2100, 6000, 307, 503, 157, 2300
    },
    {
        "magnum_empty", 5,
        {
            HIT(0,1,28,16,248,10500,2494,3000,22000,24000,EQ_DETENT,9000,TONE_DETENT,BODY_DETENT),
            HIT(265,1,36,24,288,13000,3738,6000,24500,19000,EQ_MAGNUM_RATCHET,12000,TONE_RATCHET,BODY_RATCHET),
            HIT(617,1,29,16,263,11500,2731,5000,23500,22000,EQ_DETENT,11000,TONE_DETENT,BODY_DETENT),
            HIT(1058,1,45,24,352,17500,5031,9000,24000,15500,EQ_MAGNUM_RATCHET,17000,TONE_RATCHET,BODY_RATCHET),
            HIT(1411,2,112,72,961,31800,13118,22000,24500,7500,EQ_MAGNUM_HAMMER,30000,TONE_HEAVY,BODY_HEAVY),
            ZHIT, ZHIT, ZHIT
        },
        { 4600, 6600, 7000, 5900, 4300, 2400 }, 14500,
        9800, 16200, 19600, { 11, 17, 29, 41 },
        1650, 202, 31, 1, 2500, 6500, 331, 557, 173, 2600
    },
    {
        "magnum_latch", 4,
        {
            HIT(0,1,43,16,372,15500,3681,6500,24500,16500,EQ_DETENT,13500,TONE_DETENT,BODY_DETENT),
            HIT(529,2,79,48,663,22500,8438,15000,23000,8500,EQ_FRAME_TAP,21500,TONE_FRAME,BODY_FRAME),
            HIT(1102,2,113,72,888,27000,11138,20000,22000,6500,EQ_MAGNUM_HAMMER,27000,TONE_HEAVY,BODY_HEAVY),
            HIT(1543,1,37,16,326,12000,2850,5000,23000,21000,EQ_DETENT,11000,TONE_DETENT,BODY_DETENT),
            ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 4900, 6800, 6800, 5500, 4000, 2200 }, 13800,
        10600, 16600, 19000, { 13, 19, 31, 43 },
        1900, 226, 35, 1, 2700, 7000, 367, 601, 191, 2900
    },
    {
        "sniper_empty", 3,
        {
            HIT(0,1,31,12,263,12500,2812,3000,22000,25500,EQ_PISTOL_RELEASE,10000,TONE_EDGE,BODY_EDGE),
            HIT(198,1,73,44,596,30000,10125,12500,24500,14500,EQ_BOLT_LOCK,27500,TONE_LOCK,BODY_LOCK),
            HIT(485,1,36,8,303,8500,1500,2500,18500,28500,EQ_SPRING_EDGE,8500,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 2900, 5200, 7000, 7200, 5400, 2900 }, 12500,
        8200, 14600, 20800, { 9, 15, 24, 37 },
        1500, 187, 20, 1, 2100, 6200, 293, 487, 149, 2100
    },
    {
        "sniper_bolt_dry", 8,
        {
            HIT(0,1,58,44,457,19000,6412,8000,24500,15000,EQ_BOLT_LOCK,17000,TONE_LOCK,BODY_LOCK),
            HIT(529,2,186,105,1156,16500,6394,22500,20000,5500,EQ_BOLT_RAIL,11500,TONE_RAIL,BODY_RAIL),
            HIT(1764,2,225,105,1364,13500,5231,24000,18500,4500,EQ_BOLT_RAIL,10000,TONE_RAIL,BODY_RAIL),
            HIT(3087,2,122,58,961,28500,11044,25500,20500,4000,EQ_BOLT_STOP,29200,TONE_STOP,BODY_STOP),
            HIT(4410,2,194,105,1223,15000,5812,23000,19500,5000,EQ_BOLT_RAIL,10800,TONE_RAIL,BODY_RAIL),
            HIT(6174,2,136,58,1036,23000,8912,21500,22500,7000,EQ_BOLT_STOP,24500,TONE_STOP,BODY_STOP),
            HIT(7056,1,75,44,578,25000,8438,13500,25000,12500,EQ_BOLT_LOCK,25800,TONE_LOCK,BODY_LOCK),
            HIT(7350,1,46,16,372,16500,3919,7500,23500,19000,EQ_DETENT,14500,TONE_DETENT,BODY_DETENT)
        },
        { 4900, 6900, 7100, 5800, 4200, 2400 }, 14500,
        11200, 17200, 18700, { 13, 19, 31, 47 },
        2150, 218, 38, 1, 3100, 7200, 397, 673, 211, 4300
    },
    {
        "smg_empty", 4,
        {
            HIT(0,1,26,16,232,10000,2375,2500,22500,25000,EQ_DETENT,9000,TONE_DETENT,BODY_DETENT),
            HIT(132,1,42,42,350,15500,5231,6500,24500,18000,EQ_SMG_HAMMER,14500,TONE_MID,BODY_MID),
            HIT(309,1,82,42,648,28500,9619,17000,24500,9500,EQ_SMG_HAMMER,27500,TONE_MID,BODY_MID),
            HIT(617,1,35,8,290,9500,1750,3500,20500,26000,EQ_SPRING_EDGE,9500,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 2900, 5400, 7100, 7200, 5200, 2700 }, 13200,
        7800, 14400, 21400, { 8, 13, 21, 34 },
        1400, 148, 18, 2, 1650, 5300, 257, 433, 139, 1800
    },
    {
        "smg_selector", 3,
        {
            HIT(0,1,26,16,232,10500,2494,2500,22000,26000,EQ_DETENT,8500,TONE_DETENT,BODY_DETENT),
            HIT(132,1,50,18,368,21000,5512,6500,25500,16500,EQ_SAFETY_CLICK,18500,TONE_SAFETY,BODY_SAFETY),
            HIT(353,1,28,8,232,8500,1500,2500,20500,28000,EQ_SPRING_EDGE,7500,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 1700, 3700, 6200, 7700, 6500, 3200 }, 10200,
        6800, 13100, 22600, { 7, 12, 19, 31 },
        1550, 160, 18, 2, 1650, 5400, 239, 397, 127, 1600
    },
    {
        "launcher_empty", 3,
        {
            HIT(0,2,46,16,372,12500,2969,9000,23000,15000,EQ_DETENT,11500,TONE_DETENT,BODY_DETENT),
            HIT(353,2,102,118,819,21000,9450,19000,22500,7000,EQ_TUBE_CONTACT,22500,TONE_TUBE,BODY_TUBE),
            HIT(882,3,176,118,1364,31500,14175,28500,18500,3500,EQ_TUBE_CONTACT,31200,TONE_TUBE,BODY_TUBE),
            ZHIT, ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 6800, 7900, 6200, 4400, 3300, 2000 }, 15800,
        12600, 18300, 17200, { 19, 29, 43, 59 },
        1850, 246, 34, 1, 3600, 7500, 421, 719, 227, 3800
    },
    {
        "launcher_latch", 4,
        {
            HIT(0,2,59,16,463,16000,3800,10500,23500,12500,EQ_DETENT,14500,TONE_DETENT,BODY_DETENT),
            HIT(397,2,107,118,858,22000,9900,19000,22500,7000,EQ_TUBE_CONTACT,22000,TONE_TUBE,BODY_TUBE),
            HIT(970,3,213,118,1520,30000,13500,29000,17500,3000,EQ_TUBE_CONTACT,30500,TONE_TUBE,BODY_TUBE),
            HIT(1543,1,38,8,303,10500,1875,4000,22000,23500,EQ_SPRING_EDGE,9000,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 7100, 8200, 6000, 4100, 3000, 1800 }, 16500,
        13800, 19200, 16600, { 21, 31, 47, 61 },
        2200, 265, 42, 1, 4200, 7900, 463, 773, 241, 4800
    },
    {
        "shotgun_empty", 4,
        {
            HIT(0,1,28,16,248,10000,2375,2500,22000,25000,EQ_DETENT,8500,TONE_DETENT,BODY_DETENT),
            HIT(176,1,41,24,320,14500,4169,6500,24000,19000,EQ_MAGNUM_RATCHET,13000,TONE_RATCHET,BODY_RATCHET),
            HIT(397,2,117,72,943,31500,12994,23000,23500,6500,EQ_SHOT_HAMMER,30000,TONE_HEAVY,BODY_HEAVY),
            HIT(750,1,41,8,320,9500,1750,3500,19500,27000,EQ_SPRING_EDGE,9000,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 5300, 7400, 6900, 5600, 3900, 2100 }, 15000,
        11000, 17400, 18400, { 13, 23, 37, 53 },
        1750, 198, 28, 1, 2900, 6800, 353, 587, 181, 3000
    },
    {
        "shotgun_safety", 3,
        {
            HIT(0,1,29,16,248,10000,2375,2500,22000,25500,EQ_DETENT,8500,TONE_DETENT,BODY_DETENT),
            HIT(154,1,61,18,448,23000,6038,8500,25500,14000,EQ_SAFETY_CLICK,20500,TONE_SAFETY,BODY_SAFETY),
            HIT(397,1,30,8,259,8500,1500,2500,20000,28500,EQ_SPRING_EDGE,7500,TONE_SPRING,BODY_SPRING),
            ZHIT, ZHIT, ZHIT, ZHIT, ZHIT
        },
        { 2300, 4500, 6600, 7600, 5600, 2900 }, 10500,
        7200, 13600, 22000, { 7, 12, 19, 31 },
        1600, 178, 20, 2, 1800, 5700, 251, 409, 131, 1700
    }
};

#undef ZHIT
#undef HIT
#undef EQ_PISTOL_RELEASE
#undef EQ_PISTOL_STOP
#undef EQ_SPRING_EDGE
#undef EQ_FRAME_TAP
#undef EQ_DETENT
#undef EQ_MAGNUM_RATCHET
#undef EQ_MAGNUM_HAMMER
#undef EQ_BOLT_RAIL
#undef EQ_BOLT_STOP
#undef EQ_BOLT_LOCK
#undef EQ_SMG_HAMMER
#undef EQ_TUBE_CONTACT
#undef EQ_SHOT_HAMMER
#undef EQ_SAFETY_CLICK
#undef TONE_EDGE
#undef TONE_STRIKER
#undef TONE_SPRING
#undef TONE_FRAME
#undef TONE_DETENT
#undef TONE_RATCHET
#undef TONE_HEAVY
#undef TONE_RAIL
#undef TONE_STOP
#undef TONE_LOCK
#undef TONE_MID
#undef TONE_TUBE
#undef TONE_SAFETY
#undef TONE_NONE
#undef BODY_EDGE
#undef BODY_STRIKER
#undef BODY_SPRING
#undef BODY_FRAME
#undef BODY_DETENT
#undef BODY_RATCHET
#undef BODY_HEAVY
#undef BODY_RAIL
#undef BODY_STOP
#undef BODY_LOCK
#undef BODY_MID
#undef BODY_TUBE
#undef BODY_SAFETY
#undef BODY_NONE

static gwf89_u32 gwf89_hash(gwf89_u32 x)
{
    x &= GWF89_U32_MASK;
    x ^= (x >> 16);
    x = (x * 0x7FEB352DUL) & GWF89_U32_MASK;
    x ^= (x >> 15);
    x = (x * 0x846CA68BUL) & GWF89_U32_MASK;
    x ^= (x >> 16);
    return x & GWF89_U32_MASK;
}

static unsigned short gwf89_scale_u16(unsigned short value,
                                      gwf89_s32 scale_q15)
{
    gwf89_s32 out;
    out = ((gwf89_s32)value * scale_q15 + 16384L) >> 15;
    if (value != 0U && out < 1L) out = 1L;
    if (out > 65535L) out = 65535L;
    return (unsigned short)out;
}

static gwf89_s16 gwf89_scale_q15_value(gwf89_s16 value,
                                        gwf89_s32 scale_q15)
{
    gwf89_s32 out;
    out = ((gwf89_s32)value * scale_q15) >> 15;
    if (out < 0L) out = 0L;
    if (out > 32767L) out = 32767L;
    return (gwf89_s16)out;
}

static gwf89_s32 gwf89_clamp_scale(gwf89_s32 value,
                                    gwf89_s32 low,
                                    gwf89_s32 high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static gwf89_s32 gwf89_signed_jitter(gwf89_u32 h, gwf89_s32 span)
{
    gwf89_s32 range;
    if (span <= 0L) return 0L;
    range = span * 2L + 1L;
    return (gwf89_s32)(h % (gwf89_u32)range) - span;
}

static gwf89_u32 gwf89_compute_end_runtime(const gwf89_context *ctx)
{
    gwf89_u32 end;
    gwf89_u32 e;
    unsigned int i;
    end = 0UL;
    for (i = 0U; i < (unsigned int)ctx->preset->hit_count; ++i) {
        e = (gwf89_u32)ctx->runtime_hit[i].offset_samples +
            (gwf89_u32)ctx->runtime_hit[i].attack_samples +
            (gwf89_u32)ctx->runtime_hit[i].decay_samples +
            (gwf89_u32)ctx->runtime_hit[i].hold_samples +
            (gwf89_u32)ctx->runtime_hit[i].release_samples;
        if (e > end) end = e;
    }
    return end + (gwf89_u32)ctx->preset->tail_samples;
}

static void gwf89_prepare_runtime(gwf89_context *ctx,
                                  gwf89_u32 seed,
                                  unsigned char variant,
                                  gwf89_speed speed)
{
    static const gwf89_s32 flavor_gain[GWF89_VARIANT_COUNT] = {
        33750L, 32767L, 32300L
    };
    static const gwf89_s32 flavor_cutoff[GWF89_VARIANT_COUNT] = {
        34050L, 32767L, 31750L
    };
    static const gwf89_s32 flavor_release[GWF89_VARIANT_COUNT] = {
        31450L, 32767L, 35350L
    };
    static const gwf89_s32 flavor_body[GWF89_VARIANT_COUNT] = {
        31850L, 32767L, 34400L
    };
    gwf89_s32 speed_scale;
    gwf89_s32 gain_scale;
    gwf89_s32 cutoff_scale;
    gwf89_s32 release_scale;
    gwf89_s32 body_scale;
    gwf89_s32 damping_scale;
    gwf89_s32 offset;
    gwf89_s32 jitter;
    gwf89_u32 h;
    unsigned short previous_offset;
    unsigned int i;

    if (variant >= GWF89_VARIANT_COUNT)
        variant = (unsigned char)(seed % GWF89_VARIANT_COUNT);
    if ((int)speed < (int)GWF89_SPEED_SLOW ||
        speed > GWF89_SPEED_FAST)
        speed = GWF89_SPEED_NORMAL;

    if (speed == GWF89_SPEED_SLOW) speed_scale = 38666L;
    else if (speed == GWF89_SPEED_FAST) speed_scale = 26214L;
    else speed_scale = 32767L;

    ctx->variant = variant;
    ctx->speed = (unsigned char)speed;
    previous_offset = 0U;

    for (i = 0U; i < GWF89_MAX_HITS; ++i) {
        ctx->runtime_hit[i] = ctx->preset->hit[i];
        if (i >= (unsigned int)ctx->preset->hit_count) continue;

        h = gwf89_hash(seed ^ ((gwf89_u32)i * 0x9E3779B9UL) ^
                       ((gwf89_u32)variant * 0x85EBCA6BUL) ^
                       ((gwf89_u32)ctx->speed * 0xC2B2AE35UL));

        gain_scale = flavor_gain[variant] +
                     gwf89_signed_jitter(h, 500L);
        cutoff_scale = flavor_cutoff[variant] +
                       gwf89_signed_jitter(h >> 5, 650L);
        release_scale = flavor_release[variant] +
                        gwf89_signed_jitter(h >> 11, 900L);
        body_scale = flavor_body[variant] +
                     gwf89_signed_jitter(h >> 17, 700L);

        gain_scale = gwf89_clamp_scale(gain_scale, 31457L, 34078L);
        cutoff_scale = gwf89_clamp_scale(cutoff_scale, 31130L, 34405L);
        release_scale = gwf89_clamp_scale(release_scale, 29490L, 36044L);
        body_scale = gwf89_clamp_scale(body_scale, 30800L, 34734L);
        damping_scale = cutoff_scale;
        damping_scale = gwf89_clamp_scale(damping_scale, 31130L, 34405L);

        offset = ((gwf89_s32)ctx->preset->hit[i].offset_samples *
                  speed_scale) >> 15;
        if (i != 0U) {
            jitter = gwf89_signed_jitter(h >> 23, 66L);
            if (variant == 0U) jitter -= 12L;
            else if (variant == 2U) jitter += 12L;
            offset += jitter;
            if (offset < (gwf89_s32)previous_offset + 8L)
                offset = (gwf89_s32)previous_offset + 8L;
        } else {
            offset = 0L;
        }
        if (offset > 65535L) offset = 65535L;
        ctx->runtime_hit[i].offset_samples = (unsigned short)offset;
        previous_offset = (unsigned short)offset;

        ctx->runtime_hit[i].attack_samples = gwf89_scale_u16(
            ctx->preset->hit[i].attack_samples, speed_scale);
        ctx->runtime_hit[i].decay_samples = gwf89_scale_u16(
            ctx->preset->hit[i].decay_samples, speed_scale);
        ctx->runtime_hit[i].hold_samples = gwf89_scale_u16(
            ctx->preset->hit[i].hold_samples, speed_scale);
        ctx->runtime_hit[i].release_samples = gwf89_scale_u16(
            gwf89_scale_u16(ctx->preset->hit[i].release_samples,
                            speed_scale), release_scale);

        ctx->runtime_hit[i].peak_q15 = gwf89_scale_q15_value(
            ctx->preset->hit[i].peak_q15, gain_scale);
        ctx->runtime_hit[i].sustain_q15 = gwf89_scale_q15_value(
            ctx->preset->hit[i].sustain_q15, gain_scale);

        ctx->runtime_hit[i].tone.attack_samples = gwf89_scale_u16(
            ctx->preset->hit[i].tone.attack_samples, speed_scale);
        ctx->runtime_hit[i].tone.decay_samples = gwf89_scale_u16(
            ctx->preset->hit[i].tone.decay_samples, speed_scale);
        ctx->runtime_hit[i].tone.hold_samples = gwf89_scale_u16(
            ctx->preset->hit[i].tone.hold_samples, speed_scale);
        ctx->runtime_hit[i].tone.release_samples = gwf89_scale_u16(
            gwf89_scale_u16(ctx->preset->hit[i].tone.release_samples,
                            speed_scale), release_scale);
        ctx->runtime_hit[i].tone.cutoff_base_q15 =
            gwf89_scale_q15_value(
                ctx->preset->hit[i].tone.cutoff_base_q15,
                cutoff_scale);
        ctx->runtime_hit[i].tone.cutoff_env_q15 =
            gwf89_scale_q15_value(
                ctx->preset->hit[i].tone.cutoff_env_q15,
                cutoff_scale);
        ctx->runtime_hit[i].body_wet_q15 = gwf89_scale_q15_value(
            ctx->preset->hit[i].body_wet_q15, body_scale);
        ctx->runtime_hit[i].body_feedback_q15 = gwf89_scale_q15_value(
            ctx->preset->hit[i].body_feedback_q15, body_scale);
        ctx->runtime_hit[i].body_damping_q15 = gwf89_scale_q15_value(
            ctx->preset->hit[i].body_damping_q15, damping_scale);
    }
}

void gwf89_reset(gwf89_context *ctx, gwf89_u32 seed)
{
    int i;
    int j;
    if (!ctx) return;
    ctx->preset = &gwf89_presets[0];
    ctx->rng_low = (seed ^ 0xA341316CUL) & GWF89_U32_MASK;
    ctx->rng_white = (seed ^ 0xC8013EA4UL) & GWF89_U32_MASK;
    ctx->rng_high = (seed ^ 0xAD90777DUL) & GWF89_U32_MASK;
    if (ctx->rng_low == 0UL) ctx->rng_low = 1UL;
    if (ctx->rng_white == 0UL) ctx->rng_white = 2UL;
    if (ctx->rng_high == 0UL) ctx->rng_high = 3UL;
    ctx->low_noise_state = 0;
    ctx->high_noise_prev = 0;
    for (j = 0; j < GWF89_MAX_HITS; ++j) {
        ctx->runtime_hit[j] = gwf89_presets[0].hit[j];
        for (i = 0; i < 5; ++i) ctx->hit_eq_lp[j][i] = 0;
        ctx->hit_tone_lp1[j] = 0;
        ctx->hit_tone_lp2[j] = 0;
        for (i = 0; i < GWF89_METAL_MODES; ++i)
            ctx->hit_metal_lp[j][i] = 0;
        ctx->hit_metal_write[j] = 0U;
    }
    for (i = 0; i < 5; ++i) ctx->master_eq_lp[i] = 0;
    gwf89_zero16(&ctx->hit_metal_buffer[0][0][0],
        (unsigned long)GWF89_MAX_HITS *
        (unsigned long)GWF89_METAL_MODES *
        (unsigned long)GWF89_METAL_BUFFER);
    gwf89_zero16(ctx->chorus_buffer, GWF89_CHORUS_BUFFER);
    ctx->chorus_write = 0U;
    ctx->chorus_phase = (unsigned short)(seed & 65535UL);
    gwf89_zero16(ctx->reverb_a, GWF89_REVERB_BUFFER);
    gwf89_zero16(ctx->reverb_b, GWF89_REVERB_BUFFER);
    ctx->reverb_a_pos = 0U;
    ctx->reverb_b_pos = 0U;
    gwf89_zero16(ctx->allpass, GWF89_ALLPASS_BUFFER);
    ctx->allpass_pos = 0U;
    ctx->room_send_q15 = 32767;
    ctx->last_dry = 0;
    ctx->last_room = 0;
    ctx->sample_clock = 0UL;
    ctx->end_clock = 0UL;
    ctx->variant = 0U;
    ctx->speed = (unsigned char)GWF89_SPEED_NORMAL;
    ctx->active = 0U;
}

void gwf89_init(gwf89_context *ctx, gwf89_u32 seed)
{
    gwf89_reset(ctx, seed);
}

void gwf89_trigger_ex(gwf89_context *ctx, gwf89_preset_id preset_id,
                      gwf89_u32 seed, unsigned char variant,
                      gwf89_speed speed)
{
    if (!ctx) return;
    if ((int)preset_id < 0 || preset_id >= GWF89_PRESET_COUNT)
        preset_id = GWF89_PISTOL_EMPTY;
    gwf89_reset(ctx, seed);
    ctx->preset = &gwf89_presets[(int)preset_id];
    gwf89_prepare_runtime(ctx, seed, variant, speed);
    ctx->end_clock = gwf89_compute_end_runtime(ctx);
    ctx->active = 1U;
}

void gwf89_trigger_seeded(gwf89_context *ctx, gwf89_preset_id preset_id,
                          gwf89_u32 seed)
{
    gwf89_trigger_ex(ctx, preset_id, seed, GWF89_VARIANT_AUTO,
                     GWF89_SPEED_NORMAL);
}

void gwf89_trigger(gwf89_context *ctx, gwf89_preset_id preset_id)
{
    gwf89_u32 seed;
    if (!ctx) return;
    seed = (ctx->rng_low ^ ctx->rng_white ^ ctx->rng_high ^
            ctx->sample_clock ^ 0x9E3779B9UL) & GWF89_U32_MASK;
    gwf89_trigger_ex(ctx, preset_id, seed, GWF89_VARIANT_AUTO,
                     GWF89_SPEED_NORMAL);
}

void gwf89_set_room_send(gwf89_context *ctx, gwf89_s16 room_send_q15)
{
    if (!ctx) return;
    if (room_send_q15 < 0) room_send_q15 = 0;
    ctx->room_send_q15 = room_send_q15;
}

gwf89_s16 gwf89_process_sample_stems(gwf89_context *ctx,
                                      gwf89_s16 *dry_out,
                                      gwf89_s16 *room_out)
{
    gwf89_s16 low_raw;
    gwf89_s16 white_raw;
    gwf89_s16 high_raw;
    gwf89_s16 high_diff;
    gwf89_s16 env;
    gwf89_s16 tone_env;
    gwf89_s16 mixed_hit;
    gwf89_s16 shaped;
    gwf89_s16 dry;
    gwf89_s16 room;
    gwf89_s16 mixed;
    gwf89_s32 sum;
    gwf89_s32 hit_sum;
    gwf89_u32 local;
    const gwf89_hit *hit;
    unsigned int i;

    if (!ctx || !ctx->active || !ctx->preset) {
        if (dry_out) *dry_out = 0;
        if (room_out) *room_out = 0;
        return 0;
    }

    low_raw = gwf89_noise(&ctx->rng_low);
    white_raw = gwf89_noise(&ctx->rng_white);
    high_raw = gwf89_noise(&ctx->rng_high);

    ctx->low_noise_state = gwf89_sat16(
        (gwf89_s32)ctx->low_noise_state +
        (gwf89_s32)gwf89_mul_q15(
            4200, gwf89_sat16((gwf89_s32)low_raw -
                              (gwf89_s32)ctx->low_noise_state)));
    high_diff = gwf89_sat16(((gwf89_s32)high_raw -
                             (gwf89_s32)ctx->high_noise_prev) >> 1);
    ctx->high_noise_prev = high_raw;

    hit_sum = 0L;
    for (i = 0U; i < (unsigned int)ctx->preset->hit_count; ++i) {
        hit = &ctx->runtime_hit[i];
        if (ctx->sample_clock < (gwf89_u32)hit->offset_samples) continue;
        local = ctx->sample_clock - (gwf89_u32)hit->offset_samples;
        env = gwf89_envelope(hit, local);
        mixed_hit = 0;

        if (env != 0) {
            sum = 0L;
            sum += (gwf89_s32)gwf89_mul_q15(
                ctx->low_noise_state, hit->low_mix_q15);
            sum += (gwf89_s32)gwf89_mul_q15(
                white_raw, hit->white_mix_q15);
            sum += (gwf89_s32)gwf89_mul_q15(
                high_diff, hit->high_mix_q15);
            mixed_hit = gwf89_sat16(sum);
            mixed_hit = gwf89_mul_q15(mixed_hit, env);
            tone_env = gwf89_tone_envelope(&hit->tone, local);
            mixed_hit = gwf89_particle_filter(
                ctx, i, mixed_hit, tone_env, &hit->tone);
            mixed_hit = gwf89_filterbank(
                ctx->hit_eq_lp[i], mixed_hit, hit->eq_gain_q12);
            mixed_hit = gwf89_distortion(mixed_hit, hit->drive_q15);
        }

        mixed_hit = gwf89_contact_body(ctx, i, mixed_hit, hit);
        hit_sum += (gwf89_s32)mixed_hit;
    }

    shaped = gwf89_sat16(hit_sum);
    shaped = gwf89_filterbank(ctx->master_eq_lp, shaped,
                             ctx->preset->eq_gain_q12);
    shaped = gwf89_distortion(shaped, ctx->preset->drive_q15);
    dry = gwf89_chorus(ctx, shaped);
    room = gwf89_reverb_room(ctx, dry);
    mixed = gwf89_sat16((gwf89_s32)dry +
             (gwf89_s32)gwf89_mul_q15(room, ctx->room_send_q15));

    ctx->last_dry = dry;
    ctx->last_room = room;
    if (dry_out) *dry_out = dry;
    if (room_out) *room_out = room;

    ++ctx->sample_clock;
    if (ctx->sample_clock >= ctx->end_clock) ctx->active = 0U;
    return mixed;
}

gwf89_s16 gwf89_process_sample(gwf89_context *ctx)
{
    return gwf89_process_sample_stems(ctx, (gwf89_s16 *)0,
                                      (gwf89_s16 *)0);
}

unsigned long gwf89_process(gwf89_context *ctx, gwf89_s16 *dst, unsigned long sample_count)
{
    unsigned long i;
    if (!ctx || !dst) return 0UL;
    for (i = 0UL; i < sample_count; ++i) dst[i] = gwf89_process_sample(ctx);
    return sample_count;
}

int gwf89_is_active(const gwf89_context *ctx)
{
    if (!ctx) return 0;
    return ctx->active ? 1 : 0;
}

const gwf89_preset *gwf89_get_preset(gwf89_preset_id preset_id)
{
    if ((int)preset_id < 0 || preset_id >= GWF89_PRESET_COUNT) return &gwf89_presets[0];
    return &gwf89_presets[(int)preset_id];
}

const char *gwf89_preset_name(gwf89_preset_id preset_id)
{
    return gwf89_get_preset(preset_id)->name;
}

unsigned long gwf89_context_bytes(void)
{
    return (unsigned long)sizeof(gwf89_context);
}
