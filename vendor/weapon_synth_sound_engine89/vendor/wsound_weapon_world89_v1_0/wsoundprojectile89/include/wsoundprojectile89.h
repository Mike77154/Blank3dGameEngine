#ifndef WSOUNDPROJECTILE89_H
#define WSOUNDPROJECTILE89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum wsoundprojectile89_mode_e {
    WSOUNDPROJECTILE89_SUBSONIC_WHIZ = 0,
    WSOUNDPROJECTILE89_SUPERSONIC_NWAVE = 1,
    WSOUNDPROJECTILE89_NEAR_MISS_SNAP = 2,
    WSOUNDPROJECTILE89_PELLET_SWARM = 3,
    WSOUNDPROJECTILE89_TRACER_FLYBY = 4
} wsoundprojectile89_mode;

typedef struct wsoundprojectile89_context_s {
    wsound89_u32 sample_rate;
    wsound89_u32 frame;
    wsound89_u32 total_frames;
    wsound89_u32 state;
    wsound89_i32 low;
    wsound89_i32 band;
    wsound89_i16 amplitude_q15;
    wsound89_u8 mode;
    wsound89_u8 active;
} wsoundprojectile89_context;

wsound89_result wsoundprojectile89_init(wsoundprojectile89_context *ctx, wsound89_u32 sample_rate, wsound89_u32 seed);
wsound89_result wsoundprojectile89_trigger(wsoundprojectile89_context *ctx, wsoundprojectile89_mode mode,
                                           wsound89_u16 amplitude_q15, wsound89_u16 proximity_q15);
wsound89_i16 wsoundprojectile89_process_sample(wsoundprojectile89_context *ctx);
int wsoundprojectile89_is_active(const wsoundprojectile89_context *ctx);
#ifdef __cplusplus
}
#endif
#endif
