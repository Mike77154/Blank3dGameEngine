#include "gfire89.h"

typedef char gfire89_check_s16[(sizeof(gfire89_s16) == 2) ? 1 : -1];
typedef char gfire89_check_u32[(sizeof(gfire89_u32) == 4) ? 1 : -1];

static gfire89_s32 gfire89_clamp_q15(gfire89_s32 x)
{
    if (x < 0) {
        return 0;
    }
    if (x > 32767) {
        return 32767;
    }
    return x;
}

static gfire89_s32 gfire89_clamp_s16(gfire89_s32 x)
{
    if (x < -32768) {
        return -32768;
    }
    if (x > 32767) {
        return 32767;
    }
    return x;
}

static gfire89_s32 gfire89_abs_s32(gfire89_s32 x)
{
    if (x < 0) {
        return -x;
    }
    return x;
}

static gfire89_s32 gfire89_div_pow2(gfire89_s32 x, gfire89_s32 shift)
{
    gfire89_s32 d;
    if (shift <= 0) {
        return x;
    }
    d = (gfire89_s32)(1U << (gfire89_u32)shift);
    if (x < 0) {
        return -((-x) / d);
    }
    return x / d;
}

static gfire89_s32 gfire89_mul_q15(gfire89_s32 a, gfire89_s32 b)
{
    gfire89_s32 p;
    p = a * b;
    if (p < 0) {
        return -((-p) / 32768);
    }
    return p / 32768;
}

static gfire89_s32 gfire89_lp_shift(
    gfire89_s32 state,
    gfire89_s32 input,
    gfire89_s32 shift
)
{
    return state + gfire89_div_pow2(input - state, shift);
}

static gfire89_u32 gfire89_random_u32(gfire89 *fire)
{
    fire->rng = fire->rng * 1664525U + 1013904223U;
    return fire->rng;
}

static gfire89_s32 gfire89_white(gfire89 *fire)
{
    gfire89_u32 r;
    r = gfire89_random_u32(fire);
    return (gfire89_s32)((r >> 16) & 65535U) - 32768;
}

static gfire89_s32 gfire89_random_range(
    gfire89 *fire,
    gfire89_s32 minimum,
    gfire89_s32 maximum
)
{
    gfire89_u32 span;
    gfire89_u32 value;
    if (maximum <= minimum) {
        return minimum;
    }
    span = (gfire89_u32)(maximum - minimum + 1);
    value = gfire89_random_u32(fire) % span;
    return minimum + (gfire89_s32)value;
}

static gfire89_s32 gfire89_triangle(gfire89_u32 phase)
{
    gfire89_s32 p;
    p = (gfire89_s32)(phase & 65535U);
    if (p < 32768) {
        return p * 2 - 32768;
    }
    return 98303 - p * 2;
}

static gfire89_s32 gfire89_soft_saturate(
    gfire89_s32 input,
    gfire89_s32 drive_q15
)
{
    gfire89_s32 x;
    gfire89_s32 x2;
    gfire89_s32 x3;
    gfire89_s32 y;

    x = input + gfire89_mul_q15(input, drive_q15);
    x = gfire89_clamp_s16(x);
    x2 = gfire89_mul_q15(x, x);
    x3 = gfire89_mul_q15(x2, x);
    y = x - x3 / 3;
    y += y / 2;
    return gfire89_clamp_s16(y);
}

static void gfire89_choose_filter_shifts(gfire89 *fire)
{
    gfire89_s32 offset;

    offset = 0;
    if (fire->sample_rate <= 24000) {
        offset = -1;
    } else if (fire->sample_rate >= 80000) {
        offset = 1;
    }

    fire->shift_roar_1 = 4 + offset;
    fire->shift_roar_2 = 7 + offset;
    fire->shift_body_fast = 2 + offset;
    fire->shift_body_slow = 5 + offset;
    fire->shift_hiss = 2 + offset;
    fire->shift_jet_fast = 1 + offset;
    fire->shift_jet_slow = 4 + offset;
    fire->shift_mod_slow = 13 + offset;
    fire->shift_mod_fast = 8 + offset;
    fire->shift_eq_low = 7 + offset;
    fire->shift_eq_mid = 3 + offset;

    if (fire->shift_jet_fast < 1) {
        fire->shift_jet_fast = 1;
    }
    if (fire->shift_body_fast < 1) {
        fire->shift_body_fast = 1;
    }
    if (fire->shift_eq_mid < 1) {
        fire->shift_eq_mid = 1;
    }
}

