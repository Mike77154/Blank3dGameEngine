#include "gtinkle89.h"

#define GT89_TABLE_BITS 8
#define GT89_TABLE_SIZE 256
#define GT89_PHASE_FRAC_BITS 8
#define GT89_PHASE_SCALE 65536UL
#define GT89_PITCH_Q12_ONE 4096U

static const gt89_s16 gt89_sine_table[GT89_TABLE_SIZE] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
    18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602,
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179,
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804,
};

/*
   Perceptual shell profiles. Frequencies are intentionally clustered and
   irregular: a casing is an open, asymmetric thin shell, not a tuned bell.
   Higher modes decay much faster than body modes.
*/
static const gt89_shell_profile gt89_shells[GT89_SHELL_COUNT] = {
    { /* rimfire brass: tiny, very bright A8-centered cluster */
      {{7040,32000,12,0},{7830,27600,12,0},{10490,20800,11,0},{13820,12800,10,0},{16870,6200,9,0},{19620,2400,8,0}},
      6, 6, 24600, 10800, 7920, 11840, 5200, 6300, 8, 7, 210 },
    { /* pistol brass: A8 root with asymmetric close modes */
      {{7040,31500,13,0},{7520,28400,12,0},{8920,22600,11,0},{11260,15000,10,0},{14580,7600,9,0},{18340,3100,8,0}},
      6, 6, 26200, 12600, 7040, 10560, 5600, 6600, 8, 7, 300 },
    { /* magnum brass: heavier body retained below an A8 dominant mode */
      {{6640,24800,14,0},{7040,31500,13,0},{8210,23400,12,0},{10540,15700,11,0},{13920,8200,10,0},{17680,3500,9,0}},
      6, 6, 27000, 13800, 6810, 10120, 5800, 6900, 8, 7, 365 },
    { /* rifle brass: close A8 doublet plus neck/rim partials */
      {{6890,27800,14,0},{7040,31500,13,0},{8740,23100,12,0},{11820,16200,11,0},{15130,8700,10,0},{19020,3800,9,0}},
      6, 6, 27400, 14300, 7160, 10840, 6100, 7050, 8, 7, 395 },
    { /* steel case: driest, brightest A8 family */
      {{7040,32000,13,0},{7980,29400,12,0},{10210,23700,11,0},{13480,16500,10,0},{16940,8900,9,0},{19920,3700,8,0}},
      6, 5, 28400, 15800, 8120, 12160, 7200, 7800, 8, 7, 330 },
    { /* shotgun brass head: broad contact with an explicit A8 ping */
      {{6120,22500,13,0},{7040,30700,12,0},{7920,22000,11,0},{10180,14900,10,0},{13400,7700,9,0},{17120,3200,8,0}},
      6, 6, 26800, 13600, 6920, 10280, 5400, 6200, 8, 7, 285 },
    { /* shotgun plastic hull: muted body plus A8 brass-edge resonance */
      {{4020,15800,11,0},{5650,20500,10,0},{7040,28200,9,0},{9190,14200,8,0},{12740,5100,7,0},{0,0,0,0}},
      5, 7, 22800, 8600, 7040, 9360, 2600, 2850, 7, 6, 205 }
};
/*
   Surfaces color the contact and add their own quiet resonances. They never
   transpose the casing's natural modes. bounce_gain is restitution-like
   amplitude retention; bounce_time shrinks the interval between contacts.
*/
static const gt89_surface_profile gt89_surfaces[GT89_SURFACE_COUNT] = {
    {28500,32767,{{310,3600,9,0},{1460,2300,8,0}},20500,23900,4,220,-1,0}, /* concrete */
    {31500,32767,{{1880,5200,10,0},{3670,2800,9,0}},23000,23500,5,246,0,0}, /* tile */
    {22000,24500,{{185,5600,11,0},{620,2800,9,0}},15800,22200,3,112,-2,0}, /* wood */
    {32767,32000,{{720,6500,12,0},{2760,7200,11,0}},24200,24200,6,252,1,0}, /* metal */
    {9000,16500,{{115,2100,8,0},{0,0,0,0}},6500,19000,1,38,-4,0}, /* dirt */
    {27000,27500,{{480,3400,10,0},{1320,2200,9,0}},18500,22800,3,180,-1,0} /* generic */
};

static const char *gt89_shell_names[GT89_SHELL_COUNT] = {
    "rimfire_brass", "pistol_brass", "magnum_brass", "rifle_brass",
    "steel_case", "shotgun_brass", "shotgun_plastic"
};

static const char *gt89_surface_names[GT89_SURFACE_COUNT] = {
    "concrete", "tile", "wood", "metal", "dirt", "generic"
};

static const char *gt89_region_names[GT89_HIT_REGION_COUNT] = {
    "random", "side", "mouth", "base", "rim"
};

