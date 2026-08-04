#ifndef CCS_WORLD_H
#define CCS_WORLD_H

#include "ccs_config.h"
#include "ccs_shapes.h"
#include "ccs_raycast.h"

/* ============================================================
   World
   ============================================================

   Contrato (ownership + IDs)
   --------------------------
   - ccs_body.position:
       * Es un PUNTERO EXTERNO a un ccs_vec3 manejado por el usuario.
       * CCS NO asigna/libera esa memoria.
       * Debe permanecer válido mientras el body exista en el world.
       * Si position == NULL, el shape se asume en coordenadas ya finales
         (útil para estáticos como trimesh/heightfield).

   - ccs_body.shape:
       * Es un PUNTERO EXTERNO a un shape (ccs_shape_*).
       * CCS NO asigna/libera esa memoria.
       * El objeto shape debe vivir al menos lo mismo que el body.

   - ccs_world_remove(id):
       * Implementado como swap-remove.
       * IMPORTANTE: esto puede CAMBIAR el ID de otro body (el último).
         Si guardas IDs externamente, debes refrescarlos tras remove.
*/

typedef struct {
    ccs_vec3* position; /* puntero externo a la posición */
    ccs_shape* shape;

    ccs_u32 layer;
    ccs_u32 mask;

    void* user_data;
} ccs_body;

extern ccs_body ccs_bodies[CCS_MAX_BODIES];
extern int ccs_body_count;

void ccs_world_clear(void);

int  ccs_world_add(ccs_vec3* position, ccs_shape* shape);
void ccs_world_remove(int id);

/* broadphase selection */
void ccs_world_set_broadphase(int broadphase_id);
int  ccs_world_get_broadphase(void);

/*
    Diagnóstico de overflow del broadphase (último ccs_world_step)
    -------------------------------------------------------------
    Bitmask:
      1 << 0 : Sweep pares overflow (CCS_SWEEP_MAX_PAIRS)
      1 << 1 : Sweep objetos overflow (CCS_SWEEP_MAX_OBJECTS)
      1 << 2 : Grid3D overflow (objetos fuera de bounds)
*/

enum {
    CCS_WORLD_OVERFLOW_SWEEP_PAIRS   = 1u << 0,
    CCS_WORLD_OVERFLOW_SWEEP_OBJECTS = 1u << 1,
    CCS_WORLD_OVERFLOW_GRID3D_OOB    = 1u << 2
};

unsigned int ccs_world_get_overflow_flags(void);

void ccs_world_step(void);

/* Raycast contra todos los cuerpos (hit más cercano). layer_mask=0 => sin filtro */
int ccs_world_raycast(
    const ccs_ray* ray,
    ccs_u32 layer_mask,
    ccs_raycast_hit* out_hit,
    int* out_body_id
);

#endif /* CCS_WORLD_H */
