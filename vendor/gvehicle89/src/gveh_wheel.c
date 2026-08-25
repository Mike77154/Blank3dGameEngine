#include "gveh_wheel.h"

void gveh_wheel_clear(gveh_wheel *w)
{
    w->local_pos = gveh_v3(0,0,0);
    w->radius = GVEH_FX_ONE;
    w->width = GVEH_FX_ONE / 3;
    w->steer_max = 0;
    w->drive_bias = 0;
    w->brake_bias = 0;
    w->spin = 0;
    w->spin_vel = 0;
    w->steer_angle = 0;
    w->grounded = GVEH_FALSE;
    w->compression = 0;
    w->normal_load = 0;
    w->surface_id = GVEH_SURF_ASPHALT_DRY;
}
