#ifndef GVEH_SUSP_H
#define GVEH_SUSP_H

#include "gveh_types.h"

typedef struct gveh_susp_s {
    gveh_fx rest_len;
    gveh_fx min_len;
    gveh_fx max_len;
    gveh_fx spring_k;
    gveh_fx damper_bump;
    gveh_fx damper_rebound;
    gveh_fx antiroll_k;
    gveh_fx visual_travel;
} gveh_susp;

void gveh_susp_default(gveh_susp *s);

#endif
