#ifndef CCS_MATH_H
#define CCS_MATH_H

#include "ccs_fixed.h"

/* ============================================================
   Vector 3 fixed-point
   ============================================================ */

typedef struct {
    ccs_fixed x, y, z;
} ccs_vec3;

/* Constructor helper (C89-safe; avoids non-constant aggregate init warnings) */
ccs_vec3  ccs_vec3_make(ccs_fixed x, ccs_fixed y, ccs_fixed z);

/* ============================================================
   Basic operations (ORIGINALES)
   ============================================================ */

ccs_vec3  ccs_vec3_add(ccs_vec3 a, ccs_vec3 b);
ccs_vec3  ccs_vec3_sub(ccs_vec3 a, ccs_vec3 b);
ccs_vec3  ccs_vec3_scale(ccs_vec3 v, ccs_fixed s);
ccs_fixed ccs_vec3_dot(ccs_vec3 a, ccs_vec3 b);
ccs_vec3  ccs_vec3_cross(ccs_vec3 a, ccs_vec3 b);

/* ============================================================
   Length & normalization (ORIGINALES)
   ============================================================ */

ccs_fixed ccs_vec3_len_sq(ccs_vec3 v);
ccs_fixed ccs_vec3_len(ccs_vec3 v);
ccs_vec3  ccs_vec3_normalize(ccs_vec3 v);

/* ============================================================
   Utilities for collision math (ORIGINALES)
   ============================================================ */

ccs_vec3  ccs_vec3_neg(ccs_vec3 v);
ccs_vec3  ccs_vec3_abs(ccs_vec3 v);
ccs_vec3  ccs_vec3_clamp(ccs_vec3 v, ccs_fixed min, ccs_fixed max);
ccs_vec3  ccs_vec3_project(ccs_vec3 a, ccs_vec3 onto);

/* ============================================================
   Helpers (ORIGINAL)
   ============================================================ */

ccs_vec3  ccs_vec3_zero(void);

/* ============================================================
   EXPANSIONES SEGURAS (NUEVAS)
   ============================================================ */

/* Comparaciones y tolerancias */
int       ccs_vec3_is_zero(ccs_vec3 v);
int       ccs_vec3_near_zero(ccs_vec3 v, ccs_fixed eps);

/* Normalización segura (no pierde info) */
int       ccs_vec3_normalize_safe(ccs_vec3* v);

/* Distancias */
ccs_fixed ccs_vec3_dist_sq(ccs_vec3 a, ccs_vec3 b);
ccs_fixed ccs_vec3_dist(ccs_vec3 a, ccs_vec3 b);

/* Interpolación */
ccs_vec3  ccs_vec3_lerp(ccs_vec3 a, ccs_vec3 b, ccs_fixed t);

/* Proyecciones útiles para colisión */
ccs_vec3  ccs_vec3_project_on_plane(ccs_vec3 v, ccs_vec3 normal);

/* Component-wise helpers */
ccs_vec3  ccs_vec3_min(ccs_vec3 a, ccs_vec3 b);
ccs_vec3  ccs_vec3_max(ccs_vec3 a, ccs_vec3 b);

/* Axis helpers (debug / fallback determinista) */
ccs_vec3  ccs_vec3_axis_x(void);
ccs_vec3  ccs_vec3_axis_y(void);
ccs_vec3  ccs_vec3_axis_z(void);

#endif /* CCS_MATH_H */
