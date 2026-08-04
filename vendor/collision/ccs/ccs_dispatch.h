#ifndef CCS_DISPATCH_H
#define CCS_DISPATCH_H

/*
    ccs_dispatch
    ------------
    Selecciona la rutina narrow-phase apropiada en base al tipo del shape
    (ccs_shape_header.type).

    IMPORTANTE:
    - El parámetro "shape" aquí es un puntero al SHAPE REAL (ccs_shape_sphere,
      ccs_shape_box, etc.).
    - El shape real SIEMPRE debe comenzar con ccs_shape_header.

    C89/PS1 notes:
    - Sin <stdint.h>
    - Sin malloc
*/

#include "ccs_shapes.h"
#include "ccs_narrow.h" /* ccs_contact, ccs_manifold, ccs_box/ccs_sphere */

/* ============================================================
   Capabilities (por tipo de shape)
   ============================================================ */

enum {
    CCS_SHAPE_SUPPORTS_MANIFOLD = 1u << 0
};

/* ============================================================
   Public API
   ============================================================ */

/* Boolean-only overlap test (rápido, sin datos de contacto) */
int ccs_dispatch_test(
    const void* shape_a,
    const void* shape_b
);

/* Single contact */
int ccs_dispatch_collide(
    const void* shape_a,
    const void* shape_b,
    ccs_contact* out
);

/* Manifold (si no hay rutina específica, hace fallback a single-contact) */
int ccs_dispatch_collide_manifold(
    const void* shape_a,
    const void* shape_b,
    ccs_manifold* out
);

/* Capability query */
unsigned int ccs_dispatch_shape_caps(ccs_shape_type type);

#endif /* CCS_DISPATCH_H */
