/*
    cameranaku89_profiles.c
    CC0 1.0 Universal.
    Futureable profile style bank. No heap. No floats. No doubles.
*/
#include "cameranaku89_profiles.h"

static void cnk_profile_recipe_fps(cnk_profile_recipe *r)
{
    r->style = CNK_STYLE_FPS;
    r->mode = CNK_MODE_FPS;
    r->fov_y_deg = CNK_DEG(70);
    r->ortho_height = CNK_FX_FROM_INT(10);
    r->pivot_offset = cnk_vec3_make(0, CNK_FX_FRAC(17, 10), 0);
    r->camera_offset = cnk_vec3_make(0, 0, 0);
    r->look_offset = cnk_vec3_make(0, 0, 0);
    r->distance = 0;
    r->min_distance = 0;
    r->max_distance = 0;
    r->height = 0;
    r->shoulder_x = 0;
    r->pos_lag = CNK_ONE;
    r->rot_lag = CNK_ONE;
    r->fov_lag = CNK_ONE;
    r->min_pitch_deg = -CNK_DEG(85);
    r->max_pitch_deg = CNK_DEG(85);
    r->collision_radius = 0;
    r->flags = CNK_FLAG_LOCK_ROLL | CNK_FLAG_USE_TARGET_YAW | CNK_FLAG_CLAMP_PITCH;
}

static void cnk_profile_recipe_follow(cnk_profile_recipe *r)
{
    r->style = CNK_STYLE_THIRD_PERSON;
    r->mode = CNK_MODE_FOLLOW;
    r->fov_y_deg = CNK_DEG(60);
    r->ortho_height = CNK_FX_FROM_INT(10);
    r->pivot_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(2), 0);
    r->camera_offset = cnk_vec3_make(0, 0, 0);
    r->look_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(1), 0);
    r->distance = CNK_FX_FROM_INT(10);
    r->min_distance = CNK_FX_FROM_INT(2);
    r->max_distance = CNK_FX_FROM_INT(64);
    r->height = CNK_FX_FROM_INT(3);
    r->shoulder_x = 0;
    r->pos_lag = CNK_FX_FRAC(2, 10);
    r->rot_lag = CNK_FX_FRAC(3, 10);
    r->fov_lag = CNK_FX_FRAC(3, 10);
    r->min_pitch_deg = -CNK_DEG(75);
    r->max_pitch_deg = CNK_DEG(75);
    r->collision_radius = CNK_FX_FRAC(35, 100);
    r->flags = CNK_FLAG_LOCK_ROLL | CNK_FLAG_USE_TARGET_YAW | CNK_FLAG_KEEP_LINE_OF_SIGHT | CNK_FLAG_CLAMP_PITCH;
}

static void cnk_profile_recipe_ots(cnk_profile_recipe *r)
{
    cnk_profile_recipe_follow(r);
    r->style = CNK_STYLE_OTS;
    r->mode = CNK_MODE_FOLLOW;
    r->fov_y_deg = CNK_DEG(55);
    r->pivot_offset = cnk_vec3_make(0, CNK_FX_FRAC(16, 10), 0);
    r->look_offset = cnk_vec3_make(0, CNK_FX_FRAC(12, 10), CNK_FX_FROM_INT(2));
    r->distance = CNK_FX_FROM_INT(4);
    r->min_distance = CNK_FX_FROM_INT(1);
    r->max_distance = CNK_FX_FROM_INT(16);
    r->height = CNK_FX_FROM_INT(1);
    r->shoulder_x = CNK_FX_FRAC(8, 10);
    r->pos_lag = CNK_FX_FRAC(35, 100);
    r->rot_lag = CNK_FX_FRAC(45, 100);
}

static void cnk_profile_recipe_orbit(cnk_profile_recipe *r)
{
    cnk_profile_recipe_follow(r);
    r->style = CNK_STYLE_ORBIT;
    r->mode = CNK_MODE_ORBIT;
    r->distance = CNK_FX_FROM_INT(12);
    r->min_distance = CNK_FX_FROM_INT(3);
    r->max_distance = CNK_FX_FROM_INT(40);
}

static void cnk_profile_recipe_cutscene(cnk_profile_recipe *r)
{
    cnk_profile_recipe_follow(r);
    r->style = CNK_STYLE_CUTSCENE;
    r->mode = CNK_MODE_CUTSCENE;
    r->pos_lag = CNK_ONE;
    r->rot_lag = CNK_ONE;
    r->fov_lag = CNK_ONE;
    r->flags = CNK_FLAG_LOCK_ROLL;
}

