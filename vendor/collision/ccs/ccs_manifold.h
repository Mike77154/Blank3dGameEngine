#ifndef CCS_MANIFOLD_H
#define CCS_MANIFOLD_H

#include "ccs_math.h"

/* ============================================================
   Manifold configuration
   ============================================================ */

#ifndef CCS_MAX_CONTACTS
#define CCS_MAX_CONTACTS 4
#endif

/* Warm-start matching distance (world units, fixed). */
#ifndef CCS_WARMSTART_DISTANCE
/* 0.125 in Q16.16 */
#define CCS_WARMSTART_DISTANCE ((ccs_fixed)(CCS_FIXED_ONE / 8))
#endif

/* ============================================================
   Contact point (world space)
   ============================================================ */

typedef struct {
    ccs_vec3  point;       /* punto de contacto en mundo */
    ccs_vec3  normal;      /* normal A->B */
    ccs_fixed penetration; /* profundidad (>=0) */
    int       feature_id;  /* id estable para warm-start */

    /* Warm-start impulses (solver escribe aquí). */
    ccs_fixed normal_impulse;
    ccs_fixed tangent_impulse1;
    ccs_fixed tangent_impulse2;
} ccs_contact_point;

typedef struct {
    int count;
    ccs_contact_point contacts[CCS_MAX_CONTACTS];
} ccs_manifold;

/* ============================================================
   Helpers
   ============================================================ */

void ccs_manifold_clear(ccs_manifold* m);
void ccs_manifold_reset_impulses(ccs_manifold* m);

/* Copia impulsos del manifold previo al actual usando feature_id.
   Si no hay match exacto, intenta match por cercanía (umbral configurable).
*/
void ccs_manifold_warm_start(ccs_manifold* current, const ccs_manifold* prev);

#endif /* CCS_MANIFOLD_H */
