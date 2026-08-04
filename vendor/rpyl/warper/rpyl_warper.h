#ifndef RPYL_WARPER_H
#define RPYL_WARPER_H

#include "rpyl_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

rpyl_fx rpyl_warper_clamp01(rpyl_fx t);
rpyl_fx rpyl_warper_lerp(rpyl_fx a, rpyl_fx b, rpyl_fx t);
rpyl_fx rpyl_warper_linear(rpyl_fx a, rpyl_fx b, rpyl_fx t);
rpyl_fx rpyl_warper_ease_in_quad(rpyl_fx t);
rpyl_fx rpyl_warper_ease_out_quad(rpyl_fx t);
rpyl_fx rpyl_warper_ease_in_out_quad(rpyl_fx t);
rpyl_fx rpyl_warper_smoothstep(rpyl_fx t);
rpyl_fx rpyl_warper_apply(rpyl_fx a, rpyl_fx b, rpyl_fx t, const char* curve);

#ifdef __cplusplus
}
#endif

#endif
