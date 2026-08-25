#ifndef GVEH_DRIVER_PROFILE_H
#define GVEH_DRIVER_PROFILE_H

#include "gveh_ai.h"

typedef struct gveh_driver_profile_s {
    gveh_i16 brake_bias;
    gveh_i16 apex_bias;
    gveh_i16 aggression;
    gveh_i16 smoothness;
    gveh_i16 mistake_rate;
    gveh_i16 rubber_band;
} gveh_driver_profile;

void gveh_driver_profile_clear(gveh_driver_profile *p);
void gveh_driver_profile_arcade(gveh_driver_profile *p);
void gveh_driver_profile_rival(gveh_driver_profile *p);
void gveh_driver_profile_apply(const gveh_driver_profile *p, const gveh_input *in, gveh_input *out_input, gveh_i32 tick);

#endif
