#ifndef GVEH_WATERCRAFT_H
#define GVEH_WATERCRAFT_H

#include "gveh_math.h"

typedef struct gveh_buoy_s {
    gveh_vec3 local_pos;
    gveh_fx radius;
    gveh_fx buoyancy_k;
    gveh_fx water_drag;
} gveh_buoy;

void gveh_buoy_clear(gveh_buoy *b);

#endif