/* side, mouth, base and rim mode excitation curves */
static const gt89_u16 gt89_region_mode_gain[4][GT89_PROFILE_MODES] = {
    {32767,30000,25000,18500,11500,7000},
    {12000,19500,29200,32767,28500,24500},
    {32767,25000,16500,10500,6500,3600},
    {20500,28000,32767,29200,22000,15500}
};

static const gt89_u16 gt89_region_transient_gain[4] = {
    24500, 32767, 30000, 32767
};

#if GT89_ENABLE_FM
static const gt89_u16 gt89_region_fm_gain[4] = {
    19000, 32767, 11500, 26000
};
#endif

static gt89_u16 gt89_lfsr16(gt89_u16 *state)
{
    gt89_u16 x;
    x = *state;
    if (x == 0U) x = 0xACE1U;
    x = (gt89_u16)((x >> 1) ^ ((gt89_u16)(-(gt89_s16)(x & 1U)) & 0xB400U));
    *state = x;
    return x;
}

static gt89_u16 gt89_phase_increment(gt89_u32 sample_rate, gt89_u16 hz)
{
    gt89_u32 num;
    if (sample_rate == 0UL) return 0U;
    num = (gt89_u32)hz * GT89_PHASE_SCALE;
    return (gt89_u16)(num / sample_rate);
}

static gt89_s16 gt89_sine(gt89_u16 phase)
{
    return gt89_sine_table[(phase >> GT89_PHASE_FRAC_BITS) & 255U];
}

static gt89_u16 gt89_decay(gt89_u16 env, gt89_u8 shift)
{
    gt89_u16 d;
    if (env == 0U) return 0U;
    if (shift > 15U) shift = 15U;
    d = (gt89_u16)(env >> shift);
    d = (gt89_u16)(d + 1U);
    if (d >= env) return 0U;
    return (gt89_u16)(env - d);
}

static gt89_s16 gt89_clip16(gt89_s32 x)
{
    if (x > 32767L) return 32767;
    if (x < -32768L) return -32768;
    return (gt89_s16)x;
}

static gt89_s32 gt89_mul_q15(gt89_s32 a, gt89_s32 b)
{
    return (a * b) >> 15;
}

static gt89_u32 gt89_pair_get(gt89_u16 low, gt89_u16 high)
{
    return ((gt89_u32)high << 16) | (gt89_u32)low;
}

static void gt89_pair_set(gt89_u16 *low, gt89_u16 *high, gt89_u32 value)
{
    *low = (gt89_u16)(value & 0xFFFFUL);
    *high = (gt89_u16)((value >> 16) & 0xFFFFUL);
}

#if GT89_ENABLE_INTERNAL_BOUNCES
static void gt89_pair_decrement(gt89_u16 *low, gt89_u16 *high)
{
    if (*low == 0U) {
        if (*high == 0U) return;
        *high = (gt89_u16)(*high - 1U);
        *low = 0xFFFFU;
    } else {
        *low = (gt89_u16)(*low - 1U);
    }
}
#endif

static gt89_u32 gt89_voice_age(const gt89_voice *v)
{
    return gt89_pair_get(v->age_samples_low, v->age_samples_high);
}

static gt89_u32 gt89_voice_duration(const gt89_voice *v)
{
    return gt89_pair_get(v->duration_samples_low, v->duration_samples_high);
}

static void gt89_increment_age(gt89_voice *v)
{
    v->age_samples_low = (gt89_u16)(v->age_samples_low + 1U);
    if (v->age_samples_low == 0U) {
        v->age_samples_high = (gt89_u16)(v->age_samples_high + 1U);
    }
}

static int gt89_find_voice(gt89_context *ctx)
{
    int i;
    int oldest;
    gt89_u32 oldest_age;
    oldest = 0;
    oldest_age = 0UL;
    for (i = 0; i < GT89_MAX_VOICES; ++i) {
        if (!ctx->voice[i].active) return i;
        if (gt89_voice_age(&ctx->voice[i]) >= oldest_age) {
            oldest_age = gt89_voice_age(&ctx->voice[i]);
            oldest = i;
        }
    }
    return oldest;
}

static gt89_u16 gt89_jitter_hz(gt89_u16 hz, gt89_s16 jitter_q10)
{
    gt89_s32 delta;
    gt89_s32 out;
    delta = ((gt89_s32)hz * (gt89_s32)jitter_q10) >> 10;
    out = (gt89_s32)hz + delta;
    if (out < 20L) out = 20L;
    if (out > 20000L) out = 20000L;
    return (gt89_u16)out;
}

static gt89_u8 gt89_resolve_region(gt89_u8 region, gt89_u16 *rng)
{
    if (region >= (gt89_u8)GT89_HIT_REGION_COUNT) region = (gt89_u8)GT89_HIT_RANDOM;
    if (region == (gt89_u8)GT89_HIT_RANDOM) {
        gt89_lfsr16(rng);
        region = (gt89_u8)(1U + (*rng & 3U));
    }
    return region;
}

