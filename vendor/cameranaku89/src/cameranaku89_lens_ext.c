/* cameranaku89_lens_ext.c - CC0 1.0 Universal. */
#include "cameranaku89_lens_ext.h"

CNK_API void cnk_lens_set_fov_axis(cnk_lens *lens, int fov_axis)
{
    if (lens == 0) {
        return;
    }
    lens->fov_axis = fov_axis;
    cnk_lens_validate(lens);
}

CNK_API void cnk_lens_set_aspect_policy(cnk_lens *lens, int aspect_policy)
{
    if (lens == 0) {
        return;
    }
    lens->aspect_policy = aspect_policy;
    cnk_lens_validate(lens);
}

CNK_API void cnk_lens_set_frustum_offset(cnk_lens *lens, cnk_fx x, cnk_fx y)
{
    if (lens == 0) {
        return;
    }
    lens->projection = CNK_PROJ_FRUSTUM;
    lens->frustum_offset_x = x;
    lens->frustum_offset_y = y;
    cnk_lens_validate(lens);
}

CNK_API void cnk_lens_set_viewport_offset(cnk_lens *lens, cnk_fx x, cnk_fx y)
{
    if (lens == 0) {
        return;
    }
    lens->viewport_offset_x = x;
    lens->viewport_offset_y = y;
}

CNK_API void cnk_lens_set_masks(cnk_lens *lens, cnk_u32 cull_mask, cnk_u32 effect_mask)
{
    if (lens == 0) {
        return;
    }
    lens->cull_mask = cull_mask;
    lens->effect_mask = effect_mask;
    cnk_lens_validate(lens);
}

CNK_API void cnk_camera_set_lens_ext(cnk_camera *cam, const cnk_lens *lens)
{
    if (cam == 0 || lens == 0) {
        return;
    }
    cam->profile.lens = *lens;
    cam->state.lens = *lens;
    cnk_lens_validate(&cam->profile.lens);
    cnk_lens_validate(&cam->state.lens);
}

CNK_API int cnk_camera_layer_visible(const cnk_camera *cam, cnk_u32 layer_mask)
{
    if (cam == 0) {
        return CNK_FALSE;
    }
    return (cam->state.lens.cull_mask & layer_mask) != 0 ? CNK_TRUE : CNK_FALSE;
}
