#include "wsound_ggrenadeblast89.h"

static ws_gs16 ws_ggb89_sine_table[256] = {
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
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};

static ws_gu32 ws_ggb89_rng(ws_gu32 *state)
{
    ws_gu32 v;

    v = ((*state * 1664525UL) + 1013904223UL) & WS_GU32_MASK;
    *state = v;
    return v;
}

static ws_gs16 ws_ggb89_noise(ws_gu32 *state)
{
    ws_gu32 v;

    v = ws_ggb89_rng(state);
    return (ws_gs16)((ws_gs32)((v >> 16) & 0xFFFFUL) - 32768L);
}

static ws_gu32 ws_ggb89_ms_to_samples(ws_gu32 sample_rate, ws_gu16 ms)
{
    ws_gu32 n;

    n = (sample_rate * (ws_gu32)ms) / 1000UL;
    if (n < 1UL) {
        n = 1UL;
    }
    return n;
}

static ws_gs16 ws_ggb89_env_coef(ws_gu32 samples)
{
    ws_gs32 drop;
    ws_gs32 coef;

    if (samples < 2UL) {
        return 0;
    }
    drop = 181700L / (ws_gs32)samples;
    if (drop < 1L) {
        drop = 1L;
    }
    coef = 32767L - drop;
    if (coef < 0L) {
        coef = 0L;
    }
    if (coef > 32766L) {
        coef = 32766L;
    }
    return (ws_gs16)coef;
}

static ws_gu32 ws_ggb89_phase_inc(ws_gu32 sample_rate, ws_gu16 hz)
{
    ws_gu32 upper;

    if (sample_rate < 8000UL) {
        sample_rate = 8000UL;
    }
    upper = ((ws_gu32)hz * 65536UL) / sample_rate;
    return (upper << 16) & WS_GU32_MASK;
}

static ws_gs16 ws_ggb89_scale_q15(ws_gs16 value, ws_gs16 scale)
{
    return ws_gclip16(ws_gmul_q15(value, scale));
}

/*
 * Soft-knee bus protection.  The old path hard-clipped the seven-layer
 * accumulator before EQ.  That clipping was hidden by later gain reduction,
 * but remained audible as brittle static on small speakers.
 */
static ws_gs16 ws_ggb89_bus_softlimit(ws_gs32 x)
{
    return ws_gsoftlimit16(x);
}

static void ws_ggb89_apply_intensity(ws_ggb89_params *p, ws_gs16 intensity_q15)
{
    if (intensity_q15 < 0) {
        intensity_q15 = 0;
    }
    p->main_noise_q15 = ws_ggb89_scale_q15(p->main_noise_q15, intensity_q15);
    p->low_noise_q15 = ws_ggb89_scale_q15(p->low_noise_q15, intensity_q15);
    p->high_noise_q15 = ws_ggb89_scale_q15(p->high_noise_q15, intensity_q15);
    p->fragment_noise_q15 = ws_ggb89_scale_q15(p->fragment_noise_q15, intensity_q15);
    p->debris_noise_q15 = ws_ggb89_scale_q15(p->debris_noise_q15, intensity_q15);
    p->sine_q15 = ws_ggb89_scale_q15(p->sine_q15, intensity_q15);
    p->saw_q15 = ws_ggb89_scale_q15(p->saw_q15, intensity_q15);
    p->negative_q15 = ws_ggb89_scale_q15(p->negative_q15, intensity_q15);
}

