#include "gbulletair89.h"

#define GBA89_CHORUS_MASK 255
#define GBA89_REVERB_MASK 2047

static const gba89_config gba89_presets[GBA89_PRESET_COUNT] = {
    {
        32, 36, 20, 24, 16500, 28000,
        5000, 2700, 15000, 10800, 22200,
        330, 7000,
        3400, 4300, 5000, 4700, 3000, 1800,
        1800, 24, 8, 1,
        4300, 6800, 110,
        0, 0, 0,
        -28000, 28000
    },
    {
        26, 28, 14, 20, 15500, 30000,
        8600, 3400, 17500, 12500, 23600,
        390, 8500,
        3200, 4200, 5100, 5000, 3300, 2000,
        2100, 20, 7, 2,
        4800, 7200, 110,
        7000, 18, 5,
        -30000, 30000
    },
    {
        22, 25, 12, 18, 14000, 30500,
        10400, 4800, 18800, 13800, 24800,
        450, 9000,
        3400, 4300, 5200, 5200, 3500, 2200,
        1600, 18, 6, 2,
        5200, 7800, 120,
        26000, 14, 8,
        -31000, 31000
    },
    {
        27, 38, 18, 32, 15000, 31500,
        8600, 3300, 18200, 12800, 24200,
        500, 10000,
        3800, 4700, 5500, 5200, 3300, 1900,
        1900, 22, 9, 2,
        6800, 9200, 160,
        30000, 17, 9,
        -32000, 32000
    },
    {
        34, 50, 24, 40, 16500, 31500,
        6100, 1600, 16200, 10200, 21000,
        590, 11500,
        4700, 5400, 5700, 4800, 2800, 1500,
        2200, 28, 12, 1,
        7600, 10500, 210,
        32000, 23, 10,
        -30000, 30000
    },
    {
        20, 24, 12, 36, 13000, 24000,
        8600, 5000, 16000, 11800, 23000,
        370, 6500,
        3000, 3900, 4700, 4800, 3000, 1800,
        1100, 16, 4, 1,
        9000, 11800, 250,
        18000, 12, 7,
        -22000, 22000
    },
    {
        30, 42, 24, 50, 16000, 30000,
        6100, 2700, 16500, 11200, 21600,
        480, 9200,
        4000, 5000, 5400, 4600, 2600, 1400,
        1400, 19, 5, 1,
        12500, 14000, 300,
        27000, 20, 9,
        -29000, 29000
    },
    {
        36, 46, 26, 36, 18000, 31500,
        10400, 3400, 19000, 13800, 25200,
        700, 12500,
        3500, 4600, 5800, 5900, 3800, 2200,
        3600, 30, 14, 3,
        8200, 10500, 210,
        25000, 28, 8,
        -32000, 32000
    }
};

static gba89_s16 gba89_clamp16(gba89_s32 x)
{
    if (x > 32767L) {
        return (gba89_s16)32767;
    }
    if (x < -32768L) {
        return (gba89_s16)-32768;
    }
    return (gba89_s16)x;
}

static gba89_s32 gba89_soft_clip(gba89_s32 x)
{
    gba89_s32 a;
    gba89_s32 y;
    int neg;

    neg = 0;
    if (x < 0L) {
        neg = 1;
        a = -x;
    } else {
        a = x;
    }

    if (a < 16384L) {
        y = a;
    } else if (a < 32768L) {
        y = 16384L + ((a - 16384L) >> 1);
    } else if (a < 65536L) {
        y = 24576L + ((a - 32768L) >> 3);
    } else {
        y = 28672L;
    }

    if (neg) {
        return -y;
    }
    return y;
}

static gba89_u32 gba89_random(gba89_state *state)
{
    gba89_u32 x;

    x = state->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (x == 0UL) {
        x = 0x6D2B79F5UL;
    }
    state->rng = x;
    return x;
}

static gba89_s32 gba89_interp(gba89_s32 a, gba89_s32 b, gba89_u32 pos, gba89_u32 length)
{
    gba89_s32 delta;

    if (length == 0UL || pos >= length) {
        return b;
    }
    delta = b - a;
    return a + (delta * (gba89_s32)pos) / (gba89_s32)length;
}

