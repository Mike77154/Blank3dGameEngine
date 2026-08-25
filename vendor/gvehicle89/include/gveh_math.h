#ifndef GVEH_MATH_H
#define GVEH_MATH_H

#include "gveh_types.h"

typedef struct gveh_vec3_s {
    gveh_fx x;
    gveh_fx y;
    gveh_fx z;
} gveh_vec3;

typedef struct gveh_basis_s {
    gveh_vec3 fwd;
    gveh_vec3 right;
    gveh_vec3 up;
} gveh_basis;

gveh_fx gveh_fx_from_int(gveh_i32 v);
gveh_i32 gveh_fx_to_int(gveh_fx v);
gveh_fx gveh_fx_mul(gveh_fx a, gveh_fx b);
gveh_fx gveh_fx_div(gveh_fx a, gveh_fx b);
gveh_fx gveh_fx_abs(gveh_fx v);
gveh_fx gveh_fx_clamp(gveh_fx v, gveh_fx lo, gveh_fx hi);
gveh_fx gveh_fx_lerp(gveh_fx a, gveh_fx b, gveh_fx t);
gveh_fx gveh_fx_sin256(gveh_i32 angle256);
gveh_fx gveh_fx_cos256(gveh_i32 angle256);

gveh_vec3 gveh_v3(gveh_fx x, gveh_fx y, gveh_fx z);
gveh_vec3 gveh_v3_add(gveh_vec3 a, gveh_vec3 b);
gveh_vec3 gveh_v3_sub(gveh_vec3 a, gveh_vec3 b);
gveh_vec3 gveh_v3_scale(gveh_vec3 a, gveh_fx s);
gveh_fx   gveh_v3_dot(gveh_vec3 a, gveh_vec3 b);
gveh_vec3 gveh_v3_cross(gveh_vec3 a, gveh_vec3 b);
gveh_fx   gveh_v3_len_approx(gveh_vec3 a);
gveh_vec3 gveh_v3_norm_approx(gveh_vec3 a);

gveh_basis gveh_basis_yaw_pitch_roll(gveh_i32 yaw256, gveh_i32 pitch256, gveh_i32 roll256);
gveh_vec3  gveh_basis_local_to_world(gveh_basis b, gveh_vec3 local);

#endif
