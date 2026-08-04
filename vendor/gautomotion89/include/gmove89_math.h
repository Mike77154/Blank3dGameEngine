#ifndef GMOVE89_MATH_H
#define GMOVE89_MATH_H

#include "gmove89_types.h"

#ifdef __cplusplus
extern "C" {
#endif

GMoveFx89 gmove89_fx_clamp(GMoveFx89 value, GMoveFx89 low, GMoveFx89 high);
GMoveFx89 gmove89_fx_abs(GMoveFx89 value);
GMoveFx89 gmove89_fx_add(GMoveFx89 a, GMoveFx89 b);
GMoveFx89 gmove89_fx_sub(GMoveFx89 a, GMoveFx89 b);
GMoveFx89 gmove89_fx_mul(GMoveFx89 a, GMoveFx89 b);
GMoveFx89 gmove89_fx_div(GMoveFx89 a, GMoveFx89 b);

GMoveVec3_89 gmove89_vec3(GMoveFx89 x, GMoveFx89 y, GMoveFx89 z);
GMoveVec3_89 gmove89_vec3_zero(void);
GMoveVec3_89 gmove89_vec3_add(GMoveVec3_89 a, GMoveVec3_89 b);
GMoveVec3_89 gmove89_vec3_sub(GMoveVec3_89 a, GMoveVec3_89 b);
GMoveVec3_89 gmove89_vec3_scale(GMoveVec3_89 value, GMoveFx89 scalar);
GMoveFx89 gmove89_vec3_dot(GMoveVec3_89 a, GMoveVec3_89 b);
GMoveVec3_89 gmove89_vec3_cross(GMoveVec3_89 a, GMoveVec3_89 b);
GMoveFx89 gmove89_vec3_length_approx(GMoveVec3_89 value);
GMoveVec3_89 gmove89_vec3_normalize_approx(GMoveVec3_89 value);
int gmove89_vec3_is_near_zero(GMoveVec3_89 value, GMoveFx89 epsilon);

GMoveFx89 gmove89_sin_turn(GMoveFx89 phase);
GMoveFx89 gmove89_cos_turn(GMoveFx89 phase);
GMoveFx89 gmove89_triangle_turn(GMoveFx89 phase);
GMoveFx89 gmove89_wrap_turn(GMoveFx89 phase);

#ifdef __cplusplus
}
#endif

#endif