static gt89_u16 gt89_scale_u16(gt89_u16 value, gt89_u16 gain_q15)
{
    return (gt89_u16)gt89_mul_q15((gt89_s32)value, (gt89_s32)gain_q15);
}

int gt89_init(gt89_context *ctx, gt89_u32 sample_rate, gt89_u16 seed)
{
    if (ctx == 0) return 0;
    if (sizeof(gt89_u8) != 1U || sizeof(gt89_u16) != 2U ||
        sizeof(gt89_u32) != 4U || sizeof(gt89_s32) != 4U || sizeof(gt89_s16) != 2U) return 0;
    if (sample_rate < 8000UL || sample_rate > 96000UL) return 0;
    ctx->sample_rate = sample_rate;
    ctx->seed = seed ? seed : 0xA17DU;
    ctx->master_gain_q15 = 24576U;
    ctx->trigger_counter = 0UL;
    ctx->initialized = 1U;
    gt89_reset(ctx);
    return 1;
}

void gt89_reset(gt89_context *ctx)
{
    int i;
    int j;
    if (ctx == 0) return;
    for (i = 0; i < GT89_MAX_VOICES; ++i) {
        gt89_voice *v;
        v = &ctx->voice[i];
        for (j = 0; j < GT89_RUNTIME_MODES; ++j) {
            v->mode[j].phase = 0U;
            v->mode[j].increment = 0U;
            v->mode[j].envelope_q15 = 0U;
            v->mode[j].amplitude_q15 = 0U;
            v->mode[j].decay_shift = 0U;
            v->mode[j].reserved = 0U;
        }
        v->fm_carrier_phase = 0U;
        v->fm_carrier_increment = 0U;
        v->fm_modulator_phase = 0U;
        v->fm_modulator_increment = 0U;
        v->fm_envelope_q15 = 0U;
        v->fm_index_envelope_q15 = 0U;
        v->fm_amplitude_q15 = 0U;
        v->fm_index_phase = 0U;
        v->transient_envelope_q15 = 0U;
        v->transient_amplitude_q15 = 0U;
        v->click_amplitude_q15 = 0U;
        v->noise_previous = 0U;
        v->age_samples_low = 0U;
        v->age_samples_high = 0U;
        v->duration_samples_low = 0U;
        v->duration_samples_high = 0U;
        v->bounce_countdown_low = 0U;
        v->bounce_countdown_high = 0U;
        v->bounce_interval_low = 0U;
        v->bounce_interval_high = 0U;
        v->bounce_gain_q15 = 0U;
        v->bounce_time_q15 = 0U;
        v->rng = (gt89_u16)(ctx->seed + (gt89_u16)(i * 977U));
        v->pan = 0;
        v->mode_count = 0U;
        v->fm_amplitude_decay_shift = 0U;
        v->fm_index_decay_shift = 0U;
        v->transient_decay_shift = 0U;
        v->click_samples_left = 0U;
        v->bounce_remaining = 0U;
        v->angular_velocity = 0U;
        v->hardness = 0U;
        v->hit_region = 0U;
        v->active = 0U;
    }
    ctx->trigger_counter = 0UL;
}

void gt89_set_master_gain(gt89_context *ctx, gt89_u16 gain_q15)
{
    if (ctx == 0) return;
    if (gain_q15 > 32767U) gain_q15 = 32767U;
    ctx->master_gain_q15 = gain_q15;
}

void gt89_default_impact_params(gt89_impact_params *params)
{
    if (params == 0) return;
    params->velocity = 192U;
    params->angular_velocity = 96U;
    params->bounce_count = 0U;
    params->variation = 0U;
    params->pan = 0;
    params->hit_region = (gt89_u8)GT89_HIT_RANDOM;
    params->first_bounce_ms = 0U;
    params->bounce_gain_q15 = 0U;
    params->bounce_time_q15 = 0U;
}

const gt89_shell_profile *gt89_get_shell_profile(gt89_shell_type shell)
{
    if ((int)shell < 0 || shell >= GT89_SHELL_COUNT) return 0;
    return &gt89_shells[(int)shell];
}

const gt89_surface_profile *gt89_get_surface_profile(gt89_surface_type surface)
{
    if ((int)surface < 0 || surface >= GT89_SURFACE_COUNT) return 0;
    return &gt89_surfaces[(int)surface];
}

const char *gt89_shell_name(gt89_shell_type shell)
{
    if ((int)shell < 0 || shell >= GT89_SHELL_COUNT) return "invalid";
    return gt89_shell_names[(int)shell];
}

