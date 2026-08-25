#ifndef GVEH_ASSIST_H
#define GVEH_ASSIST_H

#include "gveh_types.h"

typedef struct gveh_assist_s {
    gveh_fx abs_strength;
    gveh_fx tcs_strength;
    gveh_fx yaw_stability;
    gveh_fx countersteer;
    gveh_fx low_speed_steer;
    gveh_fx air_control;
    gveh_fx flip_assist;
} gveh_assist;

void gveh_assist_none(gveh_assist *a);
void gveh_assist_arcade(gveh_assist *a);
void gveh_assist_halo(gveh_assist *a);
void gveh_assist_gt(gveh_assist *a);

#endif
