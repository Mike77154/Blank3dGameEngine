#ifndef CCS_CONFIG_H
#define CCS_CONFIG_H

/*
    ccs_config.h
    ------------
    Configuración central (compile-time) para la librería.

    Nota PS1:
    - Evita malloc/stdio/math.
    - Mantiene límites estáticos y deterministas.

    Puedes sobreescribir cualquier macro definiéndola ANTES de incluir los headers.
*/

/* ============================================================
   Feature toggles (set to 0 to compile out modules)
   ============================================================ */

#ifndef CCS_ENABLE_PLANE
#define CCS_ENABLE_PLANE 1
#endif

#ifndef CCS_ENABLE_TRIMESH
#define CCS_ENABLE_TRIMESH 1
#endif

#ifndef CCS_ENABLE_HEIGHTFIELD
#define CCS_ENABLE_HEIGHTFIELD 1
#endif

#ifndef CCS_ENABLE_CONVEX
#define CCS_ENABLE_CONVEX 1
#endif

#ifndef CCS_ENABLE_COMPOUND
#define CCS_ENABLE_COMPOUND 1
#endif


/* ============================================================
   Límites globales
   ============================================================ */

#ifndef CCS_MAX_BODIES
#define CCS_MAX_BODIES 256
#endif

/* ============================================================
   Broadphase IDs (para ccs_world)
   ============================================================ */

#define CCS_WORLDBP_GRID3D 0
#define CCS_WORLDBP_SWEEP  1

#ifndef CCS_WORLD_DEFAULT_BROADPHASE
#define CCS_WORLD_DEFAULT_BROADPHASE CCS_WORLDBP_SWEEP
#endif

/* ============================================================
   Grid 3D broadphase (rápido, PS1-friendly)
   ============================================================ */

/* tamaño de celda en WORLD UNITS enteras (NO fixed) */
#ifndef CCS_BG3D_CELL_SIZE
#define CCS_BG3D_CELL_SIZE 64
#endif

#ifndef CCS_BG3D_X
#define CCS_BG3D_X 16
#endif

#ifndef CCS_BG3D_Y
#define CCS_BG3D_Y 8
#endif

#ifndef CCS_BG3D_Z
#define CCS_BG3D_Z 16
#endif

/* Bias para permitir coordenadas negativas: por defecto el grid está centrado en (0,0,0) */
#ifndef CCS_BG3D_BIAS_X
#define CCS_BG3D_BIAS_X (CCS_BG3D_X / 2)
#endif

#ifndef CCS_BG3D_BIAS_Y
#define CCS_BG3D_BIAS_Y (CCS_BG3D_Y / 2)
#endif

#ifndef CCS_BG3D_BIAS_Z
#define CCS_BG3D_BIAS_Z (CCS_BG3D_Z / 2)
#endif

/* Vecindad a testear (1 = 27 celdas). Si eliges CELL_SIZE >= diámetro máximo de objeto,
   1 suele bastar. */
#ifndef CCS_BG3D_NEIGHBOR_RANGE
#define CCS_BG3D_NEIGHBOR_RANGE 1
#endif

/* ============================================================
   Sweep & Prune broadphase (generalista)
   ============================================================ */

#ifndef CCS_SWEEP_MAX_OBJECTS
#define CCS_SWEEP_MAX_OBJECTS CCS_MAX_BODIES
#endif

#ifndef CCS_SWEEP_MAX_PAIRS
#define CCS_SWEEP_MAX_PAIRS 1024
#endif

/* ============================================================
   Planes (AABB aproximado)
   ============================================================

   Los shapes PLANE/HALFSPACE son infinitos; para integrarlos con broadphases
   finitos, CCS les asigna un AABB "muy grande" centrado en su punto.

   Unidad: WORLD UNITS enteras (NO fixed).
*/
#ifndef CCS_PLANE_AABB_HALF_EXTENTS
#define CCS_PLANE_AABB_HALF_EXTENTS 4096
#endif

/* ============================================================
   Compound shapes
   ============================================================ */

#ifndef CCS_COMPOUND_MAX_CHILDREN
#define CCS_COMPOUND_MAX_CHILDREN 8
#endif

/* ============================================================
   Queries (trimesh/heightfield)
   ============================================================

   Máximo de triángulos candidatos almacenados por query cuando se usa BVH/AABBTree.
   (sin malloc; se usa stack/local arrays)
*/
#ifndef CCS_QUERY_MAX_CANDIDATES
#define CCS_QUERY_MAX_CANDIDATES 64
#endif

/* ============================================================
   Shapecast (sweep) - configuración
   ============================================================ */

#ifndef CCS_SHAPECAST_SAMPLES
#define CCS_SHAPECAST_SAMPLES 16
#endif

#ifndef CCS_SHAPECAST_ITERATIONS
#define CCS_SHAPECAST_ITERATIONS 12
#endif

#endif /* CCS_CONFIG_H */
