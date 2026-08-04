#ifndef CCS_RAYCAST_H
#define CCS_RAYCAST_H

#include "ccs_math.h"
#include "ccs_narrow.h"   /* ccs_sphere / ccs_capsule / ccs_obb */
#include "ccs_shapes.h"   /* shape structs */

/* ============================================================
   Raycast
   ============================================================

   Todas las distancias están en fixed (Q16.16).
   Por convención, el rayo usa dirección normalizada.

   - origin: punto inicial
   - dir: dirección (idealmente unit length)
   - tmin/tmax: rango válido

   hit.t es la distancia desde origin.
*/

typedef struct {
    ccs_vec3  origin;
    ccs_vec3  dir;
    ccs_fixed tmin;
    ccs_fixed tmax;
} ccs_ray;

typedef struct {
    int hit;
    ccs_fixed t;
    ccs_vec3 point;
    ccs_vec3 normal; /* normal saliente del shape */
    int feature_id;
} ccs_raycast_hit;

/* Helpers */
ccs_ray ccs_ray_make(ccs_vec3 origin, ccs_vec3 dir, ccs_fixed tmin, ccs_fixed tmax);

/* Raycast vs primitives */
int ccs_raycast_sphere(const ccs_ray* ray, const ccs_sphere* s, ccs_raycast_hit* out);
int ccs_raycast_aabb(const ccs_ray* ray, ccs_vec3 bmin, ccs_vec3 bmax, ccs_raycast_hit* out);
int ccs_raycast_obb(const ccs_ray* ray, const ccs_obb* o, ccs_raycast_hit* out);
int ccs_raycast_capsule(const ccs_ray* ray, const ccs_capsule* c, ccs_raycast_hit* out);

/* Raycast vs shapes */
int ccs_raycast_shape(const ccs_ray* ray, const void* shape, ccs_raycast_hit* out);

#endif /* CCS_RAYCAST_H */
