#ifndef WSOUNDLISTENER89_H
#define WSOUNDLISTENER89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundlistener89_protection_e { WSOUNDLISTENER89_NONE=0, WSOUNDLISTENER89_EARPLUGS=1, WSOUNDLISTENER89_ELECTRONIC=2 } wsoundlistener89_protection;
typedef struct wsoundlistener89_context_s { wsound89_i32 exposure_q15,gain_q15,low_l,low_r,ring_env; wsound89_u32 phase,phase_inc; wsound89_u16 recovery_q15; } wsoundlistener89_context;
wsound89_result wsoundlistener89_init(wsoundlistener89_context*,wsound89_u32);
void wsoundlistener89_expose(wsoundlistener89_context*,wsound89_u16,wsound89_u16,wsoundlistener89_protection);
void wsoundlistener89_process_stereo(wsoundlistener89_context*,wsound89_i16,wsound89_i16,wsound89_i16*,wsound89_i16*);
#ifdef __cplusplus
}
#endif
#endif
