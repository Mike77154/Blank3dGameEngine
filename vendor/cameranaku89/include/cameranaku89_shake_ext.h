/*
    cameranaku89_shake_ext.h
    CC0 1.0 Universal.
    Declarative shake helpers that call the core shake slots.
*/
#ifndef CAMERANAKU89_SHAKE_EXT_H
#define CAMERANAKU89_SHAKE_EXT_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CNK_SHAKE_KIND_IMPULSE = 0,
    CNK_SHAKE_KIND_TRAUMA = 1,
    CNK_SHAKE_KIND_RECOIL = 2,
    CNK_SHAKE_KIND_FOOTSTEP = 3,
    CNK_SHAKE_KIND_BREATHING = 4
};

typedef struct cnk_shake_request_s {
    int kind;
    int duration_ticks;
    cnk_fx amplitude;
    cnk_fx rot_amp_deg;
    cnk_fx fov_amp_deg;
    int frequency_hz;
    cnk_u32 seed;
    int play_space;
    cnk_fx user_yaw_deg;
} cnk_shake_request;

CNK_API void cnk_shake_request_default(cnk_shake_request *request, int kind);
CNK_API int cnk_camera_add_shake_request(cnk_camera *cam, const cnk_shake_request *request);

#ifdef __cplusplus
}
#endif

#endif
