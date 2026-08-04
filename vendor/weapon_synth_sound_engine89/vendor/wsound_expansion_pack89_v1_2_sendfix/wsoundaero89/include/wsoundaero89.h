#ifndef WSOUNDAERO89_H
#define WSOUNDAERO89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundaero89_mode_e { WSOUNDAERO89_BULLET=0, WSOUNDAERO89_ROCKET=1, WSOUNDAERO89_GRENADE=2, WSOUNDAERO89_PELLET_CLOUD=3, WSOUNDAERO89_DEBRIS=4, WSOUNDAERO89_NEAR_CAMERA=5 } wsoundaero89_mode;
typedef struct wsoundaero89_context_s { wsound89_u32 state,phase,phase_inc,age,total; wsound89_i32 env,low,band; wsound89_u16 speed_q15,size_q15; wsound89_u8 active,gate; wsoundaero89_mode mode; } wsoundaero89_context;
wsound89_result wsoundaero89_init(wsoundaero89_context*,wsound89_u32,wsound89_u32);
wsound89_result wsoundaero89_trigger(wsoundaero89_context*,wsoundaero89_mode,wsound89_u16,wsound89_u16,wsound89_u32);
void wsoundaero89_set_gate(wsoundaero89_context*,int);
wsound89_i16 wsoundaero89_process_sample(wsoundaero89_context*);
int wsoundaero89_is_active(const wsoundaero89_context*);
#ifdef __cplusplus
}
#endif
#endif
