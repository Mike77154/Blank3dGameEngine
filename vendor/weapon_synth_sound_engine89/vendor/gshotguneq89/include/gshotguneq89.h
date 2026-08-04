#ifndef GSHOTGUNEQ89_H
#define GSHOTGUNEQ89_H

/*
 * gshotguneq89 v1.0
 * Six-band fixed-point master for pump-action mechanical buses.
 * Strict C89, 44100 Hz, caller-owned state, no heap, no float/double.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GSGEQ89_SAMPLE_RATE 44100UL
#define GSGEQ89_BANDS 6
#define GSGEQ89_Q12_ONE 4096
#define GSGEQ89_Q15_ONE 32767

typedef signed short gsgeq89_s16;
typedef signed int gsgeq89_s32;
typedef unsigned int gsgeq89_u32;

typedef enum gsgeq89_profile_id_e {
    GSGEQ89_PROFILE_BALANCED = 0,
    GSGEQ89_PROFILE_REMINGTON_870,
    GSGEQ89_PROFILE_MOSSBERG_500_590,
    GSGEQ89_PROFILE_BENELLI_NOVA,
    GSGEQ89_PROFILE_WINCHESTER_SXP,
    GSGEQ89_PROFILE_COUNT
} gsgeq89_profile_id;

typedef struct gsgeq89_preset_s {
    const char *name;
    gsgeq89_s16 band_gain_q12[GSGEQ89_BANDS];
    gsgeq89_s16 drive_q15;
    gsgeq89_s16 transient_q15;
    gsgeq89_s16 output_q15;
} gsgeq89_preset;

typedef struct gsgeq89_context_s {
    gsgeq89_preset preset;
    gsgeq89_s32 lowpass[5];
    gsgeq89_s32 transient_lp;
} gsgeq89_context;

int gsgeq89_get_preset(gsgeq89_profile_id profile, gsgeq89_preset *out_preset);
const char *gsgeq89_profile_name(gsgeq89_profile_id profile);
void gsgeq89_init(gsgeq89_context *ctx, gsgeq89_profile_id profile);
void gsgeq89_reset(gsgeq89_context *ctx);
void gsgeq89_set_preset(gsgeq89_context *ctx, const gsgeq89_preset *preset);
gsgeq89_s16 gsgeq89_process_sample(gsgeq89_context *ctx, gsgeq89_s16 input);
gsgeq89_u32 gsgeq89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
