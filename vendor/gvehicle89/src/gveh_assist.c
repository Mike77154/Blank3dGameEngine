#include "gveh_assist.h"

void gveh_assist_none(gveh_assist *a)
{
    a->abs_strength = 0;
    a->tcs_strength = 0;
    a->yaw_stability = 0;
    a->countersteer = 0;
    a->low_speed_steer = 0;
    a->air_control = 0;
    a->flip_assist = 0;
}

void gveh_assist_arcade(gveh_assist *a)
{
    a->abs_strength = GVEH_FX_ONE / 2;
    a->tcs_strength = GVEH_FX_ONE / 3;
    a->yaw_stability = GVEH_FX_ONE;
    a->countersteer = GVEH_FX_ONE / 2;
    a->low_speed_steer = GVEH_FX_ONE;
    a->air_control = GVEH_FX_ONE / 3;
    a->flip_assist = GVEH_FX_ONE / 2;
}

void gveh_assist_halo(gveh_assist *a)
{
    gveh_assist_arcade(a);
    a->air_control = GVEH_FX_ONE;
    a->flip_assist = GVEH_FX_ONE;
    a->low_speed_steer = GVEH_FX_ONE;
}

void gveh_assist_gt(gveh_assist *a)
{
    a->abs_strength = GVEH_FX_ONE / 2;
    a->tcs_strength = GVEH_FX_ONE / 4;
    a->yaw_stability = GVEH_FX_ONE / 5;
    a->countersteer = GVEH_FX_ONE / 5;
    a->low_speed_steer = GVEH_FX_ONE / 4;
    a->air_control = 0;
    a->flip_assist = 0;
}