static ws_ggb89_params ws_ggb89_presets[WS_GGB89_PRESET_COUNT] = {
    {
        12, 75, 330, 850, 1900,
        24500, 22000, 10500, 11500, 8500, 16500, 7600, 8500,
        82, 42, 56, 30,
        240, 260,
        { 6200, 5600, 4500, 4300, 4200, 2100 },
        390, 9000,
        120, 20, 310, 1800, 800,
        18000, 17000, 700,
        20500
    },
    {
        9, 58, 250, 650, 1500,
        25500, 18500, 13500, 10500, 6500, 14000, 6200, 7200,
        96, 50, 67, 37,
        170, 190,
        { 5600, 5100, 4600, 4700, 4500, 2500 },
        430, 9800,
        110, 16, 360, 1500, 500,
        18000, 17000, 650,
        21000
    },
    {
        8, 52, 240, 720, 1600,
        24500, 16500, 17500, 17000, 9500, 12500, 5800, 6800,
        100, 52, 70, 38,
        340, 310,
        { 4600, 4400, 4400, 5100, 6100, 4700 },
        450, 11000,
        105, 14, 400, 1400, 400,
        19500, 17000, 1100,
        20200
    },
    {
        15, 105, 440, 1450, 2900,
        23500, 22500, 12500, 12000, 12000, 17500, 10500, 11000,
        76, 36, 50, 25,
        190, 330,
        { 6500, 6100, 5200, 4700, 4100, 2700 },
        420, 9000,
        155, 22, 270, 2200, 900,
        26800, 12000, 8200,
        19000
    },
    {
        9, 62, 280, 1000, 2050,
        25000, 17500, 18500, 19500, 14500, 13500, 6400, 7200,
        90, 44, 60, 31,
        390, 420,
        { 4700, 4550, 4550, 5300, 6500, 5000 },
        470, 11500,
        115, 18, 420, 1500, 450,
        20500, 16500, 2200,
        19500
    },
    {
        14, 82, 390, 1150, 2300,
        22000, 24000, 8500, 6500, 13000, 16500, 11500, 9500,
        70, 34, 45, 23,
        80, 210,
        { 7000, 6700, 5600, 3900, 2600, 1500 },
        330, 6500,
        135, 17, 280, 1000, 350,
        19000, 20500, 1100,
        20500
    },
    {
        28, 150, 620, 1500, 3000,
        15000, 22000, 3200, 1800, 6500, 15500, 9000, 10000,
        58, 29, 38, 20,
        18, 130,
        { 7600, 7200, 5000, 2500, 1100, 500 },
        290, 4000,
        165, 12, 220, 700, 200,
        22000, 21000, 2300,
        22000
    },
    {
        13, 95, 520, 1700, 3200,
        27000, 25000, 14500, 15500, 15500, 24500, 19000, 12500,
        92, 31, 58, 18,
        270, 420,
        { 7600, 7000, 5700, 5200, 4800, 2600 },
        610, 15500,
        145, 18, 250, 1200, 400,
        26000, 14500, 5200,
        17500
    }
};

static char *ws_ggb89_names[WS_GGB89_PRESET_COUNT] = {
    "M67 open field",
    "40 mm HE open field",
    "40 mm HEDP hard impact",
    "indoor confined",
    "concrete impact",
    "dirt impact",
    "distant blast",
    "arcade heavy"
};

const char *ws_ggb89_preset_name(int preset)
{
    if (preset < 0 || preset >= WS_GGB89_PRESET_COUNT) {
        return "unknown";
    }
    return ws_ggb89_names[preset];
}

int ws_ggb89_get_preset(int preset, ws_ggb89_params *out_params)
{
    if (out_params == 0) {
        return 0;
    }
    if (preset < 0 || preset >= WS_GGB89_PRESET_COUNT) {
        return 0;
    }
    *out_params = ws_ggb89_presets[preset];
    return 1;
}

void ws_ggb89_reset(ws_ggb89 *synth)
{
    if (synth == 0) {
        return;
    }

    synth->age = 0;
    synth->active = 0;
    synth->env_positive = 0;
    synth->env_negative = 0;
    synth->env_body = 0;
    synth->env_debris = 0;
    synth->env_fragment = 0;
    synth->main_noise_state = 0;
    synth->low_noise_state = 0;
    synth->debris_noise_state = 0;
    synth->fragment_noise_state = 0;
    synth->fragment_smooth_state = 0;
    synth->fragment_gate = 0;
    synth->high_noise_state = 0;
    synth->saw_filter_state = 0;
    synth->high_prev = 0;
    synth->low_hold = 0;
    synth->debris_hold = 0;
    synth->low_div = 0;
    synth->debris_div = 0;
    synth->sine_phase = 0;
    synth->saw_phase = 0;
    ws_geq6_reset(&synth->eq);
}

void ws_ggb89_init(ws_ggb89 *synth, ws_gu32 sample_rate, ws_gu32 seed)
{
    if (synth == 0) {
        return;
    }
    if (sample_rate < 8000UL) {
        sample_rate = 8000UL;
    }
    if (sample_rate > 48000UL) {
        sample_rate = 48000UL;
    }
    if (seed == 0UL) {
        seed = 0x47A5C39DUL;
    }

    synth->sample_rate = sample_rate;
    synth->rng_a = seed & WS_GU32_MASK;
    synth->rng_b = (seed ^ 0xA341316CUL) & WS_GU32_MASK;
    synth->rng_c = (seed ^ 0xC8013EA4UL) & WS_GU32_MASK;
    synth->rng_d = (seed ^ 0xAD90777DUL) & WS_GU32_MASK;
    synth->rng_e = (seed ^ 0x7E95761EUL) & WS_GU32_MASK;

    ws_geq6_init(&synth->eq, sample_rate);
    ws_gdist_init(&synth->dist);
    ws_gchorus_init(&synth->chorus, sample_rate);
    ws_greverb_init(&synth->reverb, sample_rate);
    synth->params = ws_ggb89_presets[WS_GGB89_M67_OPEN];
    ws_ggb89_set_params(synth, &synth->params);
    ws_ggb89_reset(synth);
}