const char *gt89_surface_name(gt89_surface_type surface)
{
    if ((int)surface < 0 || surface >= GT89_SURFACE_COUNT) return "invalid";
    return gt89_surface_names[(int)surface];
}

const char *gt89_hit_region_name(gt89_hit_region region)
{
    if ((int)region < 0 || region >= GT89_HIT_REGION_COUNT) return "invalid";
    return gt89_region_names[(int)region];
}

int gt89_trigger(gt89_context *ctx,
                 gt89_shell_type shell,
                 gt89_surface_type surface,
                 gt89_u8 velocity,
                 gt89_s8 pan,
                 gt89_u8 variation)
{
    gt89_impact_params params;
    gt89_default_impact_params(&params);
    params.velocity = velocity;
    params.pan = pan;
    params.variation = variation;
    params.bounce_count = 0U;
    return gt89_trigger_ex(ctx, shell, surface, &params);
}

int gt89_trigger_ex(gt89_context *ctx,
                    gt89_shell_type shell,
                    gt89_surface_type surface,
                    const gt89_impact_params *params)
{
    const gt89_shell_profile *sp;
    const gt89_surface_profile *fp;
    sp = gt89_get_shell_profile(shell);
    fp = gt89_get_surface_profile(surface);
    if (sp == 0 || fp == 0) return -1;
    return gt89_trigger_custom_ex(ctx, sp, fp, params);
}

int gt89_trigger_drop(gt89_context *ctx,
                      gt89_shell_type shell,
                      gt89_surface_type surface,
                      gt89_u8 velocity,
                      gt89_u8 angular_velocity,
                      gt89_s8 pan,
                      gt89_u8 variation)
{
    gt89_impact_params params;
    gt89_default_impact_params(&params);
    params.velocity = velocity;
    params.angular_velocity = angular_velocity;
    params.bounce_count = GT89_AUTO_U8;
    params.variation = variation;
    params.pan = pan;
    params.hit_region = (gt89_u8)GT89_HIT_RANDOM;
    return gt89_trigger_ex(ctx, shell, surface, &params);
}

int gt89_trigger_custom(gt89_context *ctx,
                        const gt89_shell_profile *shell,
                        const gt89_surface_profile *surface,
                        gt89_u8 velocity,
                        gt89_s8 pan,
                        gt89_u8 variation)
{
    gt89_impact_params params;
    gt89_default_impact_params(&params);
    params.velocity = velocity;
    params.pan = pan;
    params.variation = variation;
    params.bounce_count = 0U;
    return gt89_trigger_custom_ex(ctx, shell, surface, &params);
}

