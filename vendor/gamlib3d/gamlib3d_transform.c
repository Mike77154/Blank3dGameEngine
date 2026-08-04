#include "gamlib3d_transform.h"

static void transform_build_rotation_matrix(const Transform* t, g3d_fix out[16])
{
    g3d_fix rx;
    g3d_fix ry;
    g3d_fix rz;
    g3d_fix mx[16];
    g3d_fix my[16];
    g3d_fix mz[16];
    g3d_fix mtemp[16];

    if (!t || !out) {
        return;
    }

    rx = gamlib_deg2rad(t->rotation.x);
    ry = gamlib_deg2rad(t->rotation.y);
    rz = gamlib_deg2rad(t->rotation.z);

    gamlib_mat4_rotation_x(mx, rx);
    gamlib_mat4_rotation_y(my, ry);
    gamlib_mat4_rotation_z(mz, rz);

    gamlib_mat4_mul(my, mx, mtemp);
    gamlib_mat4_mul(mtemp, mz, out);
}

static void transform_accumulate_move(Transform* t,
                                      const Vec3* right,
                                      const Vec3* up,
                                      const Vec3* forward,
                                      g3d_fix dx,
                                      g3d_fix dy,
                                      g3d_fix dz)
{
    Vec3 tmp;
    Vec3 acc;

    if (!t || !right || !up || !forward) {
        return;
    }

    acc = t->position;

    gamlib_vec3_scale(&tmp, right, dx);
    gamlib_vec3_add(&acc, &acc, &tmp);

    gamlib_vec3_scale(&tmp, up, dy);
    gamlib_vec3_add(&acc, &acc, &tmp);

    gamlib_vec3_scale(&tmp, forward, dz);
    gamlib_vec3_add(&acc, &acc, &tmp);

    t->position = acc;
}

void transform_init(Transform* t)
{
    if (!t) {
        return;
    }

    gamlib_vec3_set(&t->position,
                    G3D_FIX_FROM_INT(0),
                    G3D_FIX_FROM_INT(0),
                    G3D_FIX_FROM_INT(0));

    t->rotation = gamlib_vec3(G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(0));

    t->scale = gamlib_vec3(G3D_FIX_FROM_INT(1),
                           G3D_FIX_FROM_INT(1),
                           G3D_FIX_FROM_INT(1));
}

void transform_rotate(Transform* t,
                      g3d_fix dyaw_deg,
                      g3d_fix dpitch_deg,
                      g3d_fix droll_deg)
{
    if (!t) {
        return;
    }

    t->rotation.y = g3d_fix_wrap_angle_deg(g3d_fix_add_sat(t->rotation.y, dyaw_deg));
    t->rotation.x = g3d_fix_wrap_angle_deg(g3d_fix_add_sat(t->rotation.x, dpitch_deg));
    t->rotation.z = g3d_fix_wrap_angle_deg(g3d_fix_add_sat(t->rotation.z, droll_deg));
}

void transform_get_local_axes(const Transform* t,
                              Vec3* out_right,
                              Vec3* out_up,
                              Vec3* out_forward)
{
    g3d_fix mrot[16];
    Vec3 right;
    Vec3 up;
    Vec3 forward;

    if (!t) {
        return;
    }

    transform_build_rotation_matrix(t, mrot);

    right = gamlib_vec3(mrot[0], mrot[1], mrot[2]);
    up = gamlib_vec3(mrot[4], mrot[5], mrot[6]);
    forward = gamlib_vec3(g3d_fix_neg_sat(mrot[8]),
                          g3d_fix_neg_sat(mrot[9]),
                          g3d_fix_neg_sat(mrot[10]));

    gamlib_vec3_normalize(&right, &right);
    gamlib_vec3_normalize(&up, &up);
    gamlib_vec3_normalize(&forward, &forward);

    if (out_right) {
        *out_right = right;
    }
    if (out_up) {
        *out_up = up;
    }
    if (out_forward) {
        *out_forward = forward;
    }
}

void transform_move_local(Transform* t,
                          g3d_fix dx,
                          g3d_fix dy,
                          g3d_fix dz)
{
    Vec3 right;
    Vec3 up;
    Vec3 forward;

    if (!t) {
        return;
    }

    transform_get_local_axes(t, &right, &up, &forward);
    transform_accumulate_move(t, &right, &up, &forward, dx, dy, dz);
}

void transform_move_local_flat(Transform* t,
                               g3d_fix dx,
                               g3d_fix dy,
                               g3d_fix dz)
{
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 world_up;

    if (!t) {
        return;
    }

    transform_get_local_axes(t, 0, 0, &forward);

    forward.y = 0;
    gamlib_vec3_normalize(&forward, &forward);
    if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON) {
        forward = gamlib_vec3(G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(-1));
    }

    world_up = gamlib_vec3(G3D_FIX_FROM_INT(0),
                           G3D_FIX_FROM_INT(1),
                           G3D_FIX_FROM_INT(0));

    gamlib_vec3_cross(&right, &forward, &world_up);
    gamlib_vec3_normalize(&right, &right);
    if (gamlib_vec3_length(&right) <= G3D_FIX_EPSILON) {
        right = gamlib_vec3(G3D_FIX_FROM_INT(1),
                            G3D_FIX_FROM_INT(0),
                            G3D_FIX_FROM_INT(0));
    }

    up = world_up;
    transform_accumulate_move(t, &right, &up, &forward, dx, dy, dz);
}

g3d_fix transform_get_pitch_deg(const Transform* t)
{
    return t ? t->rotation.x : 0;
}

void transform_set_pitch_deg(Transform* t, g3d_fix pitch_deg)
{
    if (!t) {
        return;
    }

    t->rotation.x = g3d_fix_wrap_angle_deg(pitch_deg);
}

void transform_to_matrix4(const Transform* t, g3d_fix out[16])
{
    g3d_fix mrot[16];
    g3d_fix mtemp[16];
    g3d_fix mscale[16];
    g3d_fix mtrans[16];

    if (!t || !out) {
        return;
    }

    transform_build_rotation_matrix(t, mrot);
    gamlib_mat4_scale(mscale, t->scale.x, t->scale.y, t->scale.z);
    gamlib_mat4_mul(mrot, mscale, mtemp);

    gamlib_mat4_translation(mtrans,
                            t->position.x,
                            t->position.y,
                            t->position.z);

    gamlib_mat4_mul(mtrans, mtemp, out);
}
