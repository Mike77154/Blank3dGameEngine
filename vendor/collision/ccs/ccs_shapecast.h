#ifndef CCS_SHAPECAST_H
#define CCS_SHAPECAST_H

#include "ccs_shapes.h"
#include "ccs_contact.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int hit;
    ccs_fixed fraction; /* 0..1 (Q16.16) */
    ccs_vec3 point;
    ccs_vec3 normal;
    ccs_fixed penetration;
} ccs_shapecast_hit;

/*
    Shapecast (sweep)
    -----------------
    Sweep translacional de un shape (sin rotación) desde start_center hasta end_center.

    - El shape debe tener "center" (ver ccs_shape_set_center/get_center).
    - No hace malloc.
    - Implementación determinista basada en:
        * broad AABB-sweep interval
        * sampling + binary search con ccs_dispatch_test

    Retorna 1 si hay hit.
*/
int ccs_shapecast_shape(
    void* moving_shape,
    ccs_vec3 start_center,
    ccs_vec3 end_center,
    const void* target_shape,
    ccs_shapecast_hit* out
);

#ifdef __cplusplus
}
#endif

#endif /* CCS_SHAPECAST_H */
