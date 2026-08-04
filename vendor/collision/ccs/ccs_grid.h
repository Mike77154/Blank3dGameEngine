#ifndef CCS_GRID_H
#define CCS_GRID_H

#include "ccs_config.h"
#include "ccs_math.h"
#include "ccs_shapes.h"

/*
    Grid uniforme 2D (XZ)
    --------------------
    Broadphase muy simple.

    Nota importante:
    - Esta implementación inserta cada objeto en UNA sola celda (la del centro del AABB).
    - Si necesitas exactitud con objetos grandes, usa ccs_broad_sweep o inserta por AABB
      con una estructura que soporte múltiples referencias por objeto.
*/

#ifndef CCS_GRID_SIZE
#define CCS_GRID_SIZE 64
#endif

#ifndef CCS_GRID_DIM
#define CCS_GRID_DIM  16
#endif

#ifndef CCS_GRID_MAX_OBJECTS
#define CCS_GRID_MAX_OBJECTS CCS_MAX_BODIES
#endif

#ifndef CCS_GRID_BIAS
#define CCS_GRID_BIAS (CCS_GRID_DIM / 2)
#endif

typedef struct {
    int head;
} ccs_grid_cell;

/* grid 2D sobre plano XZ */
extern ccs_grid_cell ccs_grid[CCS_GRID_DIM][CCS_GRID_DIM];
extern int           ccs_grid_next[CCS_GRID_MAX_OBJECTS];

/* API */
void ccs_grid_clear(void);
void ccs_grid_insert_aabb(int id, const ccs_aabb* aabb);

#endif /* CCS_GRID_H */
