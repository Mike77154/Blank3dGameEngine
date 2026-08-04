#ifndef GWEAPONBODY89_H
#define GWEAPONBODY89_H

/*
   gweaponbody89 - input-excited modal/comb weapon body synthesizer.
   Strict C89 core. Fixed-point signal path. No allocation. No math.h.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GWB89_VERSION_MAJOR 1
#define GWB89_VERSION_MINOR 0
#define GWB89_VERSION_PATCH 0
#define GWB89_Q15_ONE 32767
#define GWB89_MAX_MODES 8
#define GWB89_MAX_DELAY 512
#define GWB89_MAX_SAMPLE_RATE 48000

typedef signed short gwb89_s16;
typedef unsigned short gwb89_u16;
typedef signed int gwb89_s32;
typedef unsigned int gwb89_u32;

typedef enum gwb89_preset_id {
    GWB89_PRESET_PISTOL = 0,
    GWB89_PRESET_MAGNUM,
    GWB89_PRESET_SHOTGUN,
    GWB89_PRESET_RIFLE,
    GWB89_PRESET_SNIPER,
    GWB89_PRESET_LAUNCHER,
    GWB89_PRESET_COUNT
} gwb89_preset_id;

typedef struct gwb89_mode {
    gwb89_u16 delay_at_44100;
    gwb89_s16 feedback_q15;
    gwb89_s16 damping_q15;
    gwb89_s16 gain_q15;
} gwb89_mode;

typedef struct gwb89_preset {
    gwb89_u16 mode_count;
    gwb89_u16 exciter_ms;
    gwb89_u16 tail_ms;
    gwb89_s16 exciter_noise_q15;
    gwb89_s16 input_gain_q15;
    gwb89_s16 wet_q15;
    gwb89_s16 dry_q15;
    gwb89_s16 output_gain_q15;
    gwb89_mode mode[GWB89_MAX_MODES];
} gwb89_preset;

typedef struct gwb89_context {
    gwb89_u32 sample_rate;
    gwb89_u32 rng;
    gwb89_preset preset;
    gwb89_s16 delay[GWB89_MAX_MODES][GWB89_MAX_DELAY];
    gwb89_s16 damp_state[GWB89_MAX_MODES];
    gwb89_u16 write_pos[GWB89_MAX_MODES];
    gwb89_u16 runtime_delay[GWB89_MAX_MODES];
    gwb89_u32 exciter_pos;
    gwb89_u32 exciter_samples;
    gwb89_u32 tail_remaining;
    gwb89_s16 trigger_level_q15;
} gwb89_context;

int gwb89_platform_ok(void);
int gwb89_get_preset(gwb89_preset_id id, gwb89_preset *out_preset);
int gwb89_init(gwb89_context *ctx, gwb89_u32 sample_rate,
               const gwb89_preset *preset, gwb89_u32 seed);
void gwb89_reset(gwb89_context *ctx, gwb89_u32 seed);
void gwb89_trigger(gwb89_context *ctx, gwb89_s16 intensity_q15,
                   gwb89_u32 seed);
gwb89_s16 gwb89_process_sample(gwb89_context *ctx, gwb89_s16 input);
gwb89_u32 gwb89_process(gwb89_context *ctx, const gwb89_s16 *input,
                        gwb89_s16 *output, gwb89_u32 frames);
int gwb89_is_active(const gwb89_context *ctx);
const char *gwb89_preset_name(gwb89_preset_id id);
gwb89_u32 gwb89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
