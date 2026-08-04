#ifndef B3D_FIXED_H
#define B3D_FIXED_H

#include "bolt3d/b3d_config.h"
#include "bolt3d/b3d_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_FIXED_ONE ((B3D_Fixed)65536L)
#define B3D_FIXED_HALF ((B3D_Fixed)32768L)
#define B3D_FIXED_ZERO ((B3D_Fixed)0L)
#define B3D_FIXED_MAX ((B3D_Fixed)2147483647L)
#define B3D_FIXED_MIN ((B3D_Fixed)(-2147483647L - 1L))
#define B3D_FIXED_EPSILON ((B3D_Fixed)1L)

B3D_API B3D_Fixed b3d_fixed_from_int(long value);
B3D_API long b3d_fixed_to_int(B3D_Fixed value);
B3D_API B3D_Fixed b3d_fixed_abs(B3D_Fixed value);
B3D_API B3D_Fixed b3d_fixed_neg(B3D_Fixed value);
B3D_API B3D_Fixed b3d_fixed_add_sat(B3D_Fixed a, B3D_Fixed b);
B3D_API B3D_Fixed b3d_fixed_sub_sat(B3D_Fixed a, B3D_Fixed b);
B3D_API B3D_Fixed b3d_fixed_mul(B3D_Fixed a, B3D_Fixed b);
B3D_API B3D_Fixed b3d_fixed_div(B3D_Fixed a, B3D_Fixed b);
B3D_API B3D_Fixed b3d_fixed_sqrt(B3D_Fixed value);
B3D_API B3D_Fixed b3d_fixed_clamp(B3D_Fixed value, B3D_Fixed min_value, B3D_Fixed max_value);
B3D_API B3D_Fixed b3d_fixed_lerp(B3D_Fixed a, B3D_Fixed b, B3D_Fixed t);
B3D_API B3D_Fixed b3d_fixed_mul_int(B3D_Fixed value, long scale);

#ifdef __cplusplus
}
#endif

#endif
