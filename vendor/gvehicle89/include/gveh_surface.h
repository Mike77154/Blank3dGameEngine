#ifndef GVEH_SURFACE_H
#define GVEH_SURFACE_H

#include "gveh_types.h"

#define GVEH_SURF_ASPHALT_DRY   0
#define GVEH_SURF_ASPHALT_WET   1
#define GVEH_SURF_GRAVEL        2
#define GVEH_SURF_MUD           3
#define GVEH_SURF_SNOW          4
#define GVEH_SURF_METAL         5
#define GVEH_SURF_WATER_SHALLOW 6
#define GVEH_SURF_WATER_DEEP    7

#define GVEH_SURF_FLAG_WATER    1u
#define GVEH_SURF_FLAG_LOOSE    2u
#define GVEH_SURF_FLAG_SLIPPERY 4u

typedef struct gveh_surface_s {
    gveh_u8 id;
    gveh_u16 flags;
    gveh_fx mu_long;
    gveh_fx mu_lat;
    gveh_fx rolling;
    gveh_fx drag;
    gveh_fx bump;
    gveh_i16 fx_dust;
    gveh_i16 fx_skid;
    gveh_i16 fx_splash;
} gveh_surface;

void gveh_surface_defaults(gveh_surface out_surfaces[GVEH_MAX_SURFACES]);
const gveh_surface *gveh_surface_get(const gveh_surface surfaces[GVEH_MAX_SURFACES], gveh_u8 id);

#endif
