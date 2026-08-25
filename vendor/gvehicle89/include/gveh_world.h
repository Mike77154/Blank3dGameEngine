#ifndef GVEH_WORLD_H
#define GVEH_WORLD_H

#include "gveh_math.h"
#include "gveh_surface.h"

typedef struct gveh_ground_hit_s {
    gveh_i32 hit;
    gveh_vec3 point;
    gveh_vec3 normal;
    gveh_u8 surface_id;
    gveh_fx water_height;
} gveh_ground_hit;

typedef struct gveh_world_i_s {
    void *user;
    gveh_i32 (*ground_probe)(void *user, gveh_vec3 from, gveh_fx max_down, gveh_ground_hit *out_hit);
    gveh_i32 (*sphere_probe)(void *user, gveh_vec3 center, gveh_fx radius, gveh_ground_hit *out_hit);
    gveh_fx  (*water_sample)(void *user, gveh_fx x, gveh_fx z, gveh_i32 tick);
} gveh_world_i;

gveh_i32 gveh_demo_ground_probe(void *user, gveh_vec3 from, gveh_fx max_down, gveh_ground_hit *out_hit);
gveh_i32 gveh_demo_sphere_probe(void *user, gveh_vec3 center, gveh_fx radius, gveh_ground_hit *out_hit);
gveh_fx  gveh_demo_water_sample(void *user, gveh_fx x, gveh_fx z, gveh_i32 tick);

#endif
