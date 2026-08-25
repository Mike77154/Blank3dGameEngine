#ifndef GVEH_WHEEL_H
#define GVEH_WHEEL_H

#include "gveh_math.h"
#include "gveh_surface.h"

typedef struct gveh_wheel_s {
    gveh_vec3 local_pos;
    gveh_fx radius;
    gveh_fx width;
    gveh_fx steer_max;
    gveh_fx drive_bias;
    gveh_fx brake_bias;
    gveh_fx spin;
    gveh_fx spin_vel;
    gveh_i32 steer_angle;
    gveh_i32 grounded;
    gveh_fx compression;
    gveh_fx normal_load;
    gveh_u8 surface_id;
} gveh_wheel;

void gveh_wheel_clear(gveh_wheel *w);

#endif
