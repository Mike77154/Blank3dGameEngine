#include "gveh_masspoint.h"

void gveh_masspoint_clear(gveh_masspoint *m)
{
    m->local_pos = gveh_v3(0,0,0);
    m->radius = GVEH_FX_ONE / 2;
    m->mass_share = GVEH_FX_ONE;
    m->friction_ground = GVEH_FX_ONE;
    m->friction_wall = GVEH_FX_ONE;
    m->bounce = GVEH_FX_ONE / 5;
    m->powered_impulse = 0;
    m->flags = 0;
}
