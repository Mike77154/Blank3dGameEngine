#ifndef GSHOTGUNSEQUENCE89_H
#define GSHOTGUNSEQUENCE89_H

/*
 * gshotgunsequence89 v1.4
 * Procedural shotgun report/mechanism sequence orchestrator.
 * Strict C89 core, fixed-point/integer signal path, no heap allocation.
 *
 * The pump bus is intentionally complementary:
 *   gpump89 = low/mid mechanical mass and stage impacts (the "chuk")
 *   chuecka89 = bright articulated metal harmonic (the "chic")
 *   shotpumpkin89 = continuous rail friction, chatter and endpoint texture
 *
 * Shell insertion remains chuecka-only and uses the darker insert presets.
 * Pumping uses a brightened/retimed chuecka gesture plus gpump89,
 * with a midpoint gklek89 carrier impact and a low Foley tik.
 */

#include "gpaah89.h"
#include "gpump89.h"
#include "chuecka89.h"
#include "shotpumpkin89.h"
#include "gweaponfoley89.h"
#include "gklek89.h"
#include "gshotguneq89.h"
#include "gweaponbody89.h"
#include "gmuzzlegas89.h"
#include "gballisticcrack89.h"
#include "glatetail89.h"
#include "gcinemathump89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short gss89_s16;
typedef signed int gss89_s32;
typedef unsigned int gss89_u32;
typedef unsigned short gss89_u16;
typedef unsigned char gss89_u8;

#define GSS89_VERSION_MAJOR 1
#define GSS89_VERSION_MINOR 4
#define GSS89_VERSION_PATCH 0
#define GSS89_SAMPLE_RATE 44100U
#define GSS89_Q15_ONE 32767
#define GSS89_TEXTURE_VOICES 2
#define GSS89_RECOMMENDED_TEXTURE_FRAMES 49152U

typedef enum gss89_result_e {
    GSS89_OK = 0,
    GSS89_BAD_ARGUMENT = -1,
    GSS89_UNSUPPORTED_RATE = -2,
    GSS89_TEXTURE_BUFFER_MISSING = -3,
    GSS89_TEXTURE_BUFFER_SMALL = -4,
    GSS89_DEPENDENCY_ERROR = -5
} gss89_result;


typedef enum gss89_shotgun_model_e {
    GSS89_SHOTGUN_MODEL_BALANCED = 0,
    GSS89_SHOTGUN_MODEL_REMINGTON_870,
    GSS89_SHOTGUN_MODEL_MOSSBERG_500_590,
    GSS89_SHOTGUN_MODEL_BENELLI_NOVA,
    GSS89_SHOTGUN_MODEL_WINCHESTER_SXP,
    GSS89_SHOTGUN_MODEL_COUNT
} gss89_shotgun_model;

typedef void (*gss89_tek_trigger_fn)(void *user, gss89_u32 seed,
                                      gss89_u16 delay_ms,
                                      gss89_shotgun_model model);
typedef gss89_s16 (*gss89_tek_process_fn)(void *user);
typedef int (*gss89_tek_active_fn)(const void *user);

typedef enum gss89_texture_role_e {
    GSS89_TEXTURE_GENERAL = 0,
    GSS89_TEXTURE_PUMP_HARMONIC = 1
} gss89_texture_role;

/* Source-compatibility alias retained from v1.0. */
#define GSS89_TEXTURE_PUMP_BODY GSS89_TEXTURE_PUMP_HARMONIC

typedef struct gss89_mix_s {
    gss89_s16 report_q15;
    gss89_s16 report_body_q15;
    gss89_s16 report_gas_q15;
    gss89_s16 report_crack_q15;
    gss89_s16 report_thump_q15;
    gss89_s16 report_tail_q15;

    /* Pump pair: gpump mass + equally present chuecka upper harmonic. */
    gss89_s16 pump_primary_q15;
    gss89_s16 pump_chuecka_q15;

    /* Fine alignment trims after automatic texture-to-stage synchronization. */
    gss89_u16 pump_chuecka_delay_ms;

    /* Continuous rail/friction layer supplied by shotpumpkin89. */
    gss89_s16 pump_shotpumpkin_q15;
    gss89_u16 pump_shotpumpkin_delay_ms;

    /* Mid-cycle articulation: dry klek plus a quieter Foley particle tik. */
    gss89_s16 pump_klek_q15;
    gss89_s16 pump_tik_q15;

    gss89_s16 general_chuecka_q15;
    gss89_s16 foley_q15;
    gss89_s16 master_q15;
} gss89_mix;

