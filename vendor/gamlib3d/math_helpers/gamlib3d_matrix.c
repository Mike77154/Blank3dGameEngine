/* gamlib3d_matrix.c - Implementación de matrices 4x4 fixed (columna-major) */

#include "gamlib3d_matrix.h"

static void gamlib_mat4_build_axes_translation(g3d_fix out[16],
                                               const Vec3* right,
                                               const Vec3* up,
                                               const Vec3* back,
                                               const Vec3* position)
{
    if (!out || !right || !up || !back || !position) {
        return;
    }

    out[0] = right->x;
    out[1] = right->y;
    out[2] = right->z;
    out[3] = 0;

    out[4] = up->x;
    out[5] = up->y;
    out[6] = up->z;
    out[7] = 0;

    out[8] = back->x;
    out[9] = back->y;
    out[10] = back->z;
    out[11] = 0;

    out[12] = position->x;
    out[13] = position->y;
    out[14] = position->z;
    out[15] = G3D_FIX_ONE;
}

void gamlib_mat4_identity(g3d_fix m[16])
{
    int i;

    if (!m) {
        return;
    }

    for (i = 0; i < 16; ++i) {
        m[i] = 0;
    }
    m[0] = G3D_FIX_ONE;
    m[5] = G3D_FIX_ONE;
    m[10] = G3D_FIX_ONE;
    m[15] = G3D_FIX_ONE;
}

void gamlib_mat4_copy(g3d_fix dst[16], const g3d_fix src[16])
{
    int i;

    if (!dst || !src) {
        return;
    }

    for (i = 0; i < 16; ++i) {
        dst[i] = src[i];
    }
}

void gamlib_mat4_mul(const g3d_fix a[16], const g3d_fix b[16], g3d_fix out[16])
{
    g3d_fix tmp[16];
    int col;
    int row;
    int k;

    if (!a || !b || !out) {
        return;
    }

    for (col = 0; col < 4; ++col) {
        for (row = 0; row < 4; ++row) {
            g3d_fix sum;
            sum = 0;

            for (k = 0; k < 4; ++k) {
                g3d_fix p;
                p = g3d_fix_mul(a[row + k * 4], b[k + col * 4]);
                sum = g3d_fix_add_sat(sum, p);
            }

            tmp[row + col * 4] = sum;
        }
    }

    gamlib_mat4_copy(out, tmp);
}

