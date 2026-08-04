#ifndef GMUZZLEGAS89_H
#define GMUZZLEGAS89_H

/* Procedural propellant-gas turbulence layer. C89, fixed point, no heap. */
#ifdef __cplusplus
extern "C" {
#endif

#define GMG89_VERSION_MAJOR 1
#define GMG89_VERSION_MINOR 0
#define GMG89_VERSION_PATCH 0
#define GMG89_Q15_ONE 32767
#define GMG89_MAX_SAMPLE_RATE 48000

typedef signed short gmg89_s16;
typedef unsigned short gmg89_u16;
typedef signed int gmg89_s32;
typedef unsigned int gmg89_u32;

typedef enum gmg89_preset_id {
    GMG89_PRESET_PISTOL = 0,
    GMG89_PRESET_MAGNUM,
    GMG89_PRESET_SHOTGUN,
    GMG89_PRESET_RIFLE,
    GMG89_PRESET_SNIPER,
    GMG89_PRESET_LAUNCHER,
    GMG89_PRESET_COUNT
} gmg89_preset_id;

typedef struct gmg89_band {
    gmg89_u16 decay_ms;
    gmg89_s16 level_q15;
} gmg89_band;

typedef struct gmg89_preset {
    gmg89_band low;
    gmg89_band mid;
    gmg89_band high;
    gmg89_u16 low_cut_hz;
    gmg89_u16 mid_low_hz;
    gmg89_u16 mid_high_hz;
    gmg89_u16 high_cut_hz;
    gmg89_s16 transient_q15;
    gmg89_s16 asymmetry_q15;
    gmg89_s16 drive_q15;
    gmg89_s16 output_gain_q15;
} gmg89_preset;

typedef struct gmg89_context {
    gmg89_u32 sample_rate;
    gmg89_u32 rng;
    gmg89_preset preset;
    gmg89_s32 lp_low;
    gmg89_s32 lp_mid_low;
    gmg89_s32 lp_mid_high;
    gmg89_s32 lp_high;
    gmg89_s16 a_low;
    gmg89_s16 a_mid_low;
    gmg89_s16 a_mid_high;
    gmg89_s16 a_high;
    gmg89_u32 pos;
    gmg89_u32 low_samples;
    gmg89_u32 mid_samples;
    gmg89_u32 high_samples;
    gmg89_s16 intensity_q15;
} gmg89_context;

int gmg89_platform_ok(void);
int gmg89_get_preset(gmg89_preset_id id, gmg89_preset *out_preset);
int gmg89_init(gmg89_context *ctx, gmg89_u32 sample_rate,
               const gmg89_preset *preset, gmg89_u32 seed);
void gmg89_reset(gmg89_context *ctx, gmg89_u32 seed);
void gmg89_trigger(gmg89_context *ctx, gmg89_s16 intensity_q15,
                   gmg89_u32 seed);
gmg89_s16 gmg89_process_sample(gmg89_context *ctx);
gmg89_u32 gmg89_render(gmg89_context *ctx, gmg89_s16 *output,
                       gmg89_u32 frames);
int gmg89_is_active(const gmg89_context *ctx);
const char *gmg89_preset_name(gmg89_preset_id id);
gmg89_u32 gmg89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
