#include "gveh_susp.h"

void gveh_susp_default(gveh_susp *s)
{
    s->rest_len = GVEH_FX_ONE;
    s->min_len = GVEH_FX_ONE / 5;
    s->max_len = (GVEH_FX_ONE * 3) / 2;
    s->spring_k = GVEH_FX_ONE * 12;
    s->damper_bump = GVEH_FX_ONE * 3;
    s->damper_rebound = GVEH_FX_ONE * 2;
    s->antiroll_k = GVEH_FX_ONE * 2;
    s->visual_travel = GVEH_FX_ONE;
}
