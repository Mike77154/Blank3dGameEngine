#ifndef GBALLISTICCRACK89_H
#define GBALLISTICCRACK89_H

/* Procedural supersonic projectile N-wave layer. C89, fixed point, no heap. */
#ifdef __cplusplus
extern "C" {
#endif

#define GBC89_VERSION_MAJOR 1
#define GBC89_VERSION_MINOR 0
#define GBC89_VERSION_PATCH 0
#define GBC89_Q15_ONE 32767
#define GBC89_MAX_SAMPLE_RATE 48000

typedef signed short gbc89_s16;
typedef unsigned short gbc89_u16;
typedef signed int gbc89_s32;
typedef unsigned int gbc89_u32;

typedef enum gbc89_preset_id {
    GBC89_PRESET_NEAR = 0,
    GBC89_PRESET_MEDIUM,
    GBC89_PRESET_FAR,
    GBC89_PRESET_RIFLE_PASS,
    GBC89_PRESET_SNIPER_PASS,
    GBC89_PRESET_COUNT
} gbc89_preset_id;

typedef struct gbc89_preset {
    gbc89_u16 nwave_us;
    gbc89_u16 air_ms;
    gbc89_s16 peak_q15;
    gbc89_s16 edge_noise_q15;
    gbc89_s16 air_noise_q15;
    gbc89_u16 air_cut_hz;
    gbc89_s16 output_gain_q15;
} gbc89_preset;

typedef struct gbc89_context {
    gbc89_u32 sample_rate;
    gbc89_u32 rng;
    gbc89_preset preset;
    gbc89_u32 delay_remaining;
    gbc89_u32 wave_pos;
    gbc89_u32 wave_samples;
    gbc89_u32 air_pos;
    gbc89_u32 air_samples;
    gbc89_s32 air_lp;
    gbc89_s16 air_alpha_q15;
    gbc89_s16 intensity_q15;
    gbc89_u16 active;
} gbc89_context;

int gbc89_platform_ok(void);
int gbc89_get_preset(gbc89_preset_id id, gbc89_preset *out_preset);
int gbc89_init(gbc89_context *ctx, gbc89_u32 sample_rate,
               const gbc89_preset *preset, gbc89_u32 seed);
void gbc89_reset(gbc89_context *ctx, gbc89_u32 seed);
void gbc89_trigger(gbc89_context *ctx, gbc89_u16 delay_ms,
                   gbc89_s16 intensity_q15, gbc89_u32 seed);
gbc89_s16 gbc89_process_sample(gbc89_context *ctx);
gbc89_u32 gbc89_render(gbc89_context *ctx, gbc89_s16 *output,
                       gbc89_u32 frames);
int gbc89_is_active(const gbc89_context *ctx);
const char *gbc89_preset_name(gbc89_preset_id id);
gbc89_u32 gbc89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
