/* gamlib3d_math.h - Núcleo matemático fixed-point Q20.12 para gamlib3d */
#ifndef GAMLIB3D_MATH_H
#define GAMLIB3D_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/*  Tipo base fixed                                                          */
/* ------------------------------------------------------------------------- */

/* C89 no define int32_t. Usamos long y saturamos a 32 bits con signo.      */
typedef long g3d_fix;

/* Q20.12: 20 bits enteros (incluyendo signo), 12 bits fraccionales.        */
#define G3D_FIX_FRAC        12
#define G3D_FIX_ONE         ((g3d_fix)(1L << G3D_FIX_FRAC))
#define G3D_FIX_HALF        ((g3d_fix)(1L << (G3D_FIX_FRAC - 1)))
#define G3D_FIX_EPSILON     ((g3d_fix)1L)

#define G3D_FIX_MAX         ((g3d_fix)2147483647L)
#define G3D_FIX_MIN         ((g3d_fix)(-2147483647L - 1L))

#define G3D_FIX_FROM_INT(i) ((g3d_fix)((g3d_fix)(i) * G3D_FIX_ONE))
#define G3D_FIX_TO_INT(x)   ((int)((x) / G3D_FIX_ONE))

/* Helpers opcionales de borde-API. El core no depende de float. */
#define G3D_FIX_FROM_FLOAT(f) ((g3d_fix)((f) * (float)G3D_FIX_ONE))
#define G3D_FIX_TO_FLOAT(x)   ((float)(x) / (float)G3D_FIX_ONE)

/* Constantes de ángulo en radianes (Q20.12, redondeadas al entero más cercano). */
#define G3D_FIX_PI            ((g3d_fix)12868L)
#define G3D_FIX_TWO_PI        ((g3d_fix)25736L)
#define G3D_FIX_HALF_PI       ((g3d_fix)6434L)

/* ------------------------------------------------------------------------- */
/*  Operaciones básicas fixed                                                */
/* ------------------------------------------------------------------------- */

g3d_fix g3d_fix_add_sat(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_sub_sat(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_neg_sat(g3d_fix x);

g3d_fix g3d_fix_mul(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_div(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_sqrt(g3d_fix x);

g3d_fix g3d_fix_abs(g3d_fix x);
g3d_fix g3d_fix_min(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_max(g3d_fix a, g3d_fix b);
g3d_fix g3d_fix_clamp(g3d_fix v, g3d_fix minv, g3d_fix maxv);

/* ------------------------------------------------------------------------- */
/*  Trigonometría en fixed                                                   */
/*  Implementación sin math.h: LUT seno, cos derivado y tan = sin/cos.       */
/* ------------------------------------------------------------------------- */

g3d_fix gamlib_deg2rad(g3d_fix deg); /* deg (Q20.12) -> rad (Q20.12) */
g3d_fix gamlib_rad2deg(g3d_fix rad); /* rad (Q20.12) -> deg (Q20.12) */

g3d_fix gamlib_sin(g3d_fix rad);
g3d_fix gamlib_cos(g3d_fix rad);
g3d_fix gamlib_tan(g3d_fix rad);

/* ------------------------------------------------------------------------- */
/*  Vec3 en fixed                                                            */
/* ------------------------------------------------------------------------- */

typedef struct Vec3 {
    g3d_fix x;
    g3d_fix y;
    g3d_fix z;
} Vec3;

Vec3    gamlib_vec3          (g3d_fix x, g3d_fix y, g3d_fix z);
void    gamlib_vec3_set      (Vec3* v, g3d_fix x, g3d_fix y, g3d_fix z);
void    gamlib_vec3_add      (Vec3* out, const Vec3* a, const Vec3* b);
void    gamlib_vec3_sub      (Vec3* out, const Vec3* a, const Vec3* b);
void    gamlib_vec3_scale    (Vec3* out, const Vec3* v, g3d_fix s);
g3d_fix gamlib_vec3_dot      (const Vec3* a, const Vec3* b);
void    gamlib_vec3_cross    (Vec3* out, const Vec3* a, const Vec3* b);
g3d_fix gamlib_vec3_length   (const Vec3* v);
void    gamlib_vec3_normalize(Vec3* out, const Vec3* v);

/* ------------------------------------------------------------------------- */
/*  Helpers de ángulo                                                        */
/* ------------------------------------------------------------------------- */

g3d_fix g3d_fix_wrap_angle_deg(g3d_fix a);
g3d_fix g3d_fix_wrap_angle_rad(g3d_fix a);

#ifdef __cplusplus
}
#endif

#endif /* GAMLIB3D_MATH_H */
