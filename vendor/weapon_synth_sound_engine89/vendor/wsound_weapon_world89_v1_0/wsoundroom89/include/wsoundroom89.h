#ifndef WSOUNDROOM89_H
#define WSOUNDROOM89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDROOM89_TAPS 6U

typedef enum wsoundroom89_preset_e {
    WSOUNDROOM89_SMALL = 0,
    WSOUNDROOM89_CORRIDOR = 1,
    WSOUNDROOM89_WAREHOUSE = 2,
    WSOUNDROOM89_TUNNEL = 3,
    WSOUNDROOM89_OUTDOOR = 4
} wsoundroom89_preset;

typedef struct wsoundroom89_context_s {
    wsound89_i16 *delay;
    wsound89_u32 capacity;
    wsound89_u32 write_pos;
    wsound89_u32 tap_delay[WSOUNDROOM89_TAPS];
    wsound89_i16 tap_gain_q15[WSOUNDROOM89_TAPS];
    wsound89_i16 tap_pan_q15[WSOUNDROOM89_TAPS];
    wsound89_i16 feedback_q15;
    wsound89_i16 wet_q15;
    wsound89_i16 damping_q15;
    wsound89_i32 damp_state;
} wsoundroom89_context;

wsound89_u32 wsoundroom89_required_samples(wsound89_u32 sample_rate, wsoundroom89_preset preset);
wsound89_result wsoundroom89_init(wsoundroom89_context *ctx, wsound89_u32 sample_rate,
                                  wsoundroom89_preset preset, wsound89_i16 *memory,
                                  wsound89_u32 memory_samples);
void wsoundroom89_reset(wsoundroom89_context *ctx);
void wsoundroom89_process_sample(wsoundroom89_context *ctx, wsound89_i16 input,
                                 wsound89_i16 *out_left, wsound89_i16 *out_right);
#ifdef __cplusplus
}
#endif
#endif
