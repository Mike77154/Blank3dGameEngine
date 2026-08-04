#ifndef WSOUNDBELTFEED89_H
#define WSOUNDBELTFEED89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wsoundbeltfeed89_context_s { wsound89_u32 rate,phase,step,state; wsound89_i32 env,pulse,res; wsound89_u16 tension_q15,metal_q15; wsound89_u8 active,gate,stage; } wsoundbeltfeed89_context;
wsound89_result wsoundbeltfeed89_init(wsoundbeltfeed89_context*,wsound89_u32,wsound89_u32);
wsound89_result wsoundbeltfeed89_start(wsoundbeltfeed89_context*,wsound89_u16,wsound89_u16,wsound89_u16);
void wsoundbeltfeed89_set_gate(wsoundbeltfeed89_context*,int);
wsound89_i16 wsoundbeltfeed89_process_sample(wsoundbeltfeed89_context*);
#ifdef __cplusplus
}
#endif
#endif
