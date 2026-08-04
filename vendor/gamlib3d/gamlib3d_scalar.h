/* gamlib3d_scalar.h - Alias escalar del core fixed */
#ifndef GAMLIB3D_SCALAR_H
#define GAMLIB3D_SCALAR_H

#include "math_helpers/gamlib3d_math.h"

typedef g3d_fix g3d_scalar;

/* Helpers opcionales de borde-API; el core sigue siendo fixed puro. */
#define g3d_scalar_from_float(v) ((g3d_scalar)((v) * (float)G3D_FIX_ONE))
#define g3d_scalar_to_float(v)   ((float)(v) / (float)G3D_FIX_ONE)

#endif /* GAMLIB3D_SCALAR_H */
