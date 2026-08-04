#ifndef CCS_RESOLVE_H
#define CCS_RESOLVE_H

#include "ccs_math.h"
#include "ccs_narrow.h"

/*
    Resolución posicional
    ---------------------
    Corrige penetración empujando posiciones.
    NO integra velocidad.
*/

/* Resolución simple (single contact) */
void ccs_resolve_contact(
    ccs_vec3* posA,
    ccs_vec3* posB,
    const ccs_contact* c
);

/* Resolución por manifold (recomendado) */
void ccs_resolve_manifold(
    ccs_vec3* posA,
    ccs_vec3* posB,
    const ccs_manifold* m
);

#endif /* CCS_RESOLVE_H */
