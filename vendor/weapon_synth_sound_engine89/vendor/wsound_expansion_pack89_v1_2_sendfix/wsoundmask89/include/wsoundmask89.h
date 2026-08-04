#ifndef WSOUNDMASK89_H
#define WSOUNDMASK89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum wsoundmask89_quality_e { WSOUNDMASK89_VIRTUAL=0, WSOUNDMASK89_REDUCED=1, WSOUNDMASK89_FULL=2 } wsoundmask89_quality;
typedef struct wsoundmask89_context_s { wsound89_i32 mask_env,detail_gain_q15; wsound89_u16 threshold_q15; } wsoundmask89_context;
wsound89_result wsoundmask89_init(wsoundmask89_context*);
void wsoundmask89_trigger(wsoundmask89_context*,wsound89_u16);
wsoundmask89_quality wsoundmask89_decide(const wsoundmask89_context*,wsound89_u16,wsound89_u16);
wsound89_i16 wsoundmask89_mix_sample(wsoundmask89_context*,wsound89_i16,wsound89_i16);
#ifdef __cplusplus
}
#endif
#endif
