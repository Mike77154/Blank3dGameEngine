#ifndef WSOUNDOUTDOOR89_H
#define WSOUNDOUTDOOR89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDOUTDOOR89_TAPS 8
typedef enum wsoundoutdoor89_preset_e { WSOUNDOUTDOOR89_OPEN_FIELD=0, WSOUNDOUTDOOR89_URBAN_CANYON=1, WSOUNDOUTDOOR89_FOREST=2, WSOUNDOUTDOOR89_MOUNTAIN=3 } wsoundoutdoor89_preset;
typedef struct wsoundoutdoor89_context_s { wsound89_i16 *memory; wsound89_u32 capacity,index; wsound89_u32 delay[WSOUNDOUTDOOR89_TAPS]; wsound89_i16 gain_q15[WSOUNDOUTDOOR89_TAPS]; wsound89_i32 low[WSOUNDOUTDOOR89_TAPS]; wsound89_u8 taps; } wsoundoutdoor89_context;
wsound89_result wsoundoutdoor89_init(wsoundoutdoor89_context*,wsound89_i16*,wsound89_u32,wsound89_u32,wsoundoutdoor89_preset);
wsound89_i16 wsoundoutdoor89_process_sample(wsoundoutdoor89_context*,wsound89_i16);
wsound89_i16 wsoundoutdoor89_process_wet_sample(wsoundoutdoor89_context*,wsound89_i16);
wsound89_u32 wsoundoutdoor89_required_frames(wsound89_u32,wsoundoutdoor89_preset);
#ifdef __cplusplus
}
#endif
#endif
