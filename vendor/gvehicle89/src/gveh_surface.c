#include "gveh_surface.h"
#include "gveh_math.h"

static void gveh_surface_set(gveh_surface *s, gveh_u8 id, gveh_u16 flags, gveh_i32 ml, gveh_i32 mt, gveh_i32 roll, gveh_i32 drag, gveh_i32 bump, gveh_i16 dust, gveh_i16 skid, gveh_i16 splash)
{
    s->id = id;
    s->flags = flags;
    s->mu_long = gveh_fx_from_int(ml) / 100;
    s->mu_lat = gveh_fx_from_int(mt) / 100;
    s->rolling = gveh_fx_from_int(roll) / 1000;
    s->drag = gveh_fx_from_int(drag) / 1000;
    s->bump = gveh_fx_from_int(bump) / 100;
    s->fx_dust = dust;
    s->fx_skid = skid;
    s->fx_splash = splash;
}

void gveh_surface_defaults(gveh_surface out_surfaces[GVEH_MAX_SURFACES])
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_MAX_SURFACES) {
        gveh_surface_set(&out_surfaces[i], (gveh_u8)i, 0, 80, 80, 15, 0, 0, 0, 0, 0);
        i++;
    }
    gveh_surface_set(&out_surfaces[GVEH_SURF_ASPHALT_DRY], GVEH_SURF_ASPHALT_DRY, 0, 120, 112, 12, 4, 2, 0, 1, 0);
    gveh_surface_set(&out_surfaces[GVEH_SURF_ASPHALT_WET], GVEH_SURF_ASPHALT_WET, GVEH_SURF_FLAG_SLIPPERY, 82, 72, 18, 10, 3, 0, 1, 1);
    gveh_surface_set(&out_surfaces[GVEH_SURF_GRAVEL], GVEH_SURF_GRAVEL, GVEH_SURF_FLAG_LOOSE, 78, 68, 35, 22, 16, 1, 1, 0);
    gveh_surface_set(&out_surfaces[GVEH_SURF_MUD], GVEH_SURF_MUD, GVEH_SURF_FLAG_LOOSE, 54, 42, 80, 45, 23, 1, 1, 1);
    gveh_surface_set(&out_surfaces[GVEH_SURF_SNOW], GVEH_SURF_SNOW, GVEH_SURF_FLAG_SLIPPERY, 36, 30, 45, 12, 8, 1, 1, 0);
    gveh_surface_set(&out_surfaces[GVEH_SURF_METAL], GVEH_SURF_METAL, GVEH_SURF_FLAG_SLIPPERY, 62, 58, 10, 2, 1, 0, 1, 0);
    gveh_surface_set(&out_surfaces[GVEH_SURF_WATER_SHALLOW], GVEH_SURF_WATER_SHALLOW, GVEH_SURF_FLAG_WATER, 34, 28, 110, 120, 30, 0, 0, 1);
    gveh_surface_set(&out_surfaces[GVEH_SURF_WATER_DEEP], GVEH_SURF_WATER_DEEP, GVEH_SURF_FLAG_WATER, 8, 8, 180, 220, 40, 0, 0, 1);
}

const gveh_surface *gveh_surface_get(const gveh_surface surfaces[GVEH_MAX_SURFACES], gveh_u8 id)
{
    if (id >= GVEH_MAX_SURFACES) return &surfaces[0];
    return &surfaces[id];
}