int gt89_trigger_custom_ex(gt89_context *ctx,
                           const gt89_shell_profile *shell,
                           const gt89_surface_profile *surface,
                           const gt89_impact_params *params)
{
    int index;
    int i;
    gt89_voice *v;
    gt89_u16 rng;
    gt89_s16 pitch_jitter_q10;
    gt89_u16 velocity_q15;
    gt89_u16 spin_brightness_q15;
    gt89_u32 duration_samples;
    gt89_u32 first_interval;
    gt89_u32 interval;
    gt89_u32 total_bounce_time;
    gt89_u8 count;
    gt89_u8 floor_count;
    gt89_u8 region;
    gt89_u8 bounce_count;
#if GT89_ENABLE_INTERNAL_BOUNCES
    gt89_u16 bounce_gain;
#endif
    gt89_u16 bounce_time;

    if (ctx == 0 || shell == 0 || surface == 0 || params == 0 || !ctx->initialized) return -1;
    if (params->velocity == 0U) return -1;
    index = gt89_find_voice(ctx);
    v = &ctx->voice[index];
    rng = (gt89_u16)(ctx->seed ^ (gt89_u16)ctx->trigger_counter ^
                     (gt89_u16)((gt89_u16)params->variation << 8) ^ (gt89_u16)(index * 131U));
    if (rng == 0U) rng = 0xBEEFU;
    region = gt89_resolve_region(params->hit_region, &rng);
    gt89_lfsr16(&rng);
    pitch_jitter_q10 = (gt89_s16)((gt89_s16)(rng & 31U) - 15);
    pitch_jitter_q10 = (gt89_s16)(pitch_jitter_q10 +
        (gt89_s16)(((gt89_s16)(params->angular_velocity >> 5)) - 3));
    velocity_q15 = (gt89_u16)(((gt89_u32)params->velocity * 32767UL) / 255UL);
    spin_brightness_q15 = (gt89_u16)(24576U + ((gt89_u16)params->angular_velocity << 5));
    if (spin_brightness_q15 > 32767U) spin_brightness_q15 = 32767U;

    count = shell->mode_count;
    if (count > GT89_PROFILE_MODES) count = GT89_PROFILE_MODES;
    if (count > GT89_MAX_MODES) count = GT89_MAX_MODES;

    for (i = 0; i < GT89_RUNTIME_MODES; ++i) {
        v->mode[i].phase = 0U;
        v->mode[i].increment = 0U;
        v->mode[i].envelope_q15 = 0U;
        v->mode[i].amplitude_q15 = 0U;
        v->mode[i].decay_shift = 0U;
        v->mode[i].reserved = 0U;
    }

    for (i = 0; i < (int)count; ++i) {
        gt89_u16 hz;
        gt89_s16 shift;
        gt89_u16 amp;
        gt89_u16 region_gain;
        gt89_u16 mode_spin_gain;
        gt89_lfsr16(&rng);
        hz = gt89_jitter_hz(shell->mode[i].hz,
                            (gt89_s16)(pitch_jitter_q10 + (gt89_s16)((rng & 7U) - 3U)));
        v->mode[i].phase = rng;
        v->mode[i].increment = gt89_phase_increment(ctx->sample_rate, hz);
        region_gain = gt89_region_mode_gain[(int)region - 1][i];
        mode_spin_gain = 32767U;
        if (i >= 3) mode_spin_gain = spin_brightness_q15;
        amp = gt89_scale_u16(shell->mode[i].amplitude_q15, surface->mode_gain_q15);
        amp = gt89_scale_u16(amp, region_gain);
        amp = gt89_scale_u16(amp, mode_spin_gain);
        amp = gt89_scale_u16(amp, velocity_q15);
        v->mode[i].amplitude_q15 = amp;
        v->mode[i].envelope_q15 = 32767U;
        shift = (gt89_s16)shell->mode[i].decay_shift + (gt89_s16)surface->decay_shift_delta;
        if (shift < 7) shift = 7;
        if (shift > 15) shift = 15;
        v->mode[i].decay_shift = (gt89_u8)shift;
    }

    floor_count = 0U;
#if GT89_MAX_FLOOR_MODES > 0
    for (i = 0; i < GT89_PROFILE_FLOOR_MODES && floor_count < GT89_MAX_FLOOR_MODES; ++i) {
        const gt89_mode_profile *fp;
        gt89_runtime_mode *m;
        gt89_u16 amp;
        fp = &surface->floor_mode[i];
        if (fp->hz == 0U || fp->amplitude_q15 == 0U) continue;
        m = &v->mode[(int)count + (int)floor_count];
        gt89_lfsr16(&rng);
        m->phase = rng;
        m->increment = gt89_phase_increment(ctx->sample_rate,
                         gt89_jitter_hz(fp->hz, (gt89_s16)(pitch_jitter_q10 >> 1)));
        amp = gt89_scale_u16(fp->amplitude_q15, velocity_q15);
        m->amplitude_q15 = amp;
        m->envelope_q15 = 32767U;
        m->decay_shift = fp->decay_shift;
        floor_count = (gt89_u8)(floor_count + 1U);
    }
#endif
    v->mode_count = (gt89_u8)(count + floor_count);

#if GT89_ENABLE_FM
    v->fm_carrier_phase = (gt89_u16)(rng ^ 0x3333U);
    v->fm_modulator_phase = (gt89_u16)(rng ^ 0xCCCCU);
    v->fm_carrier_increment = gt89_phase_increment(ctx->sample_rate,
        gt89_jitter_hz(shell->fm_carrier_hz, pitch_jitter_q10));
    v->fm_modulator_increment = gt89_phase_increment(ctx->sample_rate,
        gt89_jitter_hz(shell->fm_modulator_hz, (gt89_s16)-pitch_jitter_q10));
    v->fm_amplitude_q15 = gt89_scale_u16(shell->fm_amplitude_q15,
        gt89_region_fm_gain[(int)region - 1]);
    v->fm_amplitude_q15 = gt89_scale_u16(v->fm_amplitude_q15, velocity_q15);
    v->fm_amplitude_q15 = gt89_scale_u16(v->fm_amplitude_q15, spin_brightness_q15);
    v->fm_index_phase = shell->fm_index_phase;
    v->fm_envelope_q15 = 32767U;
    v->fm_index_envelope_q15 = 32767U;
    v->fm_amplitude_decay_shift = shell->fm_amplitude_decay_shift;
    v->fm_index_decay_shift = shell->fm_index_decay_shift;
#else
    v->fm_envelope_q15 = 0U;
    v->fm_index_envelope_q15 = 0U;
    v->fm_amplitude_q15 = 0U;
    v->fm_index_phase = 0U;
    v->fm_amplitude_decay_shift = 0U;
    v->fm_index_decay_shift = 0U;
#endif

    v->transient_envelope_q15 = 32767U;
    v->transient_amplitude_q15 = gt89_scale_u16(shell->transient_amplitude_q15,
                                                surface->transient_gain_q15);
    v->transient_amplitude_q15 = gt89_scale_u16(v->transient_amplitude_q15,
        gt89_region_transient_gain[(int)region - 1]);
    v->transient_amplitude_q15 = gt89_scale_u16(v->transient_amplitude_q15, velocity_q15);
    v->transient_decay_shift = shell->transient_decay_shift;
    v->click_amplitude_q15 = gt89_scale_u16(shell->click_amplitude_q15,
                                             surface->transient_gain_q15);
    v->click_amplitude_q15 = gt89_scale_u16(v->click_amplitude_q15, velocity_q15);
    v->click_samples_left = 3U;
    v->noise_previous = rng;
    v->rng = rng;
    v->pan = params->pan;
    v->angular_velocity = params->angular_velocity;
    v->hardness = surface->hardness;
    v->hit_region = region;
    v->age_samples_low = 0U;
    v->age_samples_high = 0U;

#if GT89_ENABLE_INTERNAL_BOUNCES
    bounce_count = params->bounce_count;
    if (bounce_count == GT89_AUTO_U8) {
        bounce_count = (gt89_u8)(((gt89_u16)surface->max_bounces *
                                  (gt89_u16)params->velocity + 127U) / 255U);
        if (surface->max_bounces > 0U && params->velocity > 72U && bounce_count == 0U)
            bounce_count = 1U;
    }
    if (bounce_count > surface->max_bounces + 2U) bounce_count = (gt89_u8)(surface->max_bounces + 2U);
    bounce_gain = params->bounce_gain_q15 ? params->bounce_gain_q15 : surface->bounce_gain_q15;
    bounce_time = params->bounce_time_q15 ? params->bounce_time_q15 : surface->bounce_time_q15;
    if (params->first_bounce_ms != 0U) {
        first_interval = ((gt89_u32)params->first_bounce_ms * ctx->sample_rate) / 1000UL;
    } else {
        gt89_u32 auto_ms;
        auto_ms = 26UL + ((gt89_u32)params->velocity * 72UL) / 255UL;
        auto_ms -= ((gt89_u32)params->angular_velocity * 12UL) / 255UL;
        if (auto_ms < 18UL) auto_ms = 18UL;
        first_interval = (auto_ms * ctx->sample_rate) / 1000UL;
    }
    if (first_interval < 32UL) first_interval = 32UL;
    v->bounce_remaining = bounce_count;
    v->bounce_gain_q15 = bounce_gain;
    v->bounce_time_q15 = bounce_time;
    gt89_pair_set(&v->bounce_interval_low, &v->bounce_interval_high, first_interval);
    gt89_pair_set(&v->bounce_countdown_low, &v->bounce_countdown_high, first_interval);
#else
    bounce_count = 0U;
    first_interval = 0UL;
    bounce_time = 0U;
    v->bounce_remaining = 0U;
    v->bounce_gain_q15 = 0U;
    v->bounce_time_q15 = 0U;
    gt89_pair_set(&v->bounce_interval_low, &v->bounce_interval_high, 0UL);
    gt89_pair_set(&v->bounce_countdown_low, &v->bounce_countdown_high, 0UL);
#endif

    duration_samples = ((gt89_u32)shell->nominal_duration_ms * ctx->sample_rate) / 1000UL;
    total_bounce_time = 0UL;
    interval = first_interval;
    for (i = 0; i < (int)bounce_count; ++i) {
        total_bounce_time += interval;
        interval = ((gt89_u32)interval * (gt89_u32)bounce_time) >> 15;
        if (interval < 32UL) interval = 32UL;
    }
    duration_samples += total_bounce_time + ctx->sample_rate / 8UL;
    if (duration_samples < 64UL) duration_samples = 64UL;
    if (duration_samples > ctx->sample_rate * 2UL) duration_samples = ctx->sample_rate * 2UL;
    gt89_pair_set(&v->duration_samples_low, &v->duration_samples_high, duration_samples);
    v->active = 1U;
    ctx->trigger_counter += 1UL;
    return index;
}

