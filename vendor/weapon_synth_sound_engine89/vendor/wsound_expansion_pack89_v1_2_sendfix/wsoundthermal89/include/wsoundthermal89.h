#ifndef WSOUNDTHERMAL89_H
#define WSOUNDTHERMAL89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundthermal89_material_e { WSOUNDTHERMAL89_BARREL=0, WSOUNDTHERMAL89_SUPPRESSOR=1, WSOUNDTHERMAL89_POLYMER=2 } wsoundthermal89_material;
typedef struct wsoundthermal89_context_s { wsound89_u32 state,accum; wsound89_i32 heat_q15,click,res1,res2; wsound89_u16 cooling_q15; wsound89_u8 active; wsoundthermal89_material material; } wsoundthermal89_context;
wsound89_result wsoundthermal89_init(wsoundthermal89_context*,wsound89_u32);
void wsoundthermal89_add_heat(wsoundthermal89_context*,wsoundthermal89_material,wsound89_u16);
wsound89_i16 wsoundthermal89_process_sample(wsoundthermal89_context*);
#ifdef __cplusplus
}
#endif
#endif
