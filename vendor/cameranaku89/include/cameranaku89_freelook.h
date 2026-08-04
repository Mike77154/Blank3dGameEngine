/*
    cameranaku89_freelook.h
    CC0 1.0 Universal.
    Three-rig freelook helper: bottom/mid/top rigs interpolated by a fixed y axis.
*/
#ifndef CAMERANAKU89_FREELOOK_H
#define CAMERANAKU89_FREELOOK_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_freelook_rig_s {
    cnk_fx height;
    cnk_fx radius;
    cnk_fx fov_y_deg;
} cnk_freelook_rig;

typedef struct cnk_freelook_s {
    cnk_freelook_rig bottom;
    cnk_freelook_rig middle;
    cnk_freelook_rig top;
    cnk_fx x_axis_yaw_deg;
    cnk_fx y_axis; /* 0..1 fixed */
    cnk_vec3 pivot_offset;
    cnk_vec3 look_offset;
} cnk_freelook;

CNK_API void cnk_freelook_default(cnk_freelook *fl);
CNK_API void cnk_freelook_set_axis(cnk_freelook *fl, cnk_fx yaw_deg, cnk_fx y_axis);
CNK_API cnk_state cnk_freelook_solve_provider(const cnk_freelook *fl, const cnk_target *target, const cnk_lens *lens, const cnk_transform_provider *provider);
CNK_API cnk_state cnk_freelook_solve(const cnk_freelook *fl, const cnk_target *target, const cnk_lens *lens);
CNK_API void cnk_camera_apply_freelook(cnk_camera *cam, const cnk_freelook *fl);

#ifdef __cplusplus
}
#endif

#endif
