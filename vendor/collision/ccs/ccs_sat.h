#ifndef CCS_SAT_H
#define CCS_SAT_H

#include "ccs_geom.h"
#include "ccs_contact.h"
#include "ccs_manifold.h"

/*
    SAT module
    ----------
    OBB vs OBB por Separating Axis Theorem.

    Implementa 15 pruebas estándar:
    - 3 ejes de A (normales de cara)
    - 3 ejes de B (normales de cara)
    - 9 ejes cross (Ai x Bj)

    Además:
    - Selección de normal por el eje de mínima penetración ENTRE LOS 15.
    - Generación de manifold estable (hasta 4 puntos) para casos face-face.
    - Caso edge-edge: 1 contacto (closest points between edges).

    Requisitos:
    - a->axis[i] y b->axis[j] deberían ser ortonormales (unit length).
*/

int ccs_sat_obb_obb(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_contact* out
);

int ccs_sat_obb_obb_manifold(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_manifold* out
);

#endif /* CCS_SAT_H */
