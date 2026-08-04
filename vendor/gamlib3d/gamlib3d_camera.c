/* gamlib3d_camera.c - Implementación núcleo de cámara agnóstica para juegos 3D */

#include "gamlib3d_camera.h"
#include "math_helpers/gamlib3d_math.h"
#include "math_helpers/gamlib3d_matrix.h"

void camera_init_perspective(Camera* cam,
                             g3d_scalar fov_y_deg,
                             g3d_scalar aspect,
                             g3d_scalar z_near,
                             g3d_scalar z_far)
{
    if (!cam) {
        return;
    }

    transform_init(&cam->transform);
    cam->proj_type = CAMERA_PROJ_PERSPECTIVE;

    cam->fov_y_deg = fov_y_deg;
    cam->aspect = aspect;
    cam->z_near = z_near;
    cam->z_far = z_far;

    cam->ortho_left = (g3d_scalar)-G3D_FIX_ONE;
    cam->ortho_right = (g3d_scalar)G3D_FIX_ONE;
    cam->ortho_bottom = (g3d_scalar)-G3D_FIX_ONE;
    cam->ortho_top = (g3d_scalar)G3D_FIX_ONE;
    cam->ortho_near = g3d_fix_div(G3D_FIX_ONE, G3D_FIX_FROM_INT(10));
    cam->ortho_far = G3D_FIX_FROM_INT(10);
}

void camera_init_ortho(Camera* cam,
                       g3d_scalar left,
                       g3d_scalar right,
                       g3d_scalar bottom,
                       g3d_scalar top,
                       g3d_scalar z_near,
                       g3d_scalar z_far)
{
    if (!cam) {
        return;
    }

    transform_init(&cam->transform);
    cam->proj_type = CAMERA_PROJ_ORTHO;

    cam->ortho_left = left;
    cam->ortho_right = right;
    cam->ortho_bottom = bottom;
    cam->ortho_top = top;
    cam->ortho_near = z_near;
    cam->ortho_far = z_far;

    cam->fov_y_deg = G3D_FIX_FROM_INT(60);
    cam->aspect = g3d_fix_div(G3D_FIX_FROM_INT(16), G3D_FIX_FROM_INT(9));
    cam->z_near = g3d_fix_div(G3D_FIX_ONE, G3D_FIX_FROM_INT(10));
    cam->z_far = G3D_FIX_FROM_INT(10);
}

void camera_set_aspect(Camera* cam, g3d_scalar aspect)
{
    if (!cam) {
        return;
    }
    cam->aspect = aspect;
}

Transform* camera_get_transform(Camera* cam)
{
    if (!cam) {
        return 0;
    }
    return &cam->transform;
}

const Transform* camera_get_transform_const(const Camera* cam)
{
    if (!cam) {
        return 0;
    }
    return &cam->transform;
}

void camera_build_view_matrix(const Camera* cam, g3d_scalar out_view[16])
{
    g3d_scalar world[16];

    if (!cam || !out_view) {
        return;
    }

    transform_to_matrix4(&cam->transform, world);

    if (!gamlib_mat4_invert_affine(world, out_view)) {
        gamlib_mat4_identity(out_view);
    }
}

void camera_build_proj_matrix(const Camera* cam, g3d_scalar out_proj[16])
{
    if (!cam || !out_proj) {
        return;
    }

    if (cam->proj_type == CAMERA_PROJ_PERSPECTIVE) {
        gamlib_mat4_perspective(out_proj,
                                cam->fov_y_deg,
                                cam->aspect,
                                cam->z_near,
                                cam->z_far);
    } else {
        gamlib_mat4_ortho(out_proj,
                          cam->ortho_left,
                          cam->ortho_right,
                          cam->ortho_bottom,
                          cam->ortho_top,
                          cam->ortho_near,
                          cam->ortho_far);
    }
}

void camera_build_viewproj_matrix(const Camera* cam, g3d_scalar out_viewproj[16])
{
    g3d_scalar view[16];
    g3d_scalar proj[16];

    if (!cam || !out_viewproj) {
        return;
    }

    camera_build_view_matrix(cam, view);
    camera_build_proj_matrix(cam, proj);
    gamlib_mat4_mul(proj, view, out_viewproj);
}
