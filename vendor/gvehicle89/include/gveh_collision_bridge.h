#ifndef GVEH_COLLISION_BRIDGE_H
#define GVEH_COLLISION_BRIDGE_H

#include "gveh.h"

typedef struct gveh_collision_contact_s {
    gveh_i16 hit;
    gveh_i16 id_a;
    gveh_i16 id_b;
    gveh_vec3 normal;
    gveh_fx penetration;
    gveh_fx relative_speed;
} gveh_collision_contact;

gveh_fx gveh_collision_vehicle_radius(const gveh_vehicle *v);
gveh_i32 gveh_collision_resolve_pair(gveh_vehicle *a, gveh_vehicle *b, gveh_i16 id_a, gveh_i16 id_b, gveh_collision_contact *out_contact);

#endif
