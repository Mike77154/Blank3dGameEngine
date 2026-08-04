/* gamlib3d_matrix.h - Matrices 4x4 fixed columna-major para gamlib3d */
#ifndef GAMLIB3D_MATRIX_H
#define GAMLIB3D_MATRIX_H

#include "gamlib3d_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/*  Matrices 4x4 columna-major                                               */
/*  Convención espacial: sistema diestro, +Y arriba, -Z hacia adelante.      */
/* ------------------------------------------------------------------------- */

void gamlib_mat4_identity(g3d_fix m[16]);
void gamlib_mat4_copy(g3d_fix dst[16], const g3d_fix src[16]);

/* out = a * b (alias-safe) */
void gamlib_mat4_mul(const g3d_fix a[16], const g3d_fix b[16], g3d_fix out[16]);

void gamlib_mat4_translation(g3d_fix m[16],
                             g3d_fix tx, g3d_fix ty, g3d_fix tz);

void gamlib_mat4_scale(g3d_fix m[16],
                       g3d_fix sx, g3d_fix sy, g3d_fix sz);

/* Rotaciones: ángulos en radianes Q20.12 */
void gamlib_mat4_rotation_x(g3d_fix m[16], g3d_fix angle_rad);
void gamlib_mat4_rotation_y(g3d_fix m[16], g3d_fix angle_rad);
void gamlib_mat4_rotation_z(g3d_fix m[16], g3d_fix angle_rad);

/* Proyección perspectiva (estilo OpenGL, columna-major).
   Si los parámetros son degenerados, devuelve identidad. */
void gamlib_mat4_perspective(g3d_fix out[16],
                             g3d_fix fov_y_deg,
                             g3d_fix aspect,
                             g3d_fix z_near,
                             g3d_fix z_far);

/* Proyección ortográfica (estilo OpenGL).
   Si los parámetros son degenerados, devuelve identidad. */
void gamlib_mat4_ortho(g3d_fix out[16],
                       g3d_fix left,   g3d_fix right,
                       g3d_fix bottom, g3d_fix top,
                       g3d_fix z_near, g3d_fix z_far);

/* Inversión afín 4x4 (asumiendo [A t; 0 1]). Retorna 1 si pudo, 0 si no. */
int gamlib_mat4_invert_affine(const g3d_fix m[16], g3d_fix out[16]);

/* LookAt coherente con la convención de cámara usada por camera_build_view_matrix.
   La cámara mira hacia target con +Y arriba por defecto y -Z como forward local. */
void gamlib_mat4_lookat(g3d_fix out[16],
                        const Vec3* eye,
                        const Vec3* target,
                        const Vec3* up);

#ifdef __cplusplus
}
#endif

#endif /* GAMLIB3D_MATRIX_H */