void ws_ggb89_set_params(ws_ggb89 *synth, const ws_ggb89_params *params)
{
    ws_gu32 base_delay;
    ws_gu32 depth;
    int i;

    if (synth == 0 || params == 0) {
        return;
    }

    synth->params = *params;
    synth->total_samples = ws_ggb89_ms_to_samples(synth->sample_rate,
                                                   params->total_ms);
    synth->pos_samples = ws_ggb89_ms_to_samples(synth->sample_rate,
                                                 params->positive_ms);
    synth->neg_samples = ws_ggb89_ms_to_samples(synth->sample_rate,
                                                 params->negative_ms);
    synth->body_samples = ws_ggb89_ms_to_samples(synth->sample_rate,
                                                  params->body_ms);
    synth->debris_samples = ws_ggb89_ms_to_samples(synth->sample_rate,
                                                    params->debris_ms);
    synth->fragment_window_samples =
        ws_ggb89_ms_to_samples(synth->sample_rate, params->fragment_window_ms);

    synth->pos_decay_q15 = ws_ggb89_env_coef(synth->pos_samples);
    synth->neg_decay_q15 = ws_ggb89_env_coef(synth->neg_samples);
    synth->body_decay_q15 = ws_ggb89_env_coef(synth->body_samples);
    synth->debris_decay_q15 = ws_ggb89_env_coef(synth->debris_samples);
    synth->fragment_decay_q15 =
        ws_ggb89_env_coef((synth->sample_rate * 7UL) / 1000UL);

    for (i = 0; i < 6; ++i) {
        ws_geq6_set_gain_q12(&synth->eq, i, params->eq_gain_q12[i]);
    }

    ws_gdist_set(&synth->dist, params->dist_drive_q8, params->dist_mix_q15);

    base_delay = (synth->sample_rate *
                  (ws_gu32)params->chorus_base_ms_x10) / 10000UL;
    depth = (synth->sample_rate *
             (ws_gu32)params->chorus_depth_ms_x10) / 10000UL;
    if (base_delay > 65535UL) {
        base_delay = 65535UL;
    }
    if (depth > 65535UL) {
        depth = 65535UL;
    }
    ws_gchorus_set(&synth->chorus,
                   (ws_gu16)base_delay,
                   (ws_gu16)depth,
                   params->chorus_rate_millihz,
                   params->chorus_wet_q15,
                   params->chorus_feedback_q15,
                   synth->sample_rate);

    ws_greverb_set(&synth->reverb,
                   params->reverb_feedback_q15,
                   params->reverb_damp_q15,
                   params->reverb_wet_q15);
}

void ws_ggb89_trigger_custom(ws_ggb89 *synth,
                             const ws_ggb89_params *params,
                             ws_gs16 intensity_q15)
{
    ws_ggb89_params working;
    ws_gu32 pitch_samples;

    if (synth == 0 || params == 0) {
        return;
    }

    working = *params;
    ws_ggb89_apply_intensity(&working, intensity_q15);
    ws_ggb89_set_params(synth, &working);

    synth->age = 0;
    synth->active = 1;
    synth->env_positive = 32767L;
    synth->env_negative = 0;
    synth->env_body = 32767L;
    synth->env_debris = 32767L;
    synth->env_fragment = 0;
    synth->main_noise_state = 0;
    synth->low_noise_state = 0;
    synth->debris_noise_state = 0;
    synth->fragment_noise_state = 0;
    synth->fragment_smooth_state = 0;
    synth->fragment_gate = 0;
    synth->high_noise_state = 0;
    synth->saw_filter_state = 0;
    synth->high_prev = 0;
    synth->low_hold = 0;
    synth->debris_hold = 0;
    synth->low_div = 0;
    synth->debris_div = 0;
    synth->sine_phase = 0;
    synth->saw_phase = 0;

    synth->sine_inc = ws_ggb89_phase_inc(synth->sample_rate,
                                         working.sine_start_hz);
    synth->saw_inc = ws_ggb89_phase_inc(synth->sample_rate,
                                        working.saw_start_hz);
    synth->sine_inc_end = ws_ggb89_phase_inc(synth->sample_rate,
                                             working.sine_end_hz);
    synth->saw_inc_end = ws_ggb89_phase_inc(synth->sample_rate,
                                            working.saw_end_hz);
    pitch_samples = synth->body_samples;
    if (pitch_samples < 1UL) {
        pitch_samples = 1UL;
    }
    if (synth->sine_inc > synth->sine_inc_end) {
        synth->sine_inc_step =
            (synth->sine_inc - synth->sine_inc_end) / pitch_samples;
    } else {
        synth->sine_inc_step = 0;
    }
    if (synth->saw_inc > synth->saw_inc_end) {
        synth->saw_inc_step =
            (synth->saw_inc - synth->saw_inc_end) / pitch_samples;
    } else {
        synth->saw_inc_step = 0;
    }
    ws_geq6_reset(&synth->eq);
}

