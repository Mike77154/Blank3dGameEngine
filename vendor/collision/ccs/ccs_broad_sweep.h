#ifndef CCS_BROAD_SWEEP_H
#define CCS_BROAD_SWEEP_H

/*
    Sweep & Prune (broadphase)
    -------------------------
    Versión simple, determinista y "engine-friendly".

    No hace malloc.

    Configuración:
    - Ver ccs_config.h para CCS_SWEEP_MAX_OBJECTS / CCS_SWEEP_MAX_PAIRS.

    Nota:
    - Si se excede CCS_SWEEP_MAX_PAIRS, se activa ctx->pair_overflow
      (no se agregan pares extra, pero queda registrado el evento).
*/

#include "ccs_config.h"
#include "ccs_shapes.h" /* ccs_aabb */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCS_SWEEP_MAX_OBJECTS
#define CCS_SWEEP_MAX_OBJECTS CCS_MAX_BODIES
#endif

#ifndef CCS_SWEEP_MAX_PAIRS
#define CCS_SWEEP_MAX_PAIRS 1024
#endif

typedef struct {
    int a;
    int b;
} CCS_Pair;

/* Entrada al broadphase */
typedef struct {
    ccs_aabb aabb;
    int object_id;
} CCS_SweepObject;

/* Contexto del sweep */
typedef struct {
    CCS_SweepObject objects[CCS_SWEEP_MAX_OBJECTS];
    int object_count;

    CCS_Pair pairs[CCS_SWEEP_MAX_PAIRS];
    int pair_count;

    /* flags de diagnóstico */
    int object_overflow; /* 1 si se intentó exceder CCS_SWEEP_MAX_OBJECTS */
    int pair_overflow;   /* 1 si se intentó exceder CCS_SWEEP_MAX_PAIRS */
} CCS_SweepContext;

/* API */
void ccs_sweep_init(CCS_SweepContext* ctx);
void ccs_sweep_add(CCS_SweepContext* ctx, const ccs_aabb* aabb, int object_id);
void ccs_sweep_compute(CCS_SweepContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* CCS_BROAD_SWEEP_H */
