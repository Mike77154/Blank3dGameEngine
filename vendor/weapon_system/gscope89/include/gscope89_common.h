/*
 * gscope89_common.h - shared ABI types for the split gscope89 modules.
 * C89, integer/fixed-point only, no heap ownership.
 */
#ifndef GSCOPE89_COMMON_H
#define GSCOPE89_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long g89_fx;

typedef struct g89_vec3 {
    g89_fx x;
    g89_fx y;
    g89_fx z;
} g89_vec3;

typedef struct g89_camera {
    g89_vec3 pos;
    g89_vec3 forward;
    g89_vec3 right;
    g89_vec3 up;
    short base_fov_deg_x100;
    short current_fov_deg_x100;
} g89_camera;

#define G89_FX_ONE ((g89_fx)65536L)
#define G89_FX_FROM_INT(v) ((g89_fx)((v) * 65536L))
#define G89_FX_TO_INT(v) ((int)((v) / 65536L))

#ifdef __cplusplus
}
#endif

#endif
