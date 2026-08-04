/*
    cameranaku89_profiles.h
    CC0 1.0 Universal.

    Optional futureable profile style bank for Cameranaku89.
    Keep this module replaceable: the solver only needs cnk_profile.
*/
#ifndef CAMERANAKU89_PROFILES_H
#define CAMERANAKU89_PROFILES_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_profile_recipe_s {
    int style;
    int mode;
    cnk_fx fov_y_deg;
    cnk_fx ortho_height;
    cnk_vec3 pivot_offset;
    cnk_vec3 camera_offset;
    cnk_vec3 look_offset;
    cnk_fx distance;
    cnk_fx min_distance;
    cnk_fx max_distance;
    cnk_fx height;
    cnk_fx shoulder_x;
    cnk_fx pos_lag;
    cnk_fx rot_lag;
    cnk_fx fov_lag;
    cnk_fx min_pitch_deg;
    cnk_fx max_pitch_deg;
    cnk_fx collision_radius;
    int flags;
} cnk_profile_recipe;

CNK_API void cnk_profile_from_recipe(cnk_profile *profile, const cnk_profile_recipe *recipe);
CNK_API int cnk_profile_apply_style(cnk_profile *profile, int style);

CNK_API void cnk_profile_style_fps(cnk_profile *profile);
CNK_API void cnk_profile_style_ots(cnk_profile *profile);
CNK_API void cnk_profile_style_follow(cnk_profile *profile);
CNK_API void cnk_profile_style_fixed(cnk_profile *profile, cnk_pose pose, cnk_vec3 look_at);
CNK_API void cnk_profile_style_orbit(cnk_profile *profile);
CNK_API void cnk_profile_style_cutscene(cnk_profile *profile);
CNK_API void cnk_profile_style_rail(cnk_profile *profile);

#ifdef __cplusplus
}
#endif

#endif
