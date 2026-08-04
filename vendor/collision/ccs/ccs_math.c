#include "ccs_math.h"

/* ============================================================
   Constructors
   ============================================================ */

ccs_vec3 ccs_vec3_make(ccs_fixed x, ccs_fixed y, ccs_fixed z)
{
    ccs_vec3 r;
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

/* ============================================================
   Basic ops
   ============================================================ */

ccs_vec3 ccs_vec3_add(ccs_vec3 a, ccs_vec3 b)
{
    ccs_vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

ccs_vec3 ccs_vec3_sub(ccs_vec3 a, ccs_vec3 b)
{
    ccs_vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

ccs_vec3 ccs_vec3_scale(ccs_vec3 v, ccs_fixed s)
{
    ccs_vec3 r;
    r.x = ccs_fixed_mul(v.x, s);
    r.y = ccs_fixed_mul(v.y, s);
    r.z = ccs_fixed_mul(v.z, s);
    return r;
}

ccs_fixed ccs_vec3_dot(ccs_vec3 a, ccs_vec3 b)
{
    return ccs_fixed_mul(a.x, b.x)
         + ccs_fixed_mul(a.y, b.y)
         + ccs_fixed_mul(a.z, b.z);
}

ccs_vec3 ccs_vec3_cross(ccs_vec3 a, ccs_vec3 b)
{
    ccs_vec3 r;
    r.x = ccs_fixed_mul(a.y, b.z) - ccs_fixed_mul(a.z, b.y);
    r.y = ccs_fixed_mul(a.z, b.x) - ccs_fixed_mul(a.x, b.z);
    r.z = ccs_fixed_mul(a.x, b.y) - ccs_fixed_mul(a.y, b.x);
    return r;
}

/* ============================================================
   Length
   ============================================================ */

ccs_fixed ccs_vec3_len_sq(ccs_vec3 v)
{
    return ccs_vec3_dot(v, v);
}

/* Integer sqrt wrapper – deterministic */
ccs_fixed ccs_vec3_len(ccs_vec3 v)
{
    return CCS_FIXED_SQRT(ccs_vec3_len_sq(v));
}

/* ============================================================
   Normalization
   ============================================================ */

ccs_vec3 ccs_vec3_normalize(ccs_vec3 v)
{
    ccs_fixed len;
    len = ccs_vec3_len(v);
    if (len == 0)
        return ccs_vec3_zero();

    return ccs_vec3_scale(v, ccs_fixed_div(CCS_FIXED_ONE, len));
}

/* ============================================================
   Utilities
   ============================================================ */

ccs_vec3 ccs_vec3_neg(ccs_vec3 v)
{
    ccs_vec3 r;
    r.x = -v.x;
    r.y = -v.y;
    r.z = -v.z;
    return r;
}

ccs_vec3 ccs_vec3_abs(ccs_vec3 v)
{
    ccs_vec3 r;
    r.x = ccs_fixed_abs(v.x);
    r.y = ccs_fixed_abs(v.y);
    r.z = ccs_fixed_abs(v.z);
    return r;
}

ccs_vec3 ccs_vec3_clamp(ccs_vec3 v, ccs_fixed min, ccs_fixed max)
{
    ccs_vec3 r;
    r.x = ccs_fixed_clamp(v.x, min, max);
    r.y = ccs_fixed_clamp(v.y, min, max);
    r.z = ccs_fixed_clamp(v.z, min, max);
    return r;
}

/* Project vector a onto vector onto */
ccs_vec3 ccs_vec3_project(ccs_vec3 a, ccs_vec3 onto)
{
    ccs_fixed denom;
    ccs_fixed scale;

    denom = ccs_vec3_dot(onto, onto);
    if (denom == 0)
        return ccs_vec3_zero();

    scale = ccs_fixed_div(ccs_vec3_dot(a, onto), denom);
    return ccs_vec3_scale(onto, scale);
}

ccs_vec3 ccs_vec3_zero(void)
{
    return ccs_vec3_make(0, 0, 0);
}

/* ============================================================
   EXPANSIONES SEGURAS
   ============================================================ */

int ccs_vec3_is_zero(ccs_vec3 v)
{
    return (v.x == 0 && v.y == 0 && v.z == 0);
}

int ccs_vec3_near_zero(ccs_vec3 v, ccs_fixed eps)
{
    return ccs_fixed_abs(v.x) <= eps
        && ccs_fixed_abs(v.y) <= eps
        && ccs_fixed_abs(v.z) <= eps;
}

int ccs_vec3_normalize_safe(ccs_vec3* v)
{
    ccs_fixed len;
    if (!v)
        return 0;

    len = ccs_vec3_len(*v);
    if (len == 0)
        return 0;

    *v = ccs_vec3_scale(*v, ccs_fixed_div(CCS_FIXED_ONE, len));
    return 1;
}

/* ============================================================
   Distances
   ============================================================ */

ccs_fixed ccs_vec3_dist_sq(ccs_vec3 a, ccs_vec3 b)
{
    return ccs_vec3_len_sq(ccs_vec3_sub(b, a));
}

ccs_fixed ccs_vec3_dist(ccs_vec3 a, ccs_vec3 b)
{
    return CCS_FIXED_SQRT(ccs_vec3_dist_sq(a, b));
}

/* ============================================================
   Interpolation
   ============================================================ */

ccs_vec3 ccs_vec3_lerp(ccs_vec3 a, ccs_vec3 b, ccs_fixed t)
{
    ccs_vec3 r;
    r.x = a.x + ccs_fixed_mul(b.x - a.x, t);
    r.y = a.y + ccs_fixed_mul(b.y - a.y, t);
    r.z = a.z + ccs_fixed_mul(b.z - a.z, t);
    return r;
}

/* ============================================================
   Projection helpers
   ============================================================ */

ccs_vec3 ccs_vec3_project_on_plane(ccs_vec3 v, ccs_vec3 normal)
{
    /* v - proj(v, n) */
    return ccs_vec3_sub(v, ccs_vec3_project(v, normal));
}

/* ============================================================
   Component-wise helpers
   ============================================================ */

ccs_vec3 ccs_vec3_min(ccs_vec3 a, ccs_vec3 b)
{
    ccs_vec3 r;
    r.x = (a.x < b.x) ? a.x : b.x;
    r.y = (a.y < b.y) ? a.y : b.y;
    r.z = (a.z < b.z) ? a.z : b.z;
    return r;
}

ccs_vec3 ccs_vec3_max(ccs_vec3 a, ccs_vec3 b)
{
    ccs_vec3 r;
    r.x = (a.x > b.x) ? a.x : b.x;
    r.y = (a.y > b.y) ? a.y : b.y;
    r.z = (a.z > b.z) ? a.z : b.z;
    return r;
}

/* ============================================================
   Axis helpers
   ============================================================ */

ccs_vec3 ccs_vec3_axis_x(void)
{
    return ccs_vec3_make(CCS_FIXED_ONE, 0, 0);
}

ccs_vec3 ccs_vec3_axis_y(void)
{
    return ccs_vec3_make(0, CCS_FIXED_ONE, 0);
}

ccs_vec3 ccs_vec3_axis_z(void)
{
    return ccs_vec3_make(0, 0, CCS_FIXED_ONE);
}
