#include "gveh_watercraft.h"

void gveh_buoy_clear(gveh_buoy *b)
{
    b->local_pos = gveh_v3(0,0,0);
    b->radius = GVEH_FX_ONE;
    b->buoyancy_k = GVEH_FX_ONE * 8;
    b->water_drag = GVEH_FX_ONE * 2;
}