static void gfire89_load_preset(gfire89_preset *p, gfire89_s32 preset_id)
{
    if (preset_id == GFIRE89_PRESET_TORCH) {
        p->roar_gain_q15 = 6200;
        p->body_gain_q15 = 13000;
        p->jet_gain_q15 = 14500;
        p->hiss_gain_q15 = 22000;
        p->crackle_gain_q15 = 500;
        p->burst_gain_q15 = 9000;
        p->pulse_depth_q15 = 6500;
        p->drive_q15 = 3500;
        p->brightness_q15 = 29000;
        p->crackle_rate_hz = 2;
        p->vortex_rate_hz = 5;
        p->crackle_decay_min = 6;
        p->crackle_decay_max = 8;
    } else if (preset_id == GFIRE89_PRESET_CAMPFIRE) {
        p->roar_gain_q15 = 15000;
        p->body_gain_q15 = 11000;
        p->jet_gain_q15 = 1400;
        p->hiss_gain_q15 = 3000;
        p->crackle_gain_q15 = 27500;
        p->burst_gain_q15 = 4500;
        p->pulse_depth_q15 = 5000;
        p->drive_q15 = 6000;
        p->brightness_q15 = 18000;
        p->crackle_rate_hz = 24;
        p->vortex_rate_hz = 2;
        p->crackle_decay_min = 7;
        p->crackle_decay_max = 10;
    } else if (preset_id == GFIRE89_PRESET_BONFIRE) {
        p->roar_gain_q15 = 27000;
        p->body_gain_q15 = 19500;
        p->jet_gain_q15 = 4200;
        p->hiss_gain_q15 = 6200;
        p->crackle_gain_q15 = 19000;
        p->burst_gain_q15 = 7500;
        p->pulse_depth_q15 = 8500;
        p->drive_q15 = 9500;
        p->brightness_q15 = 20000;
        p->crackle_rate_hz = 18;
        p->vortex_rate_hz = 4;
        p->crackle_decay_min = 7;
        p->crackle_decay_max = 10;
    } else if (preset_id == GFIRE89_PRESET_DEBRIS) {
        p->roar_gain_q15 = 10500;
        p->body_gain_q15 = 9000;
        p->jet_gain_q15 = 800;
        p->hiss_gain_q15 = 4200;
        p->crackle_gain_q15 = 30000;
        p->burst_gain_q15 = 4000;
        p->pulse_depth_q15 = 3500;
        p->drive_q15 = 5000;
        p->brightness_q15 = 23500;
        p->crackle_rate_hz = 38;
        p->vortex_rate_hz = 2;
        p->crackle_decay_min = 6;
        p->crackle_decay_max = 9;
    } else {
        /* v1.2 weapon-tuned flamethrower:
           pressure/body first, hiss only as air edge, audible crackle. */
        p->roar_gain_q15 = 30000;
        p->body_gain_q15 = 32000;
        p->jet_gain_q15 = 23500;
        p->hiss_gain_q15 = 7600;
        p->crackle_gain_q15 = 9800;
        p->burst_gain_q15 = 22500;
        p->pulse_depth_q15 = 24500;
        p->drive_q15 = 20500;
        p->brightness_q15 = 16500;
        p->crackle_rate_hz = 11;
        p->vortex_rate_hz = 5;
        p->crackle_decay_min = 5;
        p->crackle_decay_max = 8;
    }
}

static gfire89_u32 gfire89_hz_to_increment(
    gfire89_s32 hz,
    gfire89_s32 sample_rate
)
{
    if (hz < 1) {
        hz = 1;
    }
    return (gfire89_u32)((hz * 65536) / sample_rate);
}

static void gfire89_retune_chug(gfire89 *fire)
{
    gfire89_s32 hz;
    hz = 22;
    hz += (fire->airflow_q15 * 28) / 32767;
    hz -= (fire->size_q15 * 8) / 32767;
    hz += gfire89_random_range(fire, -5, 5);
    if (hz < 12) {
        hz = 12;
    }
    if (hz > 62) {
        hz = 62;
    }
    fire->chug_increment = gfire89_hz_to_increment(hz, fire->sample_rate);
}

static void gfire89_retune_flutter(gfire89 *fire)
{
    gfire89_s32 hz;
    hz = 80;
    hz += (fire->airflow_q15 * 105) / 32767;
    hz += gfire89_random_range(fire, -19, 19);
    if (hz < 55) {
        hz = 55;
    }
    if (hz > 220) {
        hz = 220;
    }
    fire->flutter_increment = gfire89_hz_to_increment(hz, fire->sample_rate);
}