static void cnk_profile_recipe_rail(cnk_profile_recipe *r)
{
    cnk_profile_recipe_follow(r);
    r->style = CNK_STYLE_RAIL;
    r->mode = CNK_MODE_RAIL;
    r->fov_y_deg = CNK_DEG(50);
    r->pos_lag = CNK_FX_FRAC(4, 10);
    r->rot_lag = CNK_FX_FRAC(4, 10);
}

CNK_API void cnk_profile_from_recipe(cnk_profile *profile, const cnk_profile_recipe *recipe)
{
    int w;
    int h;
    cnk_fx near_clip;
    cnk_fx far_clip;
    cnk_lens lens;
    if (profile == 0 || recipe == 0) {
        return;
    }
    w = CNK_DEFAULT_VIEW_W;
    h = CNK_DEFAULT_VIEW_H;
    near_clip = CNK_FX_FROM_INT(CNK_DEFAULT_NEAR_CLIP);
    far_clip = CNK_FX_FROM_INT(CNK_DEFAULT_FAR_CLIP);
    if (profile->lens.viewport_w > 0) {
        w = profile->lens.viewport_w;
    }
    if (profile->lens.viewport_h > 0) {
        h = profile->lens.viewport_h;
    }
    if (profile->lens.near_clip > 0) {
        near_clip = profile->lens.near_clip;
    }
    if (profile->lens.far_clip > near_clip) {
        far_clip = profile->lens.far_clip;
    }
    cnk_profile_default(profile);
    lens = profile->lens;
    cnk_lens_perspective(&lens, w, h, recipe->fov_y_deg, near_clip, far_clip);
    lens.ortho_height = recipe->ortho_height;
    cnk_lens_validate(&lens);
    profile->mode = recipe->mode;
    profile->style = recipe->style;
    profile->lens = lens;
    profile->pivot_offset = recipe->pivot_offset;
    profile->camera_offset = recipe->camera_offset;
    profile->look_offset = recipe->look_offset;
    profile->distance = recipe->distance;
    profile->min_distance = recipe->min_distance;
    profile->max_distance = recipe->max_distance;
    profile->height = recipe->height;
    profile->shoulder_x = recipe->shoulder_x;
    profile->pos_lag = recipe->pos_lag;
    profile->rot_lag = recipe->rot_lag;
    profile->fov_lag = recipe->fov_lag;
    profile->min_pitch_deg = recipe->min_pitch_deg;
    profile->max_pitch_deg = recipe->max_pitch_deg;
    profile->collision_radius = recipe->collision_radius;
    profile->flags = recipe->flags;
    cnk_profile_validate(profile);
}

CNK_API int cnk_profile_apply_style(cnk_profile *profile, int style)
{
    cnk_profile_recipe recipe;
    if (profile == 0) {
        return CNK_FALSE;
    }
    if (style == CNK_STYLE_FPS) {
        cnk_profile_recipe_fps(&recipe);
    } else if (style == CNK_STYLE_OTS) {
        cnk_profile_recipe_ots(&recipe);
    } else if (style == CNK_STYLE_THIRD_PERSON) {
        cnk_profile_recipe_follow(&recipe);
    } else if (style == CNK_STYLE_ORBIT) {
        cnk_profile_recipe_orbit(&recipe);
    } else if (style == CNK_STYLE_RAIL) {
        cnk_profile_recipe_rail(&recipe);
    } else if (style == CNK_STYLE_CUTSCENE) {
        cnk_profile_recipe_cutscene(&recipe);
    } else {
        return CNK_FALSE;
    }
    cnk_profile_from_recipe(profile, &recipe);
    return CNK_TRUE;
}

CNK_API void cnk_profile_style_fps(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_FPS);
}

CNK_API void cnk_profile_style_ots(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_OTS);
}

CNK_API void cnk_profile_style_follow(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_THIRD_PERSON);
}

CNK_API void cnk_profile_style_fixed(cnk_profile *profile, cnk_pose pose, cnk_vec3 look_at)
{
    if (profile == 0) {
        return;
    }
    cnk_profile_default(profile);
    profile->mode = CNK_MODE_FIXED;
    profile->style = CNK_STYLE_FIXED;
    profile->lens.fov_y_deg = CNK_DEG(45);
    profile->pos_lag = CNK_ONE;
    profile->rot_lag = CNK_ONE;
    profile->fov_lag = CNK_ONE;
    profile->camera_offset = pose.pos;
    profile->look_offset = look_at;
    profile->flags = CNK_FLAG_LOCK_ROLL;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_style_orbit(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_ORBIT);
}

CNK_API void cnk_profile_style_cutscene(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_CUTSCENE);
}

CNK_API void cnk_profile_style_rail(cnk_profile *profile)
{
    cnk_profile_apply_style(profile, CNK_STYLE_RAIL);
}
