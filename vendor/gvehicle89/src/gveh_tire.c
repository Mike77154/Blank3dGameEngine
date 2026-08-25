#include "gveh_tire.h"
#include "gveh_math.h"

static void gveh_tire_fill(gveh_tire_curve *c, const gveh_i16 *fx, const gveh_i16 *fy)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_TIRE_LUT) {
        c->slip_x[i] = (gveh_i16)(i * 16);
        c->force_x[i] = fx[i];
        c->slip_y[i] = (gveh_i16)(i * 16);
        c->force_y[i] = fy[i];
        c->align_y[i] = (gveh_i16)(fy[i] / 3);
        i++;
    }
}

void gveh_tire_curve_arcade(gveh_tire_curve *c)
{
    static const gveh_i16 fx[GVEH_TIRE_LUT] = {0,70,110,125,132,136,138,139,140,140,140,139,138,136,134,132};
    static const gveh_i16 fy[GVEH_TIRE_LUT] = {0,55,92,115,130,138,142,143,142,140,138,136,134,132,130,128};
    gveh_tire_fill(c, fx, fy);
}

void gveh_tire_curve_sport(gveh_tire_curve *c)
{
    static const gveh_i16 fx[GVEH_TIRE_LUT] = {0,85,130,155,166,170,168,162,154,146,138,130,124,118,112,108};
    static const gveh_i16 fy[GVEH_TIRE_LUT] = {0,75,122,150,165,172,170,162,152,142,132,124,116,110,104,98};
    gveh_tire_fill(c, fx, fy);
}

void gveh_tire_curve_rally(gveh_tire_curve *c)
{
    static const gveh_i16 fx[GVEH_TIRE_LUT] = {0,60,92,110,120,126,130,130,128,126,123,120,117,114,112,110};
    static const gveh_i16 fy[GVEH_TIRE_LUT] = {0,45,75,96,112,122,130,134,134,132,130,126,122,118,114,110};
    gveh_tire_fill(c, fx, fy);
}

gveh_fx gveh_tire_lut(const gveh_i16 xs[GVEH_TIRE_LUT], const gveh_i16 ys[GVEH_TIRE_LUT], gveh_fx x)
{
    gveh_i32 i;
    gveh_fx xv;
    gveh_fx x0;
    gveh_fx x1;
    gveh_fx y0;
    gveh_fx y1;
    gveh_fx t;

    if (x < 0) x = -x;
    xv = (x * 100) / GVEH_FX_ONE;
    if (xv <= xs[0]) return gveh_fx_from_int(ys[0]) / 100;
    i = 0;
    while (i < GVEH_TIRE_LUT - 1) {
        if (xv <= xs[i + 1]) {
            x0 = gveh_fx_from_int(xs[i]);
            x1 = gveh_fx_from_int(xs[i + 1]);
            y0 = gveh_fx_from_int(ys[i]) / 100;
            y1 = gveh_fx_from_int(ys[i + 1]) / 100;
            t = gveh_fx_div(gveh_fx_from_int((gveh_i32)xv - xs[i]), x1 - x0);
            return gveh_fx_lerp(y0, y1, t);
        }
        i++;
    }
    return gveh_fx_from_int(ys[GVEH_TIRE_LUT - 1]) / 100;
}