void gamlib_mat4_translation(g3d_fix m[16],
                             g3d_fix tx, g3d_fix ty, g3d_fix tz)
{
    gamlib_mat4_identity(m);
    if (!m) {
        return;
    }
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

void gamlib_mat4_scale(g3d_fix m[16],
                       g3d_fix sx, g3d_fix sy, g3d_fix sz)
{
    gamlib_mat4_identity(m);
    if (!m) {
        return;
    }
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

void gamlib_mat4_rotation_x(g3d_fix m[16], g3d_fix angle_rad)
{
    g3d_fix c;
    g3d_fix s;

    if (!m) {
        return;
    }

    c = gamlib_cos(angle_rad);
    s = gamlib_sin(angle_rad);

    gamlib_mat4_identity(m);
    m[5] = c;
    m[6] = s;
    m[9] = g3d_fix_neg_sat(s);
    m[10] = c;
}

void gamlib_mat4_rotation_y(g3d_fix m[16], g3d_fix angle_rad)
{
    g3d_fix c;
    g3d_fix s;

    if (!m) {
        return;
    }

    c = gamlib_cos(angle_rad);
    s = gamlib_sin(angle_rad);

    gamlib_mat4_identity(m);
    m[0] = c;
    m[2] = g3d_fix_neg_sat(s);
    m[8] = s;
    m[10] = c;
}

void gamlib_mat4_rotation_z(g3d_fix m[16], g3d_fix angle_rad)
{
    g3d_fix c;
    g3d_fix s;

    if (!m) {
        return;
    }

    c = gamlib_cos(angle_rad);
    s = gamlib_sin(angle_rad);

    gamlib_mat4_identity(m);
    m[0] = c;
    m[1] = s;
    m[4] = g3d_fix_neg_sat(s);
    m[5] = c;
}

void gamlib_mat4_perspective(g3d_fix out[16],
                             g3d_fix fov_y_deg,
                             g3d_fix aspect,
                             g3d_fix z_near,
                             g3d_fix z_far)
{
    g3d_fix half_fov;
    g3d_fix tan_half;
    g3d_fix f;
    g3d_fix num10;
    g3d_fix den;
    g3d_fix zf_zn;
    g3d_fix num14;

    if (!out) {
        return;
    }

    gamlib_mat4_identity(out);

    if (aspect <= 0) {
        return;
    }
    if (z_near <= 0) {
        return;
    }
    if (z_far <= z_near) {
        return;
    }
    if (fov_y_deg <= 0) {
        return;
    }
    if (fov_y_deg >= G3D_FIX_FROM_INT(179)) {
        return;
    }

    half_fov = gamlib_deg2rad(fov_y_deg) >> 1;
    tan_half = gamlib_tan(half_fov);
    if (g3d_fix_abs(tan_half) <= G3D_FIX_EPSILON) {
        return;
    }

    f = g3d_fix_div(G3D_FIX_ONE, tan_half);
    num10 = g3d_fix_add_sat(z_far, z_near);
    den = g3d_fix_sub_sat(z_near, z_far);
    zf_zn = g3d_fix_mul(z_far, z_near);
    num14 = g3d_fix_mul(G3D_FIX_FROM_INT(2), zf_zn);

    out[0] = g3d_fix_div(f, aspect);
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;

    out[4] = 0;
    out[5] = f;
    out[6] = 0;
    out[7] = 0;

    out[8] = 0;
    out[9] = 0;
    out[10] = g3d_fix_div(num10, den);
    out[11] = g3d_fix_neg_sat(G3D_FIX_ONE);

    out[12] = 0;
    out[13] = 0;
    out[14] = g3d_fix_div(num14, den);
    out[15] = 0;
}

void gamlib_mat4_ortho(g3d_fix out[16],
                       g3d_fix left,   g3d_fix right,
                       g3d_fix bottom, g3d_fix top,
                       g3d_fix z_near, g3d_fix z_far)
{
    g3d_fix rl;
    g3d_fix tb;
    g3d_fix fn;

    if (!out) {
        return;
    }

    gamlib_mat4_identity(out);

    rl = g3d_fix_sub_sat(right, left);
    tb = g3d_fix_sub_sat(top, bottom);
    fn = g3d_fix_sub_sat(z_far, z_near);

    if (g3d_fix_abs(rl) <= G3D_FIX_EPSILON) {
        return;
    }
    if (g3d_fix_abs(tb) <= G3D_FIX_EPSILON) {
        return;
    }
    if (g3d_fix_abs(fn) <= G3D_FIX_EPSILON) {
        return;
    }

    out[0] = g3d_fix_div(G3D_FIX_FROM_INT(2), rl);
    out[5] = g3d_fix_div(G3D_FIX_FROM_INT(2), tb);
    out[10] = g3d_fix_div(g3d_fix_neg_sat(G3D_FIX_FROM_INT(2)), fn);

    out[12] = g3d_fix_div(g3d_fix_neg_sat(g3d_fix_add_sat(right, left)), rl);
    out[13] = g3d_fix_div(g3d_fix_neg_sat(g3d_fix_add_sat(top, bottom)), tb);
    out[14] = g3d_fix_div(g3d_fix_neg_sat(g3d_fix_add_sat(z_far, z_near)), fn);
    out[15] = G3D_FIX_ONE;
}

int gamlib_mat4_invert_affine(const g3d_fix m[16], g3d_fix out[16])
{
    g3d_fix a00;
    g3d_fix a01;
    g3d_fix a02;
    g3d_fix a10;
    g3d_fix a11;
    g3d_fix a12;
    g3d_fix a20;
    g3d_fix a21;
    g3d_fix a22;
    g3d_fix t0;
    g3d_fix t1;
    g3d_fix t2;
    g3d_fix term1;
    g3d_fix term2;
    g3d_fix term3;
    g3d_fix det;
    g3d_fix det_inv;
    g3d_fix i00;
    g3d_fix i01;
    g3d_fix i02;
    g3d_fix i10;
    g3d_fix i11;
    g3d_fix i12;
    g3d_fix i20;
    g3d_fix i21;
    g3d_fix i22;
    g3d_fix v0;
    g3d_fix v1;
    g3d_fix v2;
    g3d_fix tmp;

    if (!m || !out) {
        return 0;
    }

    if ((m[3] != 0) || (m[7] != 0) || (m[11] != 0) || (m[15] != G3D_FIX_ONE)) {
        return 0;
    }

    a00 = m[0];  a01 = m[4];  a02 = m[8];
    a10 = m[1];  a11 = m[5];  a12 = m[9];
    a20 = m[2];  a21 = m[6];  a22 = m[10];

    t0 = m[12];
    t1 = m[13];
    t2 = m[14];

    term1 = g3d_fix_mul(a00,
                        g3d_fix_sub_sat(g3d_fix_mul(a11, a22),
                                        g3d_fix_mul(a12, a21)));
    term2 = g3d_fix_mul(a01,
                        g3d_fix_sub_sat(g3d_fix_mul(a10, a22),
                                        g3d_fix_mul(a12, a20)));
    term3 = g3d_fix_mul(a02,
                        g3d_fix_sub_sat(g3d_fix_mul(a10, a21),
                                        g3d_fix_mul(a11, a20)));

    det = g3d_fix_add_sat(g3d_fix_sub_sat(term1, term2), term3);
    if (g3d_fix_abs(det) <= G3D_FIX_EPSILON) {
        return 0;
    }

    det_inv = g3d_fix_div(G3D_FIX_ONE, det);

    i00 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a11, a22), g3d_fix_mul(a12, a21)), det_inv);
    i01 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a02, a21), g3d_fix_mul(a01, a22)), det_inv);
    i02 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a01, a12), g3d_fix_mul(a02, a11)), det_inv);

    i10 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a12, a20), g3d_fix_mul(a10, a22)), det_inv);
    i11 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a00, a22), g3d_fix_mul(a02, a20)), det_inv);
    i12 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a02, a10), g3d_fix_mul(a00, a12)), det_inv);

    i20 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a10, a21), g3d_fix_mul(a11, a20)), det_inv);
    i21 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a01, a20), g3d_fix_mul(a00, a21)), det_inv);
    i22 = g3d_fix_mul(g3d_fix_sub_sat(g3d_fix_mul(a00, a11), g3d_fix_mul(a01, a10)), det_inv);

    tmp = g3d_fix_add_sat(g3d_fix_mul(i00, t0), g3d_fix_mul(i01, t1));
    tmp = g3d_fix_add_sat(tmp, g3d_fix_mul(i02, t2));
    v0 = g3d_fix_neg_sat(tmp);

    tmp = g3d_fix_add_sat(g3d_fix_mul(i10, t0), g3d_fix_mul(i11, t1));
    tmp = g3d_fix_add_sat(tmp, g3d_fix_mul(i12, t2));
    v1 = g3d_fix_neg_sat(tmp);

    tmp = g3d_fix_add_sat(g3d_fix_mul(i20, t0), g3d_fix_mul(i21, t1));
    tmp = g3d_fix_add_sat(tmp, g3d_fix_mul(i22, t2));
    v2 = g3d_fix_neg_sat(tmp);

    out[0] = i00; out[4] = i01; out[8] = i02; out[12] = v0;
    out[1] = i10; out[5] = i11; out[9] = i12; out[13] = v1;
    out[2] = i20; out[6] = i21; out[10] = i22; out[14] = v2;
    out[3] = 0;
    out[7] = 0;
    out[11] = 0;
    out[15] = G3D_FIX_ONE;

    return 1;
}

