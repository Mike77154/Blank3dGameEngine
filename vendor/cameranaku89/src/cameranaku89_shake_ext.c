/* cameranaku89_shake_ext.c - CC0 1.0 Universal. */
#include "cameranaku89_shake_ext.h"

CNK_API void cnk_shake_request_default(cnk_shake_request *request, int kind)
{
    if (request == 0) {
        return;
    }
    request->kind = kind;
    request->duration_ticks = 8;
    request->amplitude = CNK_FX_FRAC(5, 100);
    request->rot_amp_deg = CNK_DEG(1);
    request->fov_amp_deg = 0;
    request->frequency_hz = 8;
    request->seed = 1UL;
    request->play_space = CNK_SHAKE_CAMERA_LOCAL;
    request->user_yaw_deg = 0;
    if (kind == CNK_SHAKE_KIND_TRAUMA) {
        request->duration_ticks = 24;
        request->amplitude = CNK_FX_FRAC(15, 100);
        request->rot_amp_deg = CNK_DEG(3);
        request->frequency_hz = 12;
    } else if (kind == CNK_SHAKE_KIND_RECOIL) {
        request->duration_ticks = 5;
        request->amplitude = CNK_FX_FRAC(3, 100);
        request->rot_amp_deg = CNK_DEG(2);
        request->fov_amp_deg = CNK_FX_FRAC(4, 10);
        request->frequency_hz = 18;
    } else if (kind == CNK_SHAKE_KIND_FOOTSTEP) {
        request->duration_ticks = 6;
        request->amplitude = CNK_FX_FRAC(2, 100);
        request->rot_amp_deg = CNK_FX_FRAC(2, 10);
        request->frequency_hz = 4;
    } else if (kind == CNK_SHAKE_KIND_BREATHING) {
        request->duration_ticks = 60;
        request->amplitude = CNK_FX_FRAC(1, 100);
        request->rot_amp_deg = CNK_FX_FRAC(1, 10);
        request->frequency_hz = 2;
    }
}

CNK_API int cnk_camera_add_shake_request(cnk_camera *cam, const cnk_shake_request *request)
{
    if (cam == 0 || request == 0) {
        return -1;
    }
    return cnk_camera_add_shake_ex(cam, request->duration_ticks, request->amplitude, request->rot_amp_deg, request->fov_amp_deg, request->frequency_hz, request->seed, request->play_space, request->user_yaw_deg);
}
