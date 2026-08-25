#include "gveh_world.h"

static gveh_fx gveh_demo_height(gveh_fx x, gveh_fx z)
{
    gveh_i32 xi;
    gveh_i32 zi;
    gveh_i32 wave;
    xi = gveh_fx_to_int(x);
    zi = gveh_fx_to_int(z);
    wave = ((xi * 3 + zi * 5) & 15) - 8;
    return gveh_fx_from_int(wave) / 16;
}

gveh_i32 gveh_demo_ground_probe(void *user, gveh_vec3 from, gveh_fx max_down, gveh_ground_hit *out_hit)
{
    gveh_fx h;
    gveh_i32 xi;
    (void)user;
    h = gveh_demo_height(from.x, from.z);
    if (from.y - h > max_down) return GVEH_FALSE;
    out_hit->hit = GVEH_TRUE;
    out_hit->point = gveh_v3(from.x, h, from.z);
    out_hit->normal = gveh_v3(0, GVEH_FX_ONE, 0);
    xi = gveh_fx_to_int(from.x);
    if (xi < -25) out_hit->surface_id = GVEH_SURF_GRAVEL;
    else if (xi > 35) out_hit->surface_id = GVEH_SURF_MUD;
    else out_hit->surface_id = GVEH_SURF_ASPHALT_DRY;
    out_hit->water_height = 0;
    return GVEH_TRUE;
}

gveh_i32 gveh_demo_sphere_probe(void *user, gveh_vec3 center, gveh_fx radius, gveh_ground_hit *out_hit)
{
    gveh_fx h;
    (void)user;
    h = gveh_demo_height(center.x, center.z);
    if (center.y - radius > h) return GVEH_FALSE;
    out_hit->hit = GVEH_TRUE;
    out_hit->point = gveh_v3(center.x, h, center.z);
    out_hit->normal = gveh_v3(0, GVEH_FX_ONE, 0);
    out_hit->surface_id = GVEH_SURF_ASPHALT_DRY;
    out_hit->water_height = 0;
    return GVEH_TRUE;
}

gveh_fx gveh_demo_water_sample(void *user, gveh_fx x, gveh_fx z, gveh_i32 tick)
{
    gveh_i32 a;
    (void)user;
    a = (gveh_fx_to_int(x) * 5 + gveh_fx_to_int(z) * 3 + tick) & 255;
    return gveh_fx_from_int(-1) + (gveh_fx_sin256(a) / 2);
}
