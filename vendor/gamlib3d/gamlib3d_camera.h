/* gamlib3d_camera.h - Núcleo de cámara agnóstica de gamlib3d */
#ifndef GAMLIB3D_CAMERA_H
#define GAMLIB3D_CAMERA_H

#include "gamlib3d_scalar.h"
#include "gamlib3d_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAMERA_PROJ_PERSPECTIVE = 0,
    CAMERA_PROJ_ORTHO       = 1
} CameraProjectionType;

typedef struct Camera {
    Transform            transform;   /* posición/rotación de la cámara */
    CameraProjectionType proj_type;

    /* Parámetros de perspectiva */
    g3d_scalar fov_y_deg;  /* campo de visión vertical en grados (Q20.12) */
    g3d_scalar aspect;     /* width / height */
    g3d_scalar z_near;
    g3d_scalar z_far;

    /* Parámetros de proyección ortográfica */
    g3d_scalar ortho_left;
    g3d_scalar ortho_right;
    g3d_scalar ortho_bottom;
    g3d_scalar ortho_top;
    g3d_scalar ortho_near;
    g3d_scalar ortho_far;
} Camera;

void camera_init_perspective(Camera* cam,
                             g3d_scalar fov_y_deg,
                             g3d_scalar aspect,
                             g3d_scalar z_near,
                             g3d_scalar z_far);

void camera_init_ortho(Camera* cam,
                       g3d_scalar left,
                       g3d_scalar right,
                       g3d_scalar bottom,
                       g3d_scalar top,
                       g3d_scalar z_near,
                       g3d_scalar z_far);

void camera_set_aspect(Camera* cam,
                       g3d_scalar aspect);

Transform*       camera_get_transform      (Camera* cam);
const Transform* camera_get_transform_const(const Camera* cam);

void camera_build_view_matrix(const Camera* cam,
                              g3d_scalar out_view[16]);

void camera_build_proj_matrix(const Camera* cam,
                              g3d_scalar out_proj[16]);

void camera_build_viewproj_matrix(const Camera* cam,
                                  g3d_scalar out_viewproj[16]);

#ifdef __cplusplus
}
#endif

#endif /* GAMLIB3D_CAMERA_H */
