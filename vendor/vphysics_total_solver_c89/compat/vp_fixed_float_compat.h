#ifndef VP_FIXED_FLOAT_COMPAT_H
#define VP_FIXED_FLOAT_COMPAT_H

/* Optional legacy host shim. Never included by the strict fixed-point core. */
#include "../include/vp_fixed.h"

static vp_fx vp_fx_from_float(float f)
{
    return (vp_fx)(f * (float)VP_FX_ONE);
}

static float vp_fx_to_float(vp_fx x)
{
    return ((float)x) / (float)VP_FX_ONE;
}

#endif
