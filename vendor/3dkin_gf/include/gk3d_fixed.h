#ifndef GK3D_FIXED_H
#define GK3D_FIXED_H

#include "gk3d_config.h"

typedef long gk3d_i32;
typedef unsigned long gk3d_u32;
typedef gk3d_i32 gk3d_fix;

#define GK3D_TRUE 1
#define GK3D_FALSE 0

#define GK3D_ONE ((gk3d_fix)1L << GK3D_FIX_SHIFT)
#define GK3D_HALF (GK3D_ONE / 2L)
#define GK3D_FROM_INT(n) ((gk3d_fix)(n) * GK3D_ONE)
#define GK3D_TO_INT(x) ((int)((x) / GK3D_ONE))
#define GK3D_ABS(x) (((x) < 0) ? -(x) : (x))
#define GK3D_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define GK3D_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define GK3D_CLAMP(v,lo,hi) (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))

#ifdef __cplusplus
extern "C" {
#endif

gk3d_fix gk3d_fix_mul(gk3d_fix a, gk3d_fix b);
gk3d_fix gk3d_fix_div(gk3d_fix a, gk3d_fix b);

#ifdef __cplusplus
}
#endif

#endif
