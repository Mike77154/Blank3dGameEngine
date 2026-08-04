#ifndef WSOUNDSPATIAL89_H
#define WSOUNDSPATIAL89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wsoundspatial89_context_s { wsound89_i16 *left_delay,*right_delay; wsound89_u16 capacity,index,itd; wsound89_i16 pan_q15; wsound89_i32 far_low; wsound89_u16 shadow_q15; } wsoundspatial89_context;
wsound89_result wsoundspatial89_init(wsoundspatial89_context*,wsound89_i16*,wsound89_i16*,wsound89_u16);
void wsoundspatial89_set_azimuth(wsoundspatial89_context*,wsound89_i16,wsound89_u16,wsound89_u16);
void wsoundspatial89_process_sample(wsoundspatial89_context*,wsound89_i16,wsound89_i16*,wsound89_i16*);
#ifdef __cplusplus
}
#endif
#endif
