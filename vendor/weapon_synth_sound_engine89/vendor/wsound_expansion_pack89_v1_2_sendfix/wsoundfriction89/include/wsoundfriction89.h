#ifndef WSOUNDFRICTION89_H
#define WSOUNDFRICTION89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundfriction89_material_e { WSOUNDFRICTION89_STEEL=0, WSOUNDFRICTION89_POLYMER=1, WSOUNDFRICTION89_WOOD=2, WSOUNDFRICTION89_CLOTH=3, WSOUNDFRICTION89_CONCRETE=4 } wsoundfriction89_material;
typedef struct wsoundfriction89_context_s { wsound89_u32 state; wsound89_i32 env,low,res1,res2; wsound89_u16 speed_q15,pressure_q15,roughness_q15; wsound89_u8 active,gate; wsoundfriction89_material material; } wsoundfriction89_context;
wsound89_result wsoundfriction89_init(wsoundfriction89_context*,wsound89_u32);
wsound89_result wsoundfriction89_start(wsoundfriction89_context*,wsoundfriction89_material,wsound89_u16,wsound89_u16,wsound89_u16);
void wsoundfriction89_set_motion(wsoundfriction89_context*,wsound89_u16,wsound89_u16);
void wsoundfriction89_set_gate(wsoundfriction89_context*,int);
wsound89_i16 wsoundfriction89_process_sample(wsoundfriction89_context*);
#ifdef __cplusplus
}
#endif
#endif
