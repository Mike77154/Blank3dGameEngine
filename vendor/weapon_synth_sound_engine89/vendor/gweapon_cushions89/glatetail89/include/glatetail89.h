#ifndef GLATETAIL89_H
#define GLATETAIL89_H

/* Four-line feedback delay network for late diffuse weapon tails. */
#ifdef __cplusplus
extern "C" {
#endif

#define GLT89_VERSION_MAJOR 1
#define GLT89_VERSION_MINOR 0
#define GLT89_VERSION_PATCH 0
#define GLT89_Q15_ONE 32767
#define GLT89_LINES 4
#define GLT89_MAX_DELAY 4096
#define GLT89_MAX_PREDELAY 4096
#define GLT89_MAX_SAMPLE_RATE 48000

typedef signed short glt89_s16;
typedef unsigned short glt89_u16;
typedef signed int glt89_s32;
typedef unsigned int glt89_u32;

typedef enum glt89_preset_id {
    GLT89_PRESET_SMALL_ROOM = 0,
    GLT89_PRESET_CORRIDOR,
    GLT89_PRESET_WAREHOUSE,
    GLT89_PRESET_TUNNEL,
    GLT89_PRESET_EXTERIOR,
    GLT89_PRESET_CATHEDRAL,
    GLT89_PRESET_COUNT
} glt89_preset_id;

typedef struct glt89_preset {
    glt89_u16 predelay_ms;
    glt89_u16 delay_ms[GLT89_LINES];
    glt89_s16 feedback_q15;
    glt89_s16 damping_q15;
    glt89_s16 input_gain_q15;
    glt89_s16 wet_q15;
    glt89_s16 dry_q15;
    glt89_s16 output_gain_q15;
    glt89_u16 bounded_tail_ms;
} glt89_preset;

typedef struct glt89_context {
    glt89_u32 sample_rate;
    glt89_preset preset;
    glt89_s16 predelay[GLT89_MAX_PREDELAY];
    glt89_u16 predelay_write;
    glt89_u16 predelay_samples;
    glt89_s16 delay[GLT89_LINES][GLT89_MAX_DELAY];
    glt89_u16 write_pos[GLT89_LINES];
    glt89_u16 runtime_delay[GLT89_LINES];
    glt89_s16 damp_state[GLT89_LINES];
    glt89_u32 tail_remaining;
} glt89_context;

int glt89_platform_ok(void);
int glt89_get_preset(glt89_preset_id id, glt89_preset *out_preset);
int glt89_init(glt89_context *ctx, glt89_u32 sample_rate,
               const glt89_preset *preset);
void glt89_reset(glt89_context *ctx);
glt89_s16 glt89_process_sample(glt89_context *ctx, glt89_s16 input);
glt89_u32 glt89_process(glt89_context *ctx, const glt89_s16 *input,
                        glt89_s16 *output, glt89_u32 frames);
void glt89_inject(glt89_context *ctx, glt89_s16 impulse);
int glt89_is_active(const glt89_context *ctx);
const char *glt89_preset_name(glt89_preset_id id);
glt89_u32 glt89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
