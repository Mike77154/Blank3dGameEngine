#ifndef WSOUNDDOPPLER89_H
#define WSOUNDDOPPLER89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wsounddoppler89_context_s { wsound89_u32 phase_q16,ratio_q16; } wsounddoppler89_context;
wsound89_result wsounddoppler89_init(wsounddoppler89_context*);
wsound89_result wsounddoppler89_set_motion(wsounddoppler89_context*,wsound89_i32,wsound89_i32,wsound89_u32);
void wsounddoppler89_set_ratio(wsounddoppler89_context*,wsound89_u32);
wsound89_i16 wsounddoppler89_process_buffer(wsounddoppler89_context*,const wsound89_i16*,wsound89_u32,int*);
#ifdef __cplusplus
}
#endif
#endif