#if GT89_ENABLE_INTERNAL_BOUNCES
static void gt89_retrigger_bounce(gt89_voice *v)
{
    int i;
    gt89_u32 interval;
    gt89_s16 drift;
    gt89_u16 local_gain;
    if (v->bounce_remaining == 0U) return;
    local_gain = v->bounce_gain_q15;
    gt89_lfsr16(&v->rng);
    local_gain = (gt89_u16)(((gt89_u32)local_gain *
                 (gt89_u32)(28672U + (v->rng & 4095U))) >> 15);
    for (i = 0; i < (int)v->mode_count; ++i) {
        gt89_runtime_mode *m;
        m = &v->mode[i];
        m->amplitude_q15 = gt89_scale_u16(m->amplitude_q15, local_gain);
        if (m->amplitude_q15 > 24U) {
            m->envelope_q15 = (gt89_u16)(24576U + (v->rng & 8191U));
            gt89_lfsr16(&v->rng);
            m->phase = (gt89_u16)(m->phase + v->rng);
        }
    }
    v->fm_amplitude_q15 = gt89_scale_u16(v->fm_amplitude_q15, local_gain);
    if (v->fm_amplitude_q15 > 20U) {
        v->fm_envelope_q15 = 24576U;
        v->fm_index_envelope_q15 = 24576U;
    }
    v->transient_amplitude_q15 = gt89_scale_u16(v->transient_amplitude_q15, local_gain);
    v->transient_envelope_q15 = 32767U;
    v->click_amplitude_q15 = gt89_scale_u16(v->click_amplitude_q15, local_gain);
    v->click_samples_left = 3U;
    v->noise_previous = v->rng;
    drift = (gt89_s16)((gt89_s16)(v->rng & 15U) - 7);
    drift = (gt89_s16)((drift * (gt89_s16)(v->angular_velocity + 32U)) >> 7);
    if ((gt89_s16)v->pan + drift < -127) v->pan = -127;
    else if ((gt89_s16)v->pan + drift > 127) v->pan = 127;
    else v->pan = (gt89_s8)((gt89_s16)v->pan + drift);
    v->bounce_remaining = (gt89_u8)(v->bounce_remaining - 1U);
    interval = gt89_pair_get(v->bounce_interval_low, v->bounce_interval_high);
    interval = ((gt89_u32)interval * (gt89_u32)v->bounce_time_q15) >> 15;
    if (interval < 32UL) interval = 32UL;
    gt89_pair_set(&v->bounce_interval_low, &v->bounce_interval_high, interval);
    gt89_pair_set(&v->bounce_countdown_low, &v->bounce_countdown_high, interval);
}

