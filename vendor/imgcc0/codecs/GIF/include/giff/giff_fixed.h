#ifndef GIFF_FIXED_H
#define GIFF_FIXED_H

#include "giff_types.h"

typedef giff_s32 giff_fx16_16;

#define GIFF_FX_ONE            65536L
#define GIFF_FX_HALF           32768L
#define GIFF_FX_FROM_INT(v)    ((giff_fx16_16)((v) * GIFF_FX_ONE))
#define GIFF_FX_TO_INT(v)      ((int)((v) / GIFF_FX_ONE))
#define GIFF_FX_FROM_FRAC(n,d) ((giff_fx16_16)(((giff_s32)(n) * GIFF_FX_ONE) / (giff_s32)(d)))

#define GIFF_FX_W_R GIFF_FX_FROM_FRAC(299, 1000)
#define GIFF_FX_W_G GIFF_FX_FROM_FRAC(587, 1000)
#define GIFF_FX_W_B GIFF_FX_FROM_FRAC(114, 1000)

#endif