static gba89_s32 gba89_envelope(const gba89_state *state)
{
    gba89_u32 p;
    gba89_u32 a_end;
    gba89_u32 d_end;
    gba89_u32 s_end;
    gba89_s32 sustain;

    p = state->age;
    sustain = state->cfg.sustain_q15;
    a_end = state->attack_samples;
    d_end = a_end + state->decay_samples;
    s_end = d_end + state->sustain_samples;

    if (p < a_end) {
        if (a_end == 0UL) {
            return 32767L;
        }
        return (32767L * (gba89_s32)p) / (gba89_s32)a_end;
    }
    if (p < d_end) {
        return gba89_interp(32767L, sustain, p - a_end, state->decay_samples);
    }
    if (p < s_end) {
        return sustain;
    }
    if (p < state->source_samples) {
        return gba89_interp(sustain, 0L, p - s_end, state->release_samples);
    }
    return 0L;
}

static gba89_s32 gba89_crack(const gba89_state *state)
{
    gba89_u32 i;
    gba89_u32 n;
    gba89_s32 level;

    if (state->cfg.crack_samples < 2U) {
        return 0L;
    }
    if (state->age < state->crack_at_samples) {
        return 0L;
    }
    i = state->age - state->crack_at_samples;
    n = (gba89_u32)state->cfg.crack_samples;
    if (i >= n) {
        return 0L;
    }
    level = state->cfg.crack_level_q15;
    return level - ((2L * level * (gba89_s32)i) / (gba89_s32)(n - 1UL));
}


static gba89_s32 gba89_equalize(gba89_state *state, gba89_s32 x)
{
    gba89_s32 b0;
    gba89_s32 b1;
    gba89_s32 b2;
    gba89_s32 b3;
    gba89_s32 b4;
    gba89_s32 b5;
    gba89_s32 y;

    state->eq_lp_1 += (850L * (x - state->eq_lp_1)) >> 15;
    state->eq_lp_2 += (2075L * (x - state->eq_lp_2)) >> 15;
    state->eq_lp_3 += (4760L * (x - state->eq_lp_3)) >> 15;
    state->eq_lp_4 += (10650L * (x - state->eq_lp_4)) >> 15;
    state->eq_lp_5 += (19650L * (x - state->eq_lp_5)) >> 15;

    b0 = state->eq_lp_1;
    b1 = state->eq_lp_2 - state->eq_lp_1;
    b2 = state->eq_lp_3 - state->eq_lp_2;
    b3 = state->eq_lp_4 - state->eq_lp_3;
    b4 = state->eq_lp_5 - state->eq_lp_4;
    b5 = x - state->eq_lp_5;

    y = (b0 * (gba89_s32)state->cfg.eq_sub_q12) >> 12;
    y += (b1 * (gba89_s32)state->cfg.eq_low_q12) >> 12;
    y += (b2 * (gba89_s32)state->cfg.eq_mid_q12) >> 12;
    y += (b3 * (gba89_s32)state->cfg.eq_presence_q12) >> 12;
    y += (b4 * (gba89_s32)state->cfg.eq_air_q12) >> 12;
    y += (b5 * (gba89_s32)state->cfg.eq_ultra_q12) >> 12;
    return y;
}

void gba89_init(gba89_state *state, gba89_u32 seed)
{
    int i;

    if (state == 0) {
        return;
    }
    state->rng = seed;
    if (state->rng == 0UL) {
        state->rng = 0xA341316CUL;
    }
    state->age = 0UL;
    state->source_samples = 0UL;
    state->total_samples = 0UL;
    state->attack_samples = 0UL;
    state->decay_samples = 0UL;
    state->sustain_samples = 0UL;
    state->release_samples = 0UL;
    state->crack_at_samples = 0UL;
    state->active = 0;
    state->hp_memory = 0L;
    state->lp_memory_1 = 0L;
    state->lp_memory_2 = 0L;
    state->body_lp_memory_1 = 0L;
    state->body_lp_memory_2 = 0L;
    state->flutter_memory = 0L;
    state->eq_lp_1 = 0L;
    state->eq_lp_2 = 0L;
    state->eq_lp_3 = 0L;
    state->eq_lp_4 = 0L;
    state->eq_lp_5 = 0L;
    state->chorus_index = 0U;
    state->chorus_phase = 0U;
    state->reverb_index = 0U;
    for (i = 0; i < GBA89_CHORUS_SIZE; ++i) {
        state->chorus_buffer[i] = 0;
    }
    for (i = 0; i < GBA89_REVERB_SIZE; ++i) {
        state->reverb_buffer[i] = 0;
    }
}