static void gt89_update_bounce(gt89_voice *v)
{
    if (v->bounce_remaining == 0U) return;
    if (gt89_pair_get(v->bounce_countdown_low, v->bounce_countdown_high) == 0UL) {
        gt89_retrigger_bounce(v);
    } else {
        gt89_pair_decrement(&v->bounce_countdown_low, &v->bounce_countdown_high);
    }
}
#endif

static gt89_s32 gt89_render_voice(gt89_voice *v)
{
    int i;
    gt89_s32 out;
    gt89_u8 any;
    out = 0L;
    any = 0U;

#if GT89_ENABLE_INTERNAL_BOUNCES
    gt89_update_bounce(v);
#endif

    for (i = 0; i < (int)v->mode_count; ++i) {
        gt89_runtime_mode *m;
        gt89_s32 s;
        m = &v->mode[i];
        if (m->envelope_q15 != 0U && m->amplitude_q15 != 0U) {
            s = gt89_sine(m->phase);
            s = gt89_mul_q15(s, m->envelope_q15);
            s = gt89_mul_q15(s, m->amplitude_q15);
            out += s;
            m->phase = (gt89_u16)(m->phase + m->increment);
            m->envelope_q15 = gt89_decay(m->envelope_q15, m->decay_shift);
            if (m->envelope_q15 != 0U) any = 1U;
        }
    }

#if GT89_ENABLE_FM
    if (v->fm_envelope_q15 != 0U && v->fm_amplitude_q15 != 0U) {
        gt89_s32 mod;
        gt89_s32 offset;
        gt89_s32 s;
        gt89_u16 warped;
        mod = gt89_sine(v->fm_modulator_phase);
        mod = gt89_mul_q15(mod, v->fm_index_envelope_q15);
        offset = gt89_mul_q15(mod, v->fm_index_phase);
        warped = (gt89_u16)(v->fm_carrier_phase + (gt89_s16)offset);
        s = gt89_sine(warped);
        s = gt89_mul_q15(s, v->fm_envelope_q15);
        s = gt89_mul_q15(s, v->fm_amplitude_q15);
        out += s;
        v->fm_carrier_phase = (gt89_u16)(v->fm_carrier_phase + v->fm_carrier_increment);
        v->fm_modulator_phase = (gt89_u16)(v->fm_modulator_phase + v->fm_modulator_increment);
        v->fm_envelope_q15 = gt89_decay(v->fm_envelope_q15, v->fm_amplitude_decay_shift);
        v->fm_index_envelope_q15 = gt89_decay(v->fm_index_envelope_q15, v->fm_index_decay_shift);
        if (v->fm_envelope_q15 != 0U) any = 1U;
    }
#endif

    if (v->transient_envelope_q15 != 0U && v->transient_amplitude_q15 != 0U) {
        gt89_s32 noise;
        gt89_s32 previous;
        gt89_s32 hp;
        gt89_s32 raw;
        gt89_s32 colored;
        gt89_lfsr16(&v->rng);
        noise = (gt89_s32)((gt89_s16)v->rng);
        previous = (gt89_s32)((gt89_s16)v->noise_previous);
        hp = (noise - previous) >> 1;
        raw = noise >> 2;
        v->noise_previous = v->rng;
        colored = ((hp * (gt89_s32)v->hardness) +
                   (raw * (gt89_s32)(255U - v->hardness))) >> 8;
        colored = gt89_mul_q15(colored, v->transient_envelope_q15);
        colored = gt89_mul_q15(colored, v->transient_amplitude_q15);
        out += colored;
        v->transient_envelope_q15 = gt89_decay(v->transient_envelope_q15,
                                                v->transient_decay_shift);
        if (v->transient_envelope_q15 != 0U) any = 1U;
    }

    if (v->click_samples_left != 0U && v->click_amplitude_q15 != 0U) {
        gt89_s32 click;
        if (v->click_samples_left == 3U) click = (gt89_s32)v->click_amplitude_q15;
        else if (v->click_samples_left == 2U) click = -((gt89_s32)v->click_amplitude_q15 * 3L / 5L);
        else click = (gt89_s32)v->click_amplitude_q15 / 4L;
        out += click;
        v->click_samples_left = (gt89_u8)(v->click_samples_left - 1U);
        any = 1U;
    }

    gt89_increment_age(v);
    if ((!any && v->bounce_remaining == 0U) || gt89_voice_age(v) >= gt89_voice_duration(v))
        v->active = 0U;
    return out;
}

