#include "gveh_driver_profile.h"

void gveh_driver_profile_clear(gveh_driver_profile *p)
{
    p->brake_bias = 0;
    p->apex_bias = 0;
    p->aggression = 0;
    p->smoothness = 0;
    p->mistake_rate = 0;
    p->rubber_band = 0;
}

void gveh_driver_profile_arcade(gveh_driver_profile *p)
{
    gveh_driver_profile_clear(p);
    p->aggression = 20;
    p->smoothness = 40;
    p->rubber_band = 20;
}

void gveh_driver_profile_rival(gveh_driver_profile *p)
{
    gveh_driver_profile_clear(p);
    p->brake_bias = 20;
    p->apex_bias = 30;
    p->aggression = 70;
    p->smoothness = 25;
    p->mistake_rate = 10;
}

void gveh_driver_profile_apply(const gveh_driver_profile *p, const gveh_input *in, gveh_input *out_input, gveh_i32 tick)
{
    gveh_i32 noise;
    gveh_fx n;
    *out_input = *in;
    if (p->mistake_rate != 0) {
        noise = (tick * 73 + tick / 3) & 255;
        noise -= 128;
        n = (gveh_fx)(noise * p->mistake_rate) / 64;
        out_input->steer = gveh_fx_clamp(out_input->steer + n, -GVEH_FX_ONE, GVEH_FX_ONE);
    }
    if (p->brake_bias > 0 && out_input->brake > 0) {
        out_input->brake += (out_input->brake * p->brake_bias) / 200;
        if (out_input->brake > GVEH_FX_ONE) out_input->brake = GVEH_FX_ONE;
    }
    if (p->aggression > 50 && out_input->throttle > 0) {
        out_input->throttle += (GVEH_FX_ONE - out_input->throttle) / 8;
    }
}
