/*
    cameranaku89_lens_ext.h
    CC0 1.0 Universal.
    Extra lens controls: fov axis, aspect policy, frustum/viewport offset and masks.
*/
#ifndef CAMERANAKU89_LENS_EXT_H
#define CAMERANAKU89_LENS_EXT_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

CNK_API void cnk_lens_set_fov_axis(cnk_lens *lens, int fov_axis);
CNK_API void cnk_lens_set_aspect_policy(cnk_lens *lens, int aspect_policy);
CNK_API void cnk_lens_set_frustum_offset(cnk_lens *lens, cnk_fx x, cnk_fx y);
CNK_API void cnk_lens_set_viewport_offset(cnk_lens *lens, cnk_fx x, cnk_fx y);
CNK_API void cnk_lens_set_masks(cnk_lens *lens, cnk_u32 cull_mask, cnk_u32 effect_mask);
CNK_API void cnk_camera_set_lens_ext(cnk_camera *cam, const cnk_lens *lens);
CNK_API int cnk_camera_layer_visible(const cnk_camera *cam, cnk_u32 layer_mask);

#ifdef __cplusplus
}
#endif

#endif