static void gt89_render_frame(gt89_context *ctx, gt89_s32 *left, gt89_s32 *right)
{
    int i;
    gt89_s32 l;
    gt89_s32 r;
    l = 0L;
    r = 0L;
    for (i = 0; i < GT89_MAX_VOICES; ++i) {
        gt89_voice *v;
        gt89_s32 s;
        gt89_s32 lg;
        gt89_s32 rg;
        v = &ctx->voice[i];
        if (!v->active) continue;
        s = gt89_render_voice(v);
        if (s > 65535L) s = 65535L;
        if (s < -65536L) s = -65536L;
        s = gt89_mul_q15(s, ctx->master_gain_q15);
        lg = 127L - (gt89_s32)v->pan;
        rg = 127L + (gt89_s32)v->pan;
        l += (s * lg) >> 8;
        r += (s * rg) >> 8;
    }
    *left = l;
    *right = r;
}

void gt89_render_mono_i16(gt89_context *ctx, gt89_s16 *output, gt89_u32 frames)
{
    gt89_u32 i;
    if (ctx == 0 || output == 0) return;
    for (i = 0UL; i < frames; ++i) {
        gt89_s32 l;
        gt89_s32 r;
        gt89_render_frame(ctx, &l, &r);
        output[i] = gt89_clip16((l + r) >> 1);
    }
}

void gt89_render_stereo_i16(gt89_context *ctx, gt89_s16 *output_interleaved, gt89_u32 frames)
{
    gt89_u32 i;
    if (ctx == 0 || output_interleaved == 0) return;
    for (i = 0UL; i < frames; ++i) {
        gt89_s32 l;
        gt89_s32 r;
        gt89_render_frame(ctx, &l, &r);
        output_interleaved[i * 2UL] = gt89_clip16(l);
        output_interleaved[i * 2UL + 1UL] = gt89_clip16(r);
    }
}

void gt89_mix_mono_i16(gt89_context *ctx, gt89_s16 *output, gt89_u32 frames)
{
    gt89_u32 i;
    if (ctx == 0 || output == 0) return;
    for (i = 0UL; i < frames; ++i) {
        gt89_s32 l;
        gt89_s32 r;
        gt89_render_frame(ctx, &l, &r);
        output[i] = gt89_clip16((gt89_s32)output[i] + ((l + r) >> 1));
    }
}

void gt89_mix_stereo_i16(gt89_context *ctx, gt89_s16 *output_interleaved, gt89_u32 frames)
{
    gt89_u32 i;
    if (ctx == 0 || output_interleaved == 0) return;
    for (i = 0UL; i < frames; ++i) {
        gt89_s32 l;
        gt89_s32 r;
        gt89_render_frame(ctx, &l, &r);
        output_interleaved[i * 2UL] = gt89_clip16((gt89_s32)output_interleaved[i * 2UL] + l);
        output_interleaved[i * 2UL + 1UL] = gt89_clip16((gt89_s32)output_interleaved[i * 2UL + 1UL] + r);
    }
}

gt89_u32 gt89_active_voice_count(const gt89_context *ctx)
{
    int i;
    gt89_u32 count;
    if (ctx == 0) return 0UL;
    count = 0UL;
    for (i = 0; i < GT89_MAX_VOICES; ++i) {
        if (ctx->voice[i].active) count += 1UL;
    }
    return count;
}

gt89_u32 gt89_context_bytes(void)
{
    return (gt89_u32)sizeof(gt89_context);
}
