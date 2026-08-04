#ifndef WSOUNDMETRICS89_H
#define WSOUNDMETRICS89_H

#include "wsound89_common.h"
#include "wsounddna89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSOUNDM89_VERSION_MAJOR 1
#define WSOUNDM89_VERSION_MINOR 0
#define WSOUNDM89_VERSION_PATCH 0
#define WSOUNDM89_BANDS 3U
#define WSOUNDM89_INVALID_FRAME 0xFFFFFFFFU

#define WSOUNDM89_FAIL_PEAK       0x0001U
#define WSOUNDM89_FAIL_CREST      0x0002U
#define WSOUNDM89_FAIL_LOW_BAND   0x0004U
#define WSOUNDM89_FAIL_MID_BAND   0x0008U
#define WSOUNDM89_FAIL_HIGH_BAND  0x0010U
#define WSOUNDM89_FAIL_ATTACK     0x0020U
#define WSOUNDM89_FAIL_DECAY      0x0040U
#define WSOUNDM89_FAIL_CLIPPING   0x0080U

typedef struct wsoundmetrics89_result_s {
    wsound89_u32 frames;
    wsound89_u16 peak_abs;
    wsound89_u16 rms_q15;
    wsound89_u16 crest_q8;
    wsound89_u16 band_ratio_q15[WSOUNDM89_BANDS];
    wsound89_u16 brightness_q15;
    wsound89_u16 attack_ms_x10;
    wsound89_u16 decay_ms;
    wsound89_u16 zero_cross_q15;
    wsound89_u32 peak_frame;
    wsound89_u32 clipped_samples;
} wsoundmetrics89_result;

typedef struct wsoundmetrics89_target_s {
    wsound89_u16 peak_min;
    wsound89_u16 peak_max;
    wsound89_u16 crest_min_q8;
    wsound89_u16 crest_max_q8;
    wsound89_u16 band_min_q15[WSOUNDM89_BANDS];
    wsound89_u16 band_max_q15[WSOUNDM89_BANDS];
    wsound89_u16 attack_max_ms_x10;
    wsound89_u16 decay_min_ms;
    wsound89_u16 decay_max_ms;
    wsound89_u16 clipping_allowed;
} wsoundmetrics89_target;

typedef struct wsoundmetrics89_context_s {
    wsound89_u32 sample_rate;
    wsound89_u32 frames;
    wsound89_u32 analysis_samples;
    wsound89_u32 square_sum;
    wsound89_u32 band_energy[WSOUNDM89_BANDS];
    wsound89_u32 peak_frame;
    wsound89_u32 onset_frame;
    wsound89_u32 decay_frame;
    wsound89_u32 clipped_samples;
    wsound89_u32 zero_crossings;
    wsound89_i32 low_state;
    wsound89_i32 high_state;
    wsound89_i32 envelope;
    wsound89_i16 previous;
    wsound89_u16 peak_abs;
    wsound89_u16 low_alpha_q15;
    wsound89_u16 high_alpha_q15;
    wsound89_u8 square_phase;
    wsound89_u8 band_phase;
    wsound89_u8 enabled;
} wsoundmetrics89_context;

wsound89_result wsoundmetrics89_init(wsoundmetrics89_context *ctx,
                                     wsound89_u32 sample_rate);
void wsoundmetrics89_reset(wsoundmetrics89_context *ctx);
void wsoundmetrics89_set_enabled(wsoundmetrics89_context *ctx, int enabled);
void wsoundmetrics89_push_mono(wsoundmetrics89_context *ctx,
                               wsound89_i16 sample);
void wsoundmetrics89_push_stereo(wsoundmetrics89_context *ctx,
                                 wsound89_i16 left,
                                 wsound89_i16 right);
wsound89_result wsoundmetrics89_finish(const wsoundmetrics89_context *ctx,
                                       wsoundmetrics89_result *result);
void wsoundmetrics89_target_defaults(const wsounddna89_profile *profile,
                                     wsounddna89_mode mode,
                                     wsoundmetrics89_target *target);
wsound89_u16 wsoundmetrics89_validate(const wsoundmetrics89_result *result,
                                      const wsoundmetrics89_target *target);
wsound89_u16 wsoundmetrics89_score_q15(const wsoundmetrics89_result *result,
                                       const wsoundmetrics89_target *target);
wsound89_u32 wsoundmetrics89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