void gfire89_init(gfire89 *fire, gfire89_s32 sample_rate, gfire89_u32 seed)
{
    gfire89_s32 i;
    unsigned char *bytes;

    if (fire == 0) {
        return;
    }

    bytes = (unsigned char *)fire;
    for (i = 0; i < (gfire89_s32)sizeof(*fire); ++i) {
        bytes[i] = 0;
    }

    if (sample_rate < 8000) {
        sample_rate = 8000;
    }

    fire->sample_rate = sample_rate;
    fire->rng = seed;
    if (fire->rng == 0U) {
        fire->rng = 0x6D2B79F5U;
    }

    fire->intensity_q15 = 32767;
    fire->airflow_q15 = 32767;
    fire->crackle_q15 = 32767;
    fire->size_q15 = 24576;
    fire->output_gain_q15 = 24576;
    fire->crack_countdown = 1;
    fire->vortex_countdown = 1;

    gfire89_choose_filter_shifts(fire);
    gfire89_set_preset(fire, GFIRE89_PRESET_FLAMETHROWER);
    gfire89_retune_chug(fire);
    gfire89_retune_flutter(fire);
}

void gfire89_set_preset(gfire89 *fire, gfire89_s32 preset_id)
{
    if (fire == 0) {
        return;
    }

    if (preset_id < 0 || preset_id >= GFIRE89_PRESET_COUNT) {
        preset_id = GFIRE89_PRESET_FLAMETHROWER;
    }

    gfire89_load_preset(&fire->preset, preset_id);
    fire->pressure_q15 = fire->preset.pulse_depth_q15;
    fire->drive_q15 = fire->preset.drive_q15;
    fire->brightness_q15 = fire->preset.brightness_q15;
    fire->crack_countdown = 1;
    fire->vortex_countdown = 1;
}

void gfire89_set_controls(
    gfire89 *fire,
    gfire89_s32 intensity_q15,
    gfire89_s32 airflow_q15,
    gfire89_s32 crackle_q15,
    gfire89_s32 size_q15
)
{
    if (fire == 0) {
        return;
    }

    fire->intensity_q15 = gfire89_clamp_q15(intensity_q15);
    fire->airflow_q15 = gfire89_clamp_q15(airflow_q15);
    fire->crackle_q15 = gfire89_clamp_q15(crackle_q15);
    fire->size_q15 = gfire89_clamp_q15(size_q15);
}

void gfire89_set_detail(
    gfire89 *fire,
    gfire89_s32 pressure_q15,
    gfire89_s32 drive_q15,
    gfire89_s32 brightness_q15
)
{
    if (fire == 0) {
        return;
    }

    fire->pressure_q15 = gfire89_clamp_q15(pressure_q15);
    fire->drive_q15 = gfire89_clamp_q15(drive_q15);
    fire->brightness_q15 = gfire89_clamp_q15(brightness_q15);
}

void gfire89_set_gate(gfire89 *fire, gfire89_s32 gate_on)
{
    if (fire == 0) {
        return;
    }
    fire->gate_q15 = gate_on ? 32767 : 0;
}

void gfire89_set_output_gain(gfire89 *fire, gfire89_s32 gain_q15)
{
    if (fire == 0) {
        return;
    }
    fire->output_gain_q15 = gfire89_clamp_q15(gain_q15);
}

static void gfire89_schedule_vortex(gfire89 *fire)
{
    gfire89_s32 rate;
    gfire89_s32 interval;

    rate = (gfire89_s32)fire->preset.vortex_rate_hz;
    rate += (fire->airflow_q15 * 5) / 32767;
    if (rate < 1) {
        rate = 1;
    }

    interval = fire->sample_rate / rate;
    fire->vortex_countdown = interval +
        gfire89_random_range(fire, -(interval / 2), interval / 2);

    if (fire->vortex_countdown < 16) {
        fire->vortex_countdown = 16;
    }

    fire->vortex_env_q15 = gfire89_random_range(fire, 6000, 19000);
    fire->vortex_decay_shift = gfire89_random_range(fire, 9, 12);
}

