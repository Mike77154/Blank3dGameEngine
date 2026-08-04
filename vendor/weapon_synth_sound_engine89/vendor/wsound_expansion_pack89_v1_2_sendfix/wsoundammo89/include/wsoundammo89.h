#ifndef WSOUNDAMMO89_H
#define WSOUNDAMMO89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundammo89_type_e { WSOUNDAMMO89_BOX_MAG=0, WSOUNDAMMO89_DRUM=1, WSOUNDAMMO89_TUBE=2, WSOUNDAMMO89_BELT_BOX=3, WSOUNDAMMO89_LOOSE_SHELLS=4 } wsoundammo89_type;
typedef struct wsoundammo89_context_s { wsound89_u32 state,accum; wsound89_i32 env,impulse,res1,res2,low; wsound89_u16 fill_q15,motion_q15; wsound89_u8 active; wsoundammo89_type type; } wsoundammo89_context;
wsound89_result wsoundammo89_init(wsoundammo89_context*,wsound89_u32);
wsound89_result wsoundammo89_trigger(wsoundammo89_context*,wsoundammo89_type,wsound89_u16,wsound89_u16);
wsound89_i16 wsoundammo89_process_sample(wsoundammo89_context*);
#ifdef __cplusplus
}
#endif
#endif
