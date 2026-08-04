#ifndef CCS_BROAD_GRID3D_H
#define CCS_BROAD_GRID3D_H

/*
    Broad Grid 3D (broadphase)
    --------------------------
    Grid uniforme 3D para generar pares candidatos.

    Este módulo es FINITO (dimensiones fijas). Para un broadphase más
    generalista (sin límites de mundo), usa Sweep (ccs_broad_sweep).

    Configuración (compile-time): ver ccs_config.h
    - CCS_BG3D_X/Y/Z
    - CCS_BG3D_NEIGHBOR_RANGE

    Diagnóstico:
    - Si un objeto cae fuera de los límites del grid, se marca como "overflow"
      y se agrega a una lista secundaria.
    - La función ccs_broad_grid3d_overflow_count() permite detectar esta
      situación (útil para debug/telemetría).
*/

#include "ccs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ccs_pair_fn)(int a, int b);

void ccs_broad_grid3d_clear(void);

/*
    Inserción: gx/gy/gz ya deben ser índices de celda válidos:
    0 <= gx < CCS_BG3D_X, etc.

    Tip: en el world, normalmente calculas:
      gx = (wx / CELL_SIZE) + BIAS_X;
*/
void ccs_broad_grid3d_insert(int id, int gx, int gy, int gz);

/*
    Itera pares candidatos.
    Incluye vecinos dentro de CCS_BG3D_NEIGHBOR_RANGE.
*/
void ccs_broad_grid3d_for_each_pair(ccs_pair_fn fn);

/* Diagnóstico */
int ccs_broad_grid3d_overflow_count(void);

#ifdef __cplusplus
}
#endif

#endif /* CCS_BROAD_GRID3D_H */