static gfire89_s32 gfire89_process_vortex(gfire89 *fire)
{
    gfire89_s32 value;
    gfire89_s32 decay;

    fire->vortex_countdown -= 1;
    if (fire->vortex_countdown <= 0) {
        gfire89_schedule_vortex(fire);
    }

    value = fire->vortex_env_q15;
    decay = gfire89_div_pow2(value, fire->vortex_decay_shift);
    if (decay < 1 && value > 0) {
        decay = 1;
    }
    fire->vortex_env_q15 -= decay;
    if (fire->vortex_env_q15 < 0) {
        fire->vortex_env_q15 = 0;
    }

    return value;
}

static void gfire89_schedule_crackle(gfire89 *fire)
{
    gfire89_s32 rate;
    gfire89_s32 base_interval;
    gfire89_s32 jitter;
    gfire89_s32 sign;

    rate = (gfire89_s32)fire->preset.crackle_rate_hz;
    rate = gfire89_mul_q15(rate * 256, fire->crackle_q15) / 256;
    if (rate < 1) {
        rate = 1;
    }

    base_interval = fire->sample_rate / rate;
    if (base_interval < 16) {
        base_interval = 16;
    }

    jitter = gfire89_random_range(
        fire,
        -(base_interval / 2),
        base_interval
    );

    fire->crack_countdown = base_interval + jitter;
    if (fire->crack_countdown < 8) {
        fire->crack_countdown = 8;
    }

    fire->crack_env_q15 = gfire89_random_range(fire, 11000, 32767);
    sign = (gfire89_random_u32(fire) & 1U) ? 1 : -1;
    fire->crack_impulse = sign *
        gfire89_random_range(fire, 9000, 25000);

    fire->crack_decay_shift = gfire89_random_range(
        fire,
        (gfire89_s32)fire->preset.crackle_decay_min,
        (gfire89_s32)fire->preset.crackle_decay_max
    );

    if ((gfire89_random_u32(fire) & 7U) == 0U) {
        fire->cluster_left = gfire89_random_range(fire, 1, 3);
        fire->cluster_gap = gfire89_random_range(
            fire,
            fire->sample_rate / 500,
            fire->sample_rate / 120
        );
    }
}

static gfire89_s32 gfire89_process_crackle(
    gfire89 *fire,
    gfire89_s32 noise
)
{
    gfire89_s32 colored;
    gfire89_s32 out;
    gfire89_s32 decay;

    fire->crack_countdown -= 1;

    if (fire->cluster_left > 0) {
        fire->cluster_gap -= 1;
        if (fire->cluster_gap <= 0) {
            fire->cluster_left -= 1;
            fire->crack_countdown = 0;
            fire->cluster_gap = gfire89_random_range(
                fire,
                fire->sample_rate / 700,
                fire->sample_rate / 140
            );
        }
    }

    if (fire->crack_countdown <= 0) {
        gfire89_schedule_crackle(fire);
    }

    colored = noise - fire->lp_hiss;
    out = gfire89_mul_q15(colored, fire->crack_env_q15);
    out += fire->crack_impulse;

    decay = gfire89_div_pow2(
        fire->crack_env_q15,
        fire->crack_decay_shift
    );

    if (decay < 1 && fire->crack_env_q15 > 0) {
        decay = 1;
    }

    fire->crack_env_q15 -= decay;
    if (fire->crack_env_q15 < 0) {
        fire->crack_env_q15 = 0;
    }

    fire->crack_impulse -=
        gfire89_div_pow2(fire->crack_impulse, 4);

    if (gfire89_abs_s32(fire->crack_impulse) < 2) {
        fire->crack_impulse = 0;
    }

    return gfire89_clamp_s16(out);
}

static gfire89_s32 gfire89_post_eq(
    gfire89 *fire,
    gfire89_s32 input
)
{
    gfire89_s32 low;
    gfire89_s32 mid;
    gfire89_s32 high;
    gfire89_s32 low_gain;
    gfire89_s32 mid_gain;
    gfire89_s32 high_gain;
    gfire89_s32 output;

    fire->eq_low_state = gfire89_lp_shift(
        fire->eq_low_state,
        input,
        fire->shift_eq_low
    );

    fire->eq_mid_state = gfire89_lp_shift(
        fire->eq_mid_state,
        input,
        fire->shift_eq_mid
    );

    low = fire->eq_low_state;
    mid = fire->eq_mid_state - fire->eq_low_state;
    high = input - fire->eq_mid_state;

    low_gain = 23500 + fire->size_q15 / 4;
    if (low_gain > 32767) {
        low_gain = 32767;
    }

    mid_gain = 26000 + fire->intensity_q15 / 8;
    if (mid_gain > 32767) {
        mid_gain = 32767;
    }

    high_gain = 7000 + (fire->brightness_q15 * 3) / 4;
    if (high_gain > 32767) {
        high_gain = 32767;
    }

    output = gfire89_mul_q15(low, low_gain);
    output += gfire89_mul_q15(mid, mid_gain);
    output += gfire89_mul_q15(high, high_gain);

    return gfire89_clamp_s16(output);
}

