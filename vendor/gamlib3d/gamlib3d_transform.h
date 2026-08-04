#ifndef GAMLIB3D_TRANSFORM_H
#define GAMLIB3D_TRANSFORM_H

#include "math_helpers/gamlib3d_math.h"
#include "math_helpers/gamlib3d_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Transform {
    Vec3 position;   /* posición global */
    Vec3 rotation;   /* rot.x=pitch, rot.y=yaw, rot.z=roll en grados Q20.12 */
    Vec3 scale;      /* escala por eje */
} Transform;

/* Inicializa en identidad: pos=0, rot=0, scale=1 */
void transform_init(Transform* t);

/* Rota en grados sumando (dyaw, dpitch, droll). */
void transform_rotate(Transform* t,
                      g3d_fix dyaw_deg,
                      g3d_fix dpitch_deg,
                      g3d_fix droll_deg);

/* Devuelve ejes locales ortonormalizados según la convención del transform.
   forward apunta hacia -Z cuando la rotación es identidad. */
void transform_get_local_axes(const Transform* t,
                              Vec3* out_right,
                              Vec3* out_up,
                              Vec3* out_forward);

/* Movimiento local completo (6DOF): usa right/up/forward reales del transform. */
void transform_move_local(Transform* t,
                          g3d_fix dx,
                          g3d_fix dy,
                          g3d_fix dz);

/* Variante de conveniencia para movimiento plano estilo FPS:
   usa world-up fijo, ignora roll y aplana el forward sobre XZ. */
void transform_move_local_flat(Transform* t,
                               g3d_fix dx,
                               g3d_fix dy,
                               g3d_fix dz);

/* Helpers de pitch usados por rotation_clamp_pitch */
g3d_fix transform_get_pitch_deg(const Transform* t);
void    transform_set_pitch_deg(Transform* t, g3d_fix pitch_deg);

/* Convierte el Transform en matriz 4×4 columna-major */
void transform_to_matrix4(const Transform* t, g3d_fix out[16]);

#ifdef __cplusplus
}
#endif

#endif /* GAMLIB3D_TRANSFORM_H */