typedef struct gss89_texture_voice_s {
    gss89_s16 *pcm;
    gss89_u32 capacity;
    gss89_u32 frames;
    gss89_u32 position;
    gss89_u32 delay_frames;
    gss89_u8 active;
    gss89_u8 role;
} gss89_texture_voice;

typedef struct gss89_context_s {
    gss89_mix mix;
    gss89_u32 sample_rate;
    gss89_u32 seed_counter;

    gpaah89_state report;
    gpump89_context pump;
    sp89_state shotpumpkin;
    sp89_preset shotpumpkin_preset;
    gss89_u32 shotpumpkin_delay_frames;
    ch89_context chuecka_synth;
    gkl89_context klek;
    gwf89_context foley;
    gsgeq89_context pump_master;
    gss89_shotgun_model shotgun_model;
    gss89_u8 pump_master_enabled;

    gss89_tek_trigger_fn tek_trigger;
    gss89_tek_process_fn tek_process;
    gss89_tek_active_fn tek_active;
    void *tek_user;

    gss89_u32 pump_tik_delay_frames;
    gss89_u32 pump_tik_seed;
    unsigned char pump_tik_pending;
    unsigned char foley_role;

    gwb89_context body;
    gmg89_context gas;
    gbc89_context crack;
    glt89_context tail;
    gct89_context thump;

    gss89_texture_voice texture[GSS89_TEXTURE_VOICES];
} gss89_context;

void gss89_mix_default(gss89_mix *mix);

gss89_result gss89_init(gss89_context *ctx,
                         gss89_u32 sample_rate,
                         gss89_u32 seed);

/* Caller-owned static scratch. Bind at least two voices for overlapping reloads. */
gss89_result gss89_bind_texture_buffer(gss89_context *ctx,
                                        gss89_u16 voice_index,
                                        gss89_s16 *pcm,
                                        gss89_u32 frame_capacity);

void gss89_set_mix(gss89_context *ctx, const gss89_mix *mix);
void gss89_set_shotpumpkin_preset(gss89_context *ctx, sp89_preset preset);
void gss89_set_shotgun_model(gss89_context *ctx, gss89_shotgun_model model);
const char *gss89_shotgun_model_name(gss89_shotgun_model model);
void gss89_enable_pump_master(gss89_context *ctx, int enabled);
void gss89_set_pump_master_profile(gss89_context *ctx,
                                    gsgeq89_profile_id profile);
void gss89_set_pump_master_custom(gss89_context *ctx,
                                   const gsgeq89_preset *preset);
void gss89_set_tek_provider(gss89_context *ctx,
                            gss89_tek_trigger_fn trigger_fn,
                            gss89_tek_process_fn process_fn,
                            gss89_tek_active_fn active_fn,
                            void *user);

void gss89_trigger_report(gss89_context *ctx, gss89_u32 seed);

gss89_result gss89_trigger_pump(gss89_context *ctx,
                                 int gpump_preset,
                                 gss89_u32 seed);

gss89_result gss89_trigger_model_pump(gss89_context *ctx,
                                       gss89_shotgun_model model,
                                       gss89_u32 seed);

gss89_result gss89_trigger_chuecka(gss89_context *ctx,
                                    ch89_preset preset,
                                    gss89_u16 repetitions,
                                    gss89_u16 intensity_q15,
                                    gss89_u32 seed);

void gss89_trigger_foley(gss89_context *ctx,
                         gwf89_preset_id preset,
                         gss89_u32 seed);

gss89_s16 gss89_process_sample(gss89_context *ctx);
gss89_u32 gss89_render_mono(gss89_context *ctx,
                             gss89_s16 *output,
                             gss89_u32 frames);
int gss89_is_active(const gss89_context *ctx);
gss89_u32 gss89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
