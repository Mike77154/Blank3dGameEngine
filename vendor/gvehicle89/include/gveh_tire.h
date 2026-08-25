#ifndef GVEH_TIRE_H
#define GVEH_TIRE_H

#include "gveh_types.h"

typedef struct gveh_tire_curve_s {
    gveh_i16 slip_x[GVEH_TIRE_LUT];
    gveh_i16 force_x[GVEH_TIRE_LUT];
    gveh_i16 slip_y[GVEH_TIRE_LUT];
    gveh_i16 force_y[GVEH_TIRE_LUT];
    gveh_i16 align_y[GVEH_TIRE_LUT];
} gveh_tire_curve;

void gveh_tire_curve_arcade(gveh_tire_curve *c);
void gveh_tire_curve_sport(gveh_tire_curve *c);
void gveh_tire_curve_rally(gveh_tire_curve *c);
gveh_fx gveh_tire_lut(const gveh_i16 xs[GVEH_TIRE_LUT], const gveh_i16 ys[GVEH_TIRE_LUT], gveh_fx x);

#endif