gfire89_s16 gfire89_process_sample(gfire89 *fire)
{
    gfire89_s32 n1;
    gfire89_s32 n2;
    gfire89_s32 roar;
    gfire89_s32 body;
    gfire89_s32 jet;
    gfire89_s32 hiss;
    gfire89_s32 crack;
    gfire89_s32 burst;
    gfire89_s32 chug;
    gfire89_s32 flutter;
    gfire89_s32 chug_mod_q15;
    gfire89_s32 flutter_mod_q15;
    gfire89_s32 slow_mod_q15;
    gfire89_s32 fast_mod_q15;
    gfire89_s32 mod_gain_q15;
    gfire89_s32 vortex_q15;
    gfire89_s32 vortex_mod_q15;
    gfire89_s32 size_roar_q15;
    gfire89_s32 airflow_body_q15;
    gfire89_s32 roar_gain_q15;
    gfire89_s32 body_gain_q15;
    gfire89_s32 jet_gain_q15;
    gfire89_s32 hiss_gain_q15;
    gfire89_s32 crack_gain_q15;
    gfire89_s32 turbulent_bus;
    gfire89_s32 mix;
    gfire89_s32 target_env;
    gfire89_s32 delta;
    gfire89_s32 sample;
    gfire89_u32 old_phase;

    if (fire == 0) {
        return 0;
    }

    n1 = gfire89_white(fire);
    n2 = gfire89_white(fire);

    fire->lp_roar_1 = gfire89_lp_shift(
        fire->lp_roar_1,
        n1,
        fire->shift_roar_1
    );

    fire->lp_roar_2 = gfire89_lp_shift(
        fire->lp_roar_2,
        n1,
        fire->shift_roar_2
    );

    roar = fire->lp_roar_1 +
        gfire89_div_pow2(fire->lp_roar_2, 1);
    roar = gfire89_clamp_s16(roar);

    fire->lp_body_fast = gfire89_lp_shift(
        fire->lp_body_fast,
        n1,
        fire->shift_body_fast
    );

    fire->lp_body_slow = gfire89_lp_shift(
        fire->lp_body_slow,
        n1,
        fire->shift_body_slow
    );

    body = fire->lp_body_fast - fire->lp_body_slow;
    body = gfire89_clamp_s16(body);

    fire->lp_hiss = gfire89_lp_shift(
        fire->lp_hiss,
        n2,
        fire->shift_hiss
    );

    hiss = n2 - fire->lp_hiss;
    hiss = gfire89_clamp_s16(hiss);

    fire->lp_jet_fast = gfire89_lp_shift(
        fire->lp_jet_fast,
        n2,
        fire->shift_jet_fast
    );

    fire->lp_jet_slow = gfire89_lp_shift(
        fire->lp_jet_slow,
        n2,
        fire->shift_jet_slow
    );

    jet = fire->lp_jet_fast - fire->lp_jet_slow;
    jet = gfire89_clamp_s16(jet);

    fire->mod_slow = gfire89_lp_shift(
        fire->mod_slow,
        n1,
        fire->shift_mod_slow
    );

    fire->mod_fast = gfire89_lp_shift(
        fire->mod_fast,
        n2,
        fire->shift_mod_fast
    );

    old_phase = fire->chug_phase;
    fire->chug_phase =
        (fire->chug_phase + fire->chug_increment) & 65535U;
    if (fire->chug_phase < old_phase) {
        gfire89_retune_chug(fire);
    }

    old_phase = fire->flutter_phase;
    fire->flutter_phase =
        (fire->flutter_phase + fire->flutter_increment) & 65535U;
    if (fire->flutter_phase < old_phase) {
        gfire89_retune_flutter(fire);
    }

    chug = gfire89_triangle(fire->chug_phase);
    flutter = gfire89_triangle(fire->flutter_phase);

    chug_mod_q15 = 32767 - fire->pressure_q15 / 5;
    chug_mod_q15 += gfire89_mul_q15(
        chug,
        fire->pressure_q15 / 5
    );
    chug_mod_q15 = gfire89_clamp_q15(chug_mod_q15);

    flutter_mod_q15 = 28672 + gfire89_mul_q15(
        flutter,
        fire->pressure_q15 / 8
    );
    flutter_mod_q15 = gfire89_clamp_q15(flutter_mod_q15);

    slow_mod_q15 = gfire89_clamp_q15(
        24576 + fire->mod_slow / 4
    );

    fast_mod_q15 = gfire89_clamp_q15(
        28672 + fire->mod_fast / 8
    );

    mod_gain_q15 = gfire89_mul_q15(
        slow_mod_q15,
        fast_mod_q15
    );

    vortex_q15 = gfire89_process_vortex(fire);
    vortex_mod_q15 = 24576 + vortex_q15 / 4;
    if (vortex_mod_q15 > 32767) {
        vortex_mod_q15 = 32767;
    }

    size_roar_q15 = 16384 + fire->size_q15 / 2;
    airflow_body_q15 = 16384 + fire->airflow_q15 / 2;

    roar_gain_q15 = gfire89_mul_q15(
        (gfire89_s32)fire->preset.roar_gain_q15,
        size_roar_q15
    );

    body_gain_q15 = gfire89_mul_q15(
        (gfire89_s32)fire->preset.body_gain_q15,
        airflow_body_q15
    );

    jet_gain_q15 = gfire89_mul_q15(
        (gfire89_s32)fire->preset.jet_gain_q15,
        fire->airflow_q15
    );

    hiss_gain_q15 = gfire89_mul_q15(
        (gfire89_s32)fire->preset.hiss_gain_q15,
        8192 + (fire->airflow_q15 * 3) / 4
    );

    crack_gain_q15 = gfire89_mul_q15(
        (gfire89_s32)fire->preset.crackle_gain_q15,
        fire->crackle_q15
    );

    roar = gfire89_mul_q15(roar, roar_gain_q15);
    roar = gfire89_mul_q15(roar, mod_gain_q15);
    roar = gfire89_mul_q15(roar, chug_mod_q15);

    body = gfire89_mul_q15(body, body_gain_q15);
    body = gfire89_mul_q15(body, vortex_mod_q15);

    jet = gfire89_mul_q15(jet, jet_gain_q15);
    jet = gfire89_mul_q15(jet, fast_mod_q15);
    jet = gfire89_mul_q15(jet, flutter_mod_q15);

    hiss = gfire89_mul_q15(hiss, hiss_gain_q15);
    hiss = gfire89_mul_q15(hiss, vortex_mod_q15);

    crack = gfire89_process_crackle(fire, n2);
    crack = gfire89_mul_q15(crack, crack_gain_q15);

    if (fire->gate_q15 > 0 && fire->previous_gate == 0) {
        fire->burst_env_q15 = 32767;
    }
    fire->previous_gate = fire->gate_q15;

    burst = gfire89_mul_q15(
        fire->lp_roar_1 + body,
        fire->burst_env_q15
    );

    burst = gfire89_mul_q15(
        burst,
        (gfire89_s32)fire->preset.burst_gain_q15
    );

    fire->burst_env_q15 -= gfire89_div_pow2(
        fire->burst_env_q15,
        11
    );

    if (fire->burst_env_q15 < 8) {
        fire->burst_env_q15 = 0;
    }

    turbulent_bus = (roar + body + jet) / 2;
    turbulent_bus = gfire89_soft_saturate(
        turbulent_bus,
        fire->drive_q15
    );

    /* Keep broadband hiss subordinate to the pressure/turbulence body. */
    mix = turbulent_bus + hiss / 3 + crack + burst / 2;
    mix = gfire89_post_eq(fire, gfire89_clamp_s16(mix));

    target_env = gfire89_mul_q15(
        fire->gate_q15,
        fire->intensity_q15
    );

    delta = target_env - fire->env_q15;
    if (delta > 0) {
        fire->env_q15 += gfire89_div_pow2(delta, 7);
    } else {
        fire->env_q15 += gfire89_div_pow2(delta, 10);
    }

    mix = gfire89_mul_q15(mix, fire->env_q15);
    mix = gfire89_mul_q15(mix, fire->output_gain_q15);

    sample = gfire89_clamp_s16(mix);
    return (gfire89_s16)sample;
}

void gfire89_render_s16(
    gfire89 *fire,
    gfire89_s16 *dst,
    gfire89_s32 frames
)
{
    gfire89_s32 i;

    if (fire == 0 || dst == 0 || frames <= 0) {
        return;
    }

    for (i = 0; i < frames; ++i) {
        dst[i] = gfire89_process_sample(fire);
    }
}
