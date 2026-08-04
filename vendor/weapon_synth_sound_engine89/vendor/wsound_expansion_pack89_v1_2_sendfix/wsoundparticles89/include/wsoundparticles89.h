#ifndef WSOUNDPARTICLES89_H
#define WSOUNDPARTICLES89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundparticles89_material_e { WSOUNDPARTICLES89_METAL=0, WSOUNDPARTICLES89_GLASS=1, WSOUNDPARTICLES89_GRAVEL=2, WSOUNDPARTICLES89_WOOD=3, WSOUNDPARTICLES89_EMBERS=4 } wsoundparticles89_material;
typedef struct wsoundparticles89_context_s { wsound89_u32 state,accum; wsound89_i32 env,impulse,low,res1,res2; wsound89_u16 density_q15,energy_q15; wsound89_u8 active; wsoundparticles89_material material; } wsoundparticles89_context;
wsound89_result wsoundparticles89_init(wsoundparticles89_context*,wsound89_u32);
wsound89_result wsoundparticles89_trigger(wsoundparticles89_context*,wsoundparticles89_material,wsound89_u16,wsound89_u16);
wsound89_i16 wsoundparticles89_process_sample(wsoundparticles89_context*);
int wsoundparticles89_is_active(const wsoundparticles89_context*);
#ifdef __cplusplus
}
#endif
#endif
