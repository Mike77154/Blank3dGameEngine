#ifndef GWEAPONFOLEY89_H
#define GWEAPONFOLEY89_H

/*
    gweaponfoley89 - dry mechanical weapon Foley from three noise sources.
    C89 core. No malloc/calloc/realloc/free. No float/double. No math.h.

    Assumptions:
      - signed short is at least 16 bits
      - signed long is at least 32 bits
      - unsigned long is at least 32 bits
      - audio rate is fixed at 44100 Hz

    The caller owns the context. Keep it static/global on small-stack targets.
*/

#define GWF89_SAMPLE_RATE       44100
#define GWF89_MAX_HITS          8
#define GWF89_CHORUS_BUFFER     512
#define GWF89_REVERB_BUFFER     1024
#define GWF89_ALLPASS_BUFFER    512
#define GWF89_METAL_BUFFER      64
#define GWF89_METAL_MODES       4
#define GWF89_VARIANT_COUNT     3
#define GWF89_VARIANT_AUTO      255

#define GWF89_Q15_ONE           32767
#define GWF89_EQ_ONE            4096

#define GWF89_VERSION_MAJOR     1
#define GWF89_VERSION_MINOR     6
#define GWF89_VERSION_PATCH     0

typedef signed short gwf89_s16;
typedef signed long  gwf89_s32;
typedef unsigned long gwf89_u32;

typedef enum gwf89_preset_id {
    GWF89_PISTOL_EMPTY = 0,
    GWF89_PISTOL_HANDLING,
    GWF89_MAGNUM_EMPTY,
    GWF89_MAGNUM_LATCH,
    GWF89_SNIPER_EMPTY,
    GWF89_SNIPER_BOLT_DRY,
    GWF89_SMG_EMPTY,
    GWF89_SMG_SELECTOR,
    GWF89_LAUNCHER_EMPTY,
    GWF89_LAUNCHER_LATCH,
    GWF89_SHOTGUN_EMPTY,
    GWF89_SHOTGUN_SAFETY,
    GWF89_PRESET_COUNT
} gwf89_preset_id;

typedef enum gwf89_speed {
    GWF89_SPEED_SLOW = 0,
    GWF89_SPEED_NORMAL,
    GWF89_SPEED_FAST
} gwf89_speed;

typedef struct gwf89_tone_shape {
    unsigned short attack_samples;
    unsigned short decay_samples;
    unsigned short hold_samples;
    unsigned short release_samples;
    gwf89_s16 sustain_q15;
    gwf89_s16 cutoff_base_q15;
    gwf89_s16 cutoff_env_q15;
    gwf89_s16 emphasis_q15;
    gwf89_s16 high_mix_q15;
    gwf89_s16 band_mix_q15;
} gwf89_tone_shape;

typedef struct gwf89_hit {
    unsigned short offset_samples;
    unsigned short attack_samples;
    unsigned short decay_samples;
    unsigned short hold_samples;
    unsigned short release_samples;
    gwf89_s16 peak_q15;
    gwf89_s16 sustain_q15;
    gwf89_s16 low_mix_q15;
    gwf89_s16 white_mix_q15;
    gwf89_s16 high_mix_q15;
    signed short eq_gain_q12[6];
    gwf89_s16 drive_q15;
    gwf89_tone_shape tone;
    gwf89_s16 body_wet_q15;
    gwf89_s16 body_feedback_q15;
    gwf89_s16 body_damping_q15;
} gwf89_hit;

typedef struct gwf89_preset {
    const char *name;
    unsigned char hit_count;
    gwf89_hit hit[GWF89_MAX_HITS];
    signed short eq_gain_q12[6];
    gwf89_s16 drive_q15;
    gwf89_s16 metal_wet_q15;
    gwf89_s16 metal_feedback_q15;
    gwf89_s16 metal_damping_q15;
    unsigned char metal_delay[GWF89_METAL_MODES];
    gwf89_s16 chorus_wet_q15;
    unsigned short chorus_base_delay;
    unsigned short chorus_depth;
    unsigned short chorus_phase_step;
    gwf89_s16 reverb_wet_q15;
    gwf89_s16 reverb_feedback_q15;
    unsigned short reverb_delay_a;
    unsigned short reverb_delay_b;
    unsigned short allpass_delay;
    unsigned short tail_samples;
} gwf89_preset;

typedef struct gwf89_context {
    const gwf89_preset *preset;
    gwf89_hit runtime_hit[GWF89_MAX_HITS];
    gwf89_u32 rng_low;
    gwf89_u32 rng_white;
    gwf89_u32 rng_high;
    gwf89_s16 low_noise_state;
    gwf89_s16 high_noise_prev;
    gwf89_s16 hit_eq_lp[GWF89_MAX_HITS][5];
    gwf89_s16 hit_tone_lp1[GWF89_MAX_HITS];
    gwf89_s16 hit_tone_lp2[GWF89_MAX_HITS];
    gwf89_s16 master_eq_lp[5];
    gwf89_s16 hit_metal_buffer[GWF89_MAX_HITS][GWF89_METAL_MODES][GWF89_METAL_BUFFER];
    gwf89_s16 hit_metal_lp[GWF89_MAX_HITS][GWF89_METAL_MODES];
    unsigned short hit_metal_write[GWF89_MAX_HITS];
    gwf89_s16 chorus_buffer[GWF89_CHORUS_BUFFER];
    unsigned short chorus_write;
    unsigned short chorus_phase;
    gwf89_s16 reverb_a[GWF89_REVERB_BUFFER];
    gwf89_s16 reverb_b[GWF89_REVERB_BUFFER];
    unsigned short reverb_a_pos;
    unsigned short reverb_b_pos;
    gwf89_s16 allpass[GWF89_ALLPASS_BUFFER];
    unsigned short allpass_pos;
    gwf89_s16 room_send_q15;
    gwf89_s16 last_dry;
    gwf89_s16 last_room;
    gwf89_u32 sample_clock;
    gwf89_u32 end_clock;
    unsigned char variant;
    unsigned char speed;
    unsigned char active;
} gwf89_context;

void gwf89_init(gwf89_context *ctx, gwf89_u32 seed);
void gwf89_reset(gwf89_context *ctx, gwf89_u32 seed);
void gwf89_trigger(gwf89_context *ctx, gwf89_preset_id preset_id);
void gwf89_trigger_seeded(gwf89_context *ctx, gwf89_preset_id preset_id, gwf89_u32 seed);
void gwf89_trigger_ex(gwf89_context *ctx, gwf89_preset_id preset_id,
                      gwf89_u32 seed, unsigned char variant,
                      gwf89_speed speed);
void gwf89_set_room_send(gwf89_context *ctx, gwf89_s16 room_send_q15);
gwf89_s16 gwf89_process_sample(gwf89_context *ctx);
gwf89_s16 gwf89_process_sample_stems(gwf89_context *ctx,
                                      gwf89_s16 *dry_out,
                                      gwf89_s16 *room_out);
unsigned long gwf89_process(gwf89_context *ctx, gwf89_s16 *dst,
                            unsigned long sample_count);
int gwf89_is_active(const gwf89_context *ctx);
const gwf89_preset *gwf89_get_preset(gwf89_preset_id preset_id);
const char *gwf89_preset_name(gwf89_preset_id preset_id);
unsigned long gwf89_context_bytes(void);

#endif
