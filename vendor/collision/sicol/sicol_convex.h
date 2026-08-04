#ifndef SICOL_CONVEX_H
#define SICOL_CONVEX_H

/* ============================================================
 * SICOL - Support mapping helpers for convex / convex-like shapes
 * ============================================================ */

#include "sicol_shape.h"

int sicol_shape_is_support_mapped(const sicol_shape_t* s);
void sicol_shape_center_point(const sicol_shape_t* s, fx out[3]);
int sicol_shape_support_point(const sicol_shape_t* s, const fx dir[3], fx out[3]);

#endif /* SICOL_CONVEX_H */
