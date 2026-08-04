#ifndef WSOUNDPROP89_H
#define WSOUNDPROP89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsoundprop89_context_s {
    wsound89_i16 *delay;
    wsound89_u32 capacity;
    wsound89_u32 write_pos;
    wsound89_u32 delay_samples;
    wsound89_u32 sample_rate;
    wsound89_i32 lowpass;
    wsound89_i16 gain_q15;
    wsound89_i16 directivity_q15;
    wsound89_i16 occlusion_q15;
    wsound89_u8 lowpass_shift;
} wsoundprop89_context;

wsound89_u32 wsoundprop89_required_samples(wsound89_u32 sample_rate, wsound89_u32 max_distance_cm,
                                           wsound89_u32 speed_cm_s);
wsound89_result wsoundprop89_init(wsoundprop89_context *ctx, wsound89_u32 sample_rate,
                                  wsound89_i16 *delay_memory, wsound89_u32 delay_samples);
wsound89_result wsoundprop89_set_path(wsoundprop89_context *ctx, wsound89_u32 distance_cm,
                                      wsound89_u32 speed_cm_s, wsound89_i16 muzzle_dot_q15,
                                      wsound89_u16 occlusion_q15);
wsound89_i16 wsoundprop89_process_sample(wsoundprop89_context *ctx, wsound89_i16 input);
void wsoundprop89_reset(wsoundprop89_context *ctx);
#ifdef __cplusplus
}
#endif
#endif
