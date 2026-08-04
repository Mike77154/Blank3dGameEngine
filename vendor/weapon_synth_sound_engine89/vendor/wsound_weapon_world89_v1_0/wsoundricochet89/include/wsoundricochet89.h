#ifndef WSOUNDRICOCHET89_H
#define WSOUNDRICOCHET89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wsoundricochet89_context_s {
    wsound89_i16 delay[32];
    wsound89_u32 state;
    wsound89_u32 frame;
    wsound89_u32 total_frames;
    wsound89_i32 envelope;
    wsound89_u16 index;
    wsound89_u16 delay_length;
    wsound89_u16 target_length;
    wsound89_i16 feedback_q15;
    wsound89_i16 noise_q15;
    wsound89_u8 active;
} wsoundricochet89_context;

wsound89_result wsoundricochet89_init(wsoundricochet89_context *ctx, wsound89_u32 seed);
wsound89_result wsoundricochet89_trigger(wsoundricochet89_context *ctx, wsound89_u16 energy_q15,
                                         wsound89_u16 grazing_q15, wsound89_u16 roughness_q15,
                                         wsound89_u32 sample_rate);
wsound89_i16 wsoundricochet89_process_sample(wsoundricochet89_context *ctx);
int wsoundricochet89_is_active(const wsoundricochet89_context *ctx);
#ifdef __cplusplus
}
#endif
#endif