const gba89_config *gba89_get_preset(int preset_index)
{
    if (preset_index < 0 || preset_index >= GBA89_PRESET_COUNT) {
        preset_index = GBA89_PRESET_SUPERSONIC_SNAP;
    }
    return &gba89_presets[preset_index];
}

void gba89_trigger(gba89_state *state, const gba89_config *config)
{
    if (state == 0 || config == 0) {
        return;
    }

    state->cfg = *config;
    state->age = 0UL;
    state->attack_samples = (gba89_u32)config->attack_ms * 48UL;
    state->decay_samples = (gba89_u32)config->decay_ms * 48UL;
    state->sustain_samples = (gba89_u32)config->sustain_ms * 48UL;
    state->release_samples = (gba89_u32)config->release_ms * 48UL;
    state->source_samples = state->attack_samples + state->decay_samples +
                            state->sustain_samples + state->release_samples;
    if (state->source_samples == 0UL) {
        state->source_samples = 1UL;
    }
    state->total_samples = state->source_samples +
                           ((gba89_u32)config->reverb_tail_ms * 48UL);
    state->crack_at_samples = (gba89_u32)config->crack_delay_ms * 48UL;
    state->active = 1;
    state->hp_memory = 0L;
    state->lp_memory_1 = 0L;
    state->lp_memory_2 = 0L;
    state->body_lp_memory_1 = 0L;
    state->body_lp_memory_2 = 0L;
    state->flutter_memory = 0L;
    state->eq_lp_1 = 0L;
    state->eq_lp_2 = 0L;
    state->eq_lp_3 = 0L;
    state->eq_lp_4 = 0L;
    state->eq_lp_5 = 0L;
}

void gba89_trigger_preset(gba89_state *state, int preset_index)
{
    gba89_trigger(state, gba89_get_preset(preset_index));
}

void gba89_stop(gba89_state *state)
{
    if (state != 0) {
        state->active = 0;
    }
}

int gba89_is_active(const gba89_state *state)
{
    if (state == 0) {
        return 0;
    }
    return state->active;
}

