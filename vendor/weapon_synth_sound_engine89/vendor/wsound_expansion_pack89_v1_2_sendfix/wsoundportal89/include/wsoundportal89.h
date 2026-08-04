#ifndef WSOUNDPORTAL89_H
#define WSOUNDPORTAL89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wsoundportal89_context_s { wsound89_i16 *memory; wsound89_u32 capacity,index,delay; wsound89_i32 low,midlow; wsound89_u16 low_q15,mid_q15,high_q15,opening_q15; } wsoundportal89_context;
wsound89_result wsoundportal89_init(wsoundportal89_context*,wsound89_i16*,wsound89_u32,wsound89_u32);
wsound89_result wsoundportal89_set_path(wsoundportal89_context*,wsound89_u32,wsound89_u16,wsound89_u16,wsound89_u16,wsound89_u16);
wsound89_i16 wsoundportal89_process_sample(wsoundportal89_context*,wsound89_i16);
#ifdef __cplusplus
}
#endif
#endif