void ws_ggb89_trigger(ws_ggb89 *synth, int preset, ws_gs16 intensity_q15)
{
    ws_ggb89_params p;

    if (!ws_ggb89_get_preset(preset, &p)) {
        ws_ggb89_get_preset(WS_GGB89_M67_OPEN, &p);
    }
    ws_ggb89_trigger_custom(synth, &p, intensity_q15);
}

static ws_gs32 ws_ggb89_layer(ws_gs16 sample, ws_gs32 env, ws_gs16 gain)
{
    ws_gs32 a;

    a = ws_gmul_q15(sample, env);
    return ws_gmul_q15(a, gain);
}

ws_gs16 ws_ggb89_process(ws_ggb89 *synth)
{
    ws_gs16 n_main_raw;
    ws_gs16 n_main;
    ws_gs16 n_low;
    ws_gs16 n_high_raw;
    ws_gs16 n_high;
    ws_gs16 n_frag_raw;
    ws_gs16 n_frag;
    ws_gs16 n_debris;
    ws_gs16 sine;
    ws_gs16 saw_raw;
    ws_gs16 saw;
    ws_gs32 saw_mod;
    ws_gs32 mix;
    ws_gs32 pressure;
    ws_gu32 rv;
    ws_gu32 index;
    ws_gs16 dry;
    ws_gs16 effected;

    if (synth == 0) {
        return 0;
    }

    mix = 0;
    if (synth->active) {
        n_main_raw = ws_ggb89_noise(&synth->rng_a);
        synth->main_noise_state +=
            ws_gmul_q15((ws_gs32)n_main_raw - synth->main_noise_state,
                        14500);
        n_main = ws_gclip16(synth->main_noise_state);

        if (synth->low_div == 0) {
            synth->low_hold = ws_ggb89_noise(&synth->rng_b);
        }
        synth->low_div = (ws_gu16)((synth->low_div + 1) & 1);
        synth->low_noise_state +=
            ws_gmul_q15((ws_gs32)synth->low_hold - synth->low_noise_state,
                        5200);
        n_low = ws_gclip16(synth->low_noise_state);

        n_high_raw = ws_ggb89_noise(&synth->rng_c);
        synth->high_noise_state +=
            ws_gmul_q15(((ws_gs32)n_high_raw - synth->high_prev) -
                        synth->high_noise_state, 8600);
        n_high = ws_gclip16(synth->high_noise_state);
        synth->high_prev = n_high_raw;

        /*
         * Speaker-safe shard layer.  v1.0.1 evaluated density every audio
         * sample, so the hard-impact presets produced hundreds of overlapping
         * micro-events per second.  The two-stage low-pass, quarter-rate
         * scheduler and retrigger guard keep discrete debris without turning
         * the layer into wide-band static on small speakers.
         */
        n_frag_raw = ws_ggb89_noise(&synth->rng_d);
        synth->fragment_noise_state +=
            ws_gmul_q15((ws_gs32)n_frag_raw -
                        synth->fragment_noise_state, 10500);
        synth->fragment_smooth_state +=
            ws_gmul_q15(synth->fragment_noise_state -
                        synth->fragment_smooth_state, 8800);
        n_frag = ws_gclip16(synth->fragment_smooth_state);
        if (synth->age < synth->fragment_window_samples &&
            (synth->age & 3UL) == 0UL &&
            synth->fragment_gate < 6500L) {
            rv = ws_ggb89_rng(&synth->rng_d) & 0xFFFFUL;
            if (rv < synth->params.fragment_density) {
                rv = ws_ggb89_rng(&synth->rng_d) & 0x1FFFUL;
                synth->fragment_gate = 18500L + (ws_gs32)rv;
            }
        }
        synth->env_fragment +=
            ws_gmul_q15(synth->fragment_gate - synth->env_fragment, 5200);

        if (synth->debris_div == 0) {
            synth->debris_hold = ws_ggb89_noise(&synth->rng_e);
        }
        synth->debris_div = (ws_gu16)((synth->debris_div + 1) & 7);
        synth->debris_noise_state +=
            ws_gmul_q15((ws_gs32)synth->debris_hold -
                        synth->debris_noise_state, 2800);
        n_debris = ws_gclip16(synth->debris_noise_state);

        index = (synth->sine_phase >> 24) & 0xFFUL;
        sine = ws_ggb89_sine_table[index];
        saw_raw = (ws_gs16)((ws_gs32)((synth->saw_phase >> 16) & 0xFFFFUL) - 32768L);
        synth->saw_filter_state +=
            ws_gmul_q15((ws_gs32)saw_raw - synth->saw_filter_state, 1200);
        saw_mod = 24576L + ((ws_gs32)n_low >> 2);
        if (saw_mod < 12000L) {
            saw_mod = 12000L;
        }
        if (saw_mod > 32767L) {
            saw_mod = 32767L;
        }
        saw = ws_gclip16(ws_gmul_q15(synth->saw_filter_state, saw_mod));

        pressure = ws_gmul_q15(synth->env_positive, 12500);
        pressure -= ws_gmul_q15(synth->env_negative,
                                synth->params.negative_q15);
        mix += pressure;
        mix += ws_ggb89_layer(n_main, synth->env_positive,
                              synth->params.main_noise_q15);
        mix += ws_ggb89_layer(n_low, synth->env_body,
                              synth->params.low_noise_q15);
        mix += ws_ggb89_layer(n_high, synth->env_positive,
                              synth->params.high_noise_q15);
        mix += ws_ggb89_layer(n_frag, synth->env_fragment,
                              synth->params.fragment_noise_q15);
        mix += ws_ggb89_layer(n_debris, synth->env_debris,
                              synth->params.debris_noise_q15);
        mix += ws_ggb89_layer(sine, synth->env_body,
                              synth->params.sine_q15);
        mix += ws_ggb89_layer(saw, synth->env_body,
                              synth->params.saw_q15);

        synth->sine_phase =
            (synth->sine_phase + synth->sine_inc) & WS_GU32_MASK;
        synth->saw_phase =
            (synth->saw_phase + synth->saw_inc) & WS_GU32_MASK;

        if (synth->sine_inc > synth->sine_inc_end + synth->sine_inc_step) {
            synth->sine_inc -= synth->sine_inc_step;
        }
        if (synth->saw_inc > synth->saw_inc_end + synth->saw_inc_step) {
            synth->saw_inc -= synth->saw_inc_step;
        }

        synth->env_positive =
            ws_gmul_q15(synth->env_positive, synth->pos_decay_q15);
        synth->env_body =
            ws_gmul_q15(synth->env_body, synth->body_decay_q15);
        if (synth->age == synth->pos_samples) {
            synth->env_negative = 32767L;
        } else if (synth->age > synth->pos_samples &&
                   synth->env_negative > 0) {
            synth->env_negative =
                ws_gmul_q15(synth->env_negative, synth->neg_decay_q15);
        }
        if (synth->age < synth->pos_samples) {
            synth->env_debris = 0;
        } else if (synth->env_debris == 0 &&
                   synth->age == synth->pos_samples) {
            synth->env_debris = 32767L;
        } else {
            synth->env_debris =
                ws_gmul_q15(synth->env_debris, synth->debris_decay_q15);
        }
        synth->fragment_gate =
            ws_gmul_q15(synth->fragment_gate, synth->fragment_decay_q15);
        if (synth->fragment_gate < 8L && synth->env_fragment < 8L) {
            synth->fragment_gate = 0;
            synth->env_fragment = 0;
        }

        synth->age += 1UL;
        if (synth->age >= synth->total_samples) {
            synth->active = 0;
        }
    }

    dry = ws_ggb89_bus_softlimit(mix);
    dry = ws_geq6_process(&synth->eq, dry);
    dry = ws_gdist_process(&synth->dist, dry);
    dry = ws_gclip16(ws_gmul_q15(dry, synth->params.output_gain_q15));
    effected = ws_gchorus_process(&synth->chorus, dry);
    effected = ws_greverb_process(&synth->reverb, effected);
    return effected;
}

void ws_ggb89_process_block(ws_ggb89 *synth, ws_gs16 *output, ws_gu32 frames)
{
    ws_gu32 i;

    if (synth == 0 || output == 0) {
        return;
    }
    for (i = 0; i < frames; ++i) {
        output[i] = ws_ggb89_process(synth);
    }
}

int ws_ggb89_is_active(const ws_ggb89 *synth)
{
    if (synth == 0) {
        return 0;
    }
    return synth->active;
}
