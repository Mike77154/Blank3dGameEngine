/*
    cameranaku89_spring_arm.h
    CC0 1.0 Universal.
    Fixed-point spring arm helper for follow/orbit/OTS-style cameras.
*/
#ifndef CAMERANAKU89_SPRING_ARM_H
#define CAMERANAKU89_SPRING_ARM_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_spring_arm_s {
    cnk_fx target_length;
    cnk_fx current_length;
    cnk_fx min_length;
    cnk_fx max_length;
    cnk_fx probe_radius;
    cnk_fx return_lag;
    cnk_fx collision_lag;
    cnk_fx max_lag_distance;
    int collision_enabled;
} cnk_spring_arm;

CNK_API void cnk_spring_arm_default(cnk_spring_arm *arm);
CNK_API void cnk_spring_arm_set_limits(cnk_spring_arm *arm, cnk_fx min_length, cnk_fx max_length);
CNK_API void cnk_spring_arm_set_lag(cnk_spring_arm *arm, cnk_fx return_lag, cnk_fx collision_lag);
CNK_API int cnk_spring_arm_apply_provider(cnk_spring_arm *arm, cnk_state *state, cnk_vec3 pivot, cnk_world_probe_fn probe, void *probe_user, const cnk_transform_provider *provider);
CNK_API int cnk_spring_arm_apply(cnk_spring_arm *arm, cnk_state *state, cnk_vec3 pivot, cnk_world_probe_fn probe, void *probe_user);

#ifdef __cplusplus
}
#endif

#endif
