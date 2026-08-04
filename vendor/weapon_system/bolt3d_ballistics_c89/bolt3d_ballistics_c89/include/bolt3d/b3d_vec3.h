#ifndef B3D_VEC3_H
#define B3D_VEC3_H

#include "bolt3d/b3d_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct B3D_Vec3 {
    B3D_Fixed x;
    B3D_Fixed y;
    B3D_Fixed z;
} B3D_Vec3;

B3D_API B3D_Vec3 b3d_vec3(B3D_Fixed x, B3D_Fixed y, B3D_Fixed z);
B3D_API B3D_Vec3 b3d_vec3_zero(void);
B3D_API B3D_Vec3 b3d_vec3_add(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Vec3 b3d_vec3_sub(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Vec3 b3d_vec3_scale(B3D_Vec3 v, B3D_Fixed scale);
B3D_API B3D_Fixed b3d_vec3_dot(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Vec3 b3d_vec3_cross(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Vec3 b3d_vec3_mul_components(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Vec3 b3d_vec3_div_components(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Fixed b3d_vec3_length_sq(B3D_Vec3 v);
B3D_API B3D_Fixed b3d_vec3_length(B3D_Vec3 v);
B3D_API B3D_Vec3 b3d_vec3_normalize(B3D_Vec3 v);
B3D_API B3D_Vec3 b3d_vec3_lerp(B3D_Vec3 a, B3D_Vec3 b, B3D_Fixed t);
B3D_API B3D_Vec3 b3d_vec3_reflect(B3D_Vec3 v, B3D_Vec3 normal);
B3D_API B3D_Vec3 b3d_vec3_project(B3D_Vec3 v, B3D_Vec3 normal);
B3D_API B3D_Vec3 b3d_vec3_reject(B3D_Vec3 v, B3D_Vec3 normal);
B3D_API B3D_Fixed b3d_vec3_distance_sq(B3D_Vec3 a, B3D_Vec3 b);
B3D_API B3D_Fixed b3d_vec3_distance(B3D_Vec3 a, B3D_Vec3 b);

#ifdef __cplusplus
}
#endif

#endif