void gamlib_mat4_lookat(g3d_fix out[16],
                        const Vec3* eye,
                        const Vec3* target,
                        const Vec3* up)
{
    Vec3 forward;
    Vec3 world_up;
    Vec3 right;
    Vec3 real_up;
    Vec3 back;
    Vec3 tmp;
    Vec3 alt_up;
    g3d_fix world[16];

    if (!out || !eye || !target || !up) {
        return;
    }

    gamlib_vec3_sub(&tmp, target, eye);
    gamlib_vec3_normalize(&forward, &tmp);
    if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON) {
        forward = gamlib_vec3(G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(0),
                              G3D_FIX_FROM_INT(-1));
    }

    gamlib_vec3_normalize(&world_up, up);
    if (gamlib_vec3_length(&world_up) <= G3D_FIX_EPSILON) {
        world_up = gamlib_vec3(G3D_FIX_FROM_INT(0),
                               G3D_FIX_FROM_INT(1),
                               G3D_FIX_FROM_INT(0));
    }

    gamlib_vec3_cross(&right, &forward, &world_up);
    gamlib_vec3_normalize(&right, &right);
    if (gamlib_vec3_length(&right) <= G3D_FIX_EPSILON) {
        if (g3d_fix_abs(forward.y) < G3D_FIX_FROM_INT(1)) {
            alt_up = gamlib_vec3(G3D_FIX_FROM_INT(0),
                                 G3D_FIX_FROM_INT(1),
                                 G3D_FIX_FROM_INT(0));
        } else {
            alt_up = gamlib_vec3(G3D_FIX_FROM_INT(0),
                                 G3D_FIX_FROM_INT(0),
                                 G3D_FIX_FROM_INT(1));
        }

        gamlib_vec3_cross(&right, &forward, &alt_up);
        gamlib_vec3_normalize(&right, &right);
    }

    gamlib_vec3_cross(&real_up, &right, &forward);
    gamlib_vec3_normalize(&real_up, &real_up);

    back = gamlib_vec3(g3d_fix_neg_sat(forward.x),
                       g3d_fix_neg_sat(forward.y),
                       g3d_fix_neg_sat(forward.z));

    gamlib_mat4_build_axes_translation(world, &right, &real_up, &back, eye);
    if (!gamlib_mat4_invert_affine(world, out)) {
        gamlib_mat4_identity(out);
    }
}
