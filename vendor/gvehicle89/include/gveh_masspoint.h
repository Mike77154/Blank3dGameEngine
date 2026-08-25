#ifndef GVEH_MASSPOINT_H
#define GVEH_MASSPOINT_H

#include "gveh_math.h"

#define GVEH_MP_POWERED 1u
#define GVEH_MP_NOSE    2u
#define GVEH_MP_TAIL    4u

typedef struct gveh_masspoint_s {
    gveh_vec3 local_pos;
    gveh_fx radius;
    gveh_fx mass_share;
    gveh_fx friction_ground;
    gveh_fx friction_wall;
    gveh_fx bounce;
    gveh_fx powered_impulse;
    gveh_u16 flags;
} gveh_masspoint;

void gveh_masspoint_clear(gveh_masspoint *m);

#endif
