#include "vehiclephysics89.h"
#include <stdio.h>

int main(void)
{
    gveh_body body;
    gveh_fx dt;
    gveh_body_clear(&body);
    gveh_body_set_mass(&body, gveh_fx_from_int(100));
    dt = gveh_fx_div(GVEH_FX_ONE, gveh_fx_from_int(60));
    vehiclephysics89_apply_gravity(&body, gveh_fx_from_int(100), gveh_fx_from_int(10));
    vehiclephysics89_integrate(&body, dt);
    if (body.vel.y >= 0) return 1;
    printf("vehiclephysics89: OK\n");
    return 0;
}
