#ifndef CCS_TRIANGLE_H
#define CCS_TRIANGLE_H

#include "ccs_math.h"
#include "ccs_contact.h"
#include "ccs_narrow.h" /* ccs_sphere/ccs_capsule/ccs_obb */
#include "ccs_raycast.h" /* ccs_ray/ccs_raycast_hit */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_vec3 c;
} ccs_triangle;

/* Returns a (non-zero) normal; attempts to normalize safely. */
ccs_vec3 ccs_triangle_normal(const ccs_triangle* t);

/* Closest point on triangle to p */
ccs_vec3 ccs_triangle_closest_point(const ccs_triangle* t, ccs_vec3 p);

/* Collision contacts: normal is from triangle -> shape */
int ccs_triangle_sphere_contact(const ccs_triangle* tri, const ccs_sphere* s, ccs_contact* out);
int ccs_triangle_capsule_contact(const ccs_triangle* tri, const ccs_capsule* c, ccs_contact* out);
int ccs_triangle_obb_contact(const ccs_triangle* tri, const ccs_obb* o, ccs_contact* out);

/* Raycast vs triangle (two-sided). Returns 1 if hit within [tmin,tmax]. */
int ccs_raycast_triangle(const ccs_ray* ray, const ccs_triangle* tri, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_TRIANGLE_H */
