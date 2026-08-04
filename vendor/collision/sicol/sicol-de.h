#ifndef SICOL_DE_H
#define SICOL_DE_H

/* ============================================================
 * SICOL-DE legacy compatibility API
 * ------------------------------------------------------------
 * Simple integer-unit wrapper over the fixed-point world.
 * Values passed here are interpreted as integer world units.
 * ============================================================ */

#include "sicol_math_fx.h"
#include "sicol_shape.h"

typedef int sicol_fix;
typedef int sicol_id;

#define SICOL_MAX 512

void sicol_init(void);

sicol_id sicol_create_aabb(
    sicol_fix x, sicol_fix y, sicol_fix z,
    sicol_fix hx, sicol_fix hy, sicol_fix hz
);

sicol_id sicol_create_aabb_fx(
    fx x, fx y, fx z,
    fx hx, fx hy, fx hz
);

void sicol_set_position(
    sicol_id id,
    sicol_fix x, sicol_fix y, sicol_fix z
);

void sicol_set_position_fx(
    sicol_id id,
    fx x, fx y, fx z
);

int sicol_get_shape(sicol_id id, sicol_shape_t* out_shape);
int sicol_test(sicol_id a, sicol_id b);
void sicol_kill(sicol_id id);

#endif /* SICOL_DE_H */
