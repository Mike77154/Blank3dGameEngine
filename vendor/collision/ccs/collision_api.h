#ifndef COLLISION_API_H
#define COLLISION_API_H

#include "ccs_world.h"
#include "ccs_raycast.h"

/* ============================================================
   Small C API wrapper
   ============================================================

   Este wrapper existe para pruebas rápidas.
   Internamente usa ccs_world_*.
*/

void collision_reset(void);

int collision_create_box_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed hx, ccs_fixed hy, ccs_fixed hz,
    unsigned int layer
);

int collision_create_sphere_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed radius,
    unsigned int layer
);

int collision_create_capsule_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed ax, ccs_fixed ay, ccs_fixed az,
    ccs_fixed half_height,
    ccs_fixed radius,
    unsigned int layer
);

int collision_create_obb_fixed(
    ccs_fixed x, ccs_fixed y, ccs_fixed z,
    ccs_fixed hx, ccs_fixed hy, ccs_fixed hz,
    /* axes must be (approximately) orthonormal */
    ccs_fixed ax0, ccs_fixed ay0, ccs_fixed az0,
    ccs_fixed ax1, ccs_fixed ay1, ccs_fixed az1,
    ccs_fixed ax2, ccs_fixed ay2, ccs_fixed az2,
    unsigned int layer
);

void collision_set_position_fixed(int id, ccs_fixed x, ccs_fixed y, ccs_fixed z);

void collision_step(void);

/* Raycast (closest hit). Returns 1 if hit. */
int collision_raycast_fixed(
    ccs_fixed ox, ccs_fixed oy, ccs_fixed oz,
    ccs_fixed dx, ccs_fixed dy, ccs_fixed dz,
    ccs_fixed tmin, ccs_fixed tmax,
    unsigned int layer_mask,
    int* out_id,
    ccs_fixed* out_t,
    ccs_fixed* out_px, ccs_fixed* out_py, ccs_fixed* out_pz,
    ccs_fixed* out_nx, ccs_fixed* out_ny, ccs_fixed* out_nz
);

#endif