static void gba89_render_one(gba89_state *state, gba89_s16 *left_out, gba89_s16 *right_out)
{
    gba89_s32 white;
    gba89_s32 mod_noise;
    gba89_s32 flutter;
    gba89_s32 hp_coeff;
    gba89_s32 lp_coeff;
    gba89_s32 high;
    gba89_s32 band;
    gba89_s32 env;
    gba89_s32 dry;
    gba89_s32 crack;
    gba89_s32 equalized;
    gba89_s32 driven;
    gba89_u16 tri;
    gba89_u16 delay;
    gba89_u16 read_index;
    gba89_s32 delayed;
    gba89_s32 chorused;
    gba89_s32 tap_l;
    gba89_s32 tap_r;
    gba89_s32 feedback_tap;
    gba89_s32 reverb_write;
    gba89_s32 pan;
    gba89_s32 gain_l;
    gba89_s32 gain_r;
    gba89_s32 left;
    gba89_s32 right;
    gba89_s32 wet_l;
    gba89_s32 wet_r;

    if (state == 0 || !state->active) {
        *left_out = 0;
        *right_out = 0;
        return;
    }

    white = (gba89_s32)((gba89_random(state) >> 16) & 65535UL) - 32768L;
    mod_noise = (gba89_s32)((gba89_random(state) >> 17) & 32767UL) - 16384L;
    state->flutter_memory += (mod_noise - state->flutter_memory) >> 6;
    flutter = 32767L + ((state->flutter_memory * (gba89_s32)state->cfg.flutter_q15) >> 15);
    if (flutter < 8192L) {
        flutter = 8192L;
    }
    if (flutter > 49152L) {
        flutter = 49152L;
    }

    hp_coeff = gba89_interp(state->cfg.hp_start_q15, state->cfg.hp_end_q15,
                            state->age, state->source_samples);
    lp_coeff = gba89_interp(state->cfg.lp_start_q15, state->cfg.lp_end_q15,
                            state->age, state->source_samples);

    state->hp_memory += (hp_coeff * (white - state->hp_memory)) >> 15;
    high = white - state->hp_memory;
    state->lp_memory_1 += (lp_coeff * (high - state->lp_memory_1)) >> 15;
    state->lp_memory_2 += (lp_coeff * (state->lp_memory_1 - state->lp_memory_2)) >> 15;
    band = state->lp_memory_2;

    env = gba89_envelope(state);
    dry = (band * env) >> 15;
    dry = (dry * flutter) >> 15;
    dry = (dry * (gba89_s32)state->cfg.level_q15) >> 15;

    state->body_lp_memory_1 +=
        ((gba89_s32)state->cfg.body_lp_q15 *
         (dry - state->body_lp_memory_1)) >> 15;
    state->body_lp_memory_2 +=
        ((gba89_s32)state->cfg.body_lp_q15 *
         (state->body_lp_memory_1 - state->body_lp_memory_2)) >> 15;
    dry = state->body_lp_memory_2;

    crack = gba89_crack(state);
    dry += crack;

    equalized = gba89_equalize(state, dry);
    driven = (equalized * (gba89_s32)state->cfg.drive_q8_8) >> 8;
    dry = gba89_soft_clip(driven);

    if (state->chorus_phase < 32768U) {
        tri = state->chorus_phase;
    } else {
        tri = (gba89_u16)(65535U - state->chorus_phase);
    }
    delay = (gba89_u16)(state->cfg.chorus_base_samples +
            (((gba89_u32)state->cfg.chorus_depth_samples * (gba89_u32)tri) >> 15));
    if (delay > 255U) {
        delay = 255U;
    }
    read_index = (gba89_u16)((state->chorus_index - delay) & GBA89_CHORUS_MASK);
    delayed = state->chorus_buffer[read_index];
    chorused = dry + (((delayed - dry) * (gba89_s32)state->cfg.chorus_mix_q15) >> 15);
    state->chorus_buffer[state->chorus_index] = gba89_clamp16(dry);
    state->chorus_index = (gba89_u16)((state->chorus_index + 1U) & GBA89_CHORUS_MASK);
    state->chorus_phase = (gba89_u16)(state->chorus_phase + state->cfg.chorus_rate_step);

    tap_l = (gba89_s32)state->reverb_buffer[(state->reverb_index - 421U) & GBA89_REVERB_MASK];
    tap_l += ((gba89_s32)state->reverb_buffer[(state->reverb_index - 1117U) & GBA89_REVERB_MASK]) >> 1;
    tap_r = (gba89_s32)state->reverb_buffer[(state->reverb_index - 613U) & GBA89_REVERB_MASK];
    tap_r += ((gba89_s32)state->reverb_buffer[(state->reverb_index - 1439U) & GBA89_REVERB_MASK]) >> 1;
    feedback_tap = state->reverb_buffer[(state->reverb_index - 1741U) & GBA89_REVERB_MASK];
    reverb_write = chorused + ((feedback_tap * (gba89_s32)state->cfg.reverb_feedback_q15) >> 15);
    state->reverb_buffer[state->reverb_index] = gba89_clamp16(reverb_write);
    state->reverb_index = (gba89_u16)((state->reverb_index + 1U) & GBA89_REVERB_MASK);

    pan = gba89_interp(state->cfg.pan_start_q15, state->cfg.pan_end_q15,
                       state->age, state->source_samples);
    gain_l = (32767L - pan) >> 1;
    gain_r = (32767L + pan) >> 1;
    left = (chorused * gain_l) >> 15;
    right = (chorused * gain_r) >> 15;

    wet_l = (tap_l * (gba89_s32)state->cfg.reverb_mix_q15) >> 15;
    wet_r = (tap_r * (gba89_s32)state->cfg.reverb_mix_q15) >> 15;
    left += wet_l;
    right += wet_r;

    *left_out = gba89_clamp16(left);
    *right_out = gba89_clamp16(right);

    state->age += 1UL;
    if (state->age >= state->total_samples) {
        state->active = 0;
    }
}

void gba89_render_stereo(gba89_state *state, gba89_s16 *interleaved, gba89_u32 frames)
{
    gba89_u32 i;
    gba89_s16 left;
    gba89_s16 right;

    if (interleaved == 0) {
        return;
    }
    for (i = 0UL; i < frames; ++i) {
        gba89_render_one(state, &left, &right);
        interleaved[i * 2UL] = left;
        interleaved[i * 2UL + 1UL] = right;
    }
}

void gba89_render_mono(gba89_state *state, gba89_s16 *mono, gba89_u32 frames)
{
    gba89_u32 i;
    gba89_s16 left;
    gba89_s16 right;
    gba89_s32 sum;

    if (mono == 0) {
        return;
    }
    for (i = 0UL; i < frames; ++i) {
        gba89_render_one(state, &left, &right);
        sum = (gba89_s32)left + (gba89_s32)right;
        mono[i] = gba89_clamp16(sum >> 1);
    }
}
