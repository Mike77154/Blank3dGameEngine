#include "gveh_math.h"

static const gveh_i16 gveh_sin_lut_64[65] = {
    0,25,50,75,100,125,150,174,199,223,247,271,294,318,341,363,
    386,408,429,450,471,492,512,531,550,568,586,604,621,637,653,668,
    683,697,710,724,736,748,759,770,780,789,798,806,814,821,827,833,
    838,843,847,850,853,856,858,859,860,860,860,859,858,856,853,850,846
};

gveh_fx gveh_fx_from_int(gveh_i32 v)
{
    return (gveh_fx)(v << GVEH_FX_SHIFT);
}

gveh_i32 gveh_fx_to_int(gveh_fx v)
{
    if (v >= 0) return (gveh_i32)(v >> GVEH_FX_SHIFT);
    return (gveh_i32)(-((-v) >> GVEH_FX_SHIFT));
}

gveh_fx gveh_fx_abs(gveh_fx v)
{
    if (v < 0) return -v;
    return v;
}

gveh_fx gveh_fx_clamp(gveh_fx v, gveh_fx lo, gveh_fx hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

gveh_fx gveh_fx_mul(gveh_fx a, gveh_fx b)
{
    gveh_i32 sign;
    gveh_u32 ua;
    gveh_u32 ub;
    gveh_u32 hi;
    gveh_u32 lo;
    gveh_i32 out;

    sign = 1;
    if (a < 0) { sign = -sign; a = -a; }
    if (b < 0) { sign = -sign; b = -b; }
    ua = (gveh_u32)a;
    ub = (gveh_u32)b;
    hi = (ua >> GVEH_FX_SHIFT) * ub;
    lo = ((ua & (GVEH_FX_ONE - 1)) * ub) >> GVEH_FX_SHIFT;
    out = (gveh_i32)(hi + lo);
    if (sign < 0) out = -out;
    return out;
}

gveh_fx gveh_fx_div(gveh_fx a, gveh_fx b)
{
    gveh_i32 sign;
    gveh_u32 ua;
    gveh_u32 ub;
    gveh_u32 q;
    gveh_u32 r;
    gveh_u32 out;
    gveh_i32 i;

    if (b == 0) return 0;
    sign = 1;
    if (a < 0) { sign = -sign; a = -a; }
    if (b < 0) { sign = -sign; b = -b; }
    ua = (gveh_u32)a;
    ub = (gveh_u32)b;
    q = ua / ub;
    r = ua % ub;
    out = q << GVEH_FX_SHIFT;
    i = 0;
    while (i < GVEH_FX_SHIFT) {
        r = r << 1;
        if (r >= ub) {
            r -= ub;
            out |= (1UL << (GVEH_FX_SHIFT - 1 - i));
        }
        i++;
    }
    if (sign < 0) return -(gveh_i32)out;
    return (gveh_i32)out;
}

gveh_fx gveh_fx_lerp(gveh_fx a, gveh_fx b, gveh_fx t)
{
    return a + gveh_fx_mul(b - a, t);
}

gveh_fx gveh_fx_sin256(gveh_i32 angle256)
{
    gveh_i32 a;
    gveh_i32 q;
    gveh_i32 idx;
    gveh_fx v;

    a = angle256 & 255;
    q = a >> 6;
    idx = a & 63;
    if (q == 0) v = (gveh_fx)gveh_sin_lut_64[idx];
    else if (q == 1) v = (gveh_fx)gveh_sin_lut_64[64 - idx];
    else if (q == 2) v = -(gveh_fx)gveh_sin_lut_64[idx];
    else v = -(gveh_fx)gveh_sin_lut_64[64 - idx];
    return (v * GVEH_FX_ONE) / 860;
}

gveh_fx gveh_fx_cos256(gveh_i32 angle256)
{
    return gveh_fx_sin256(angle256 + 64);
}

gveh_vec3 gveh_v3(gveh_fx x, gveh_fx y, gveh_fx z)
{
    gveh_vec3 r;
    r.x = x; r.y = y; r.z = z;
    return r;
}

gveh_vec3 gveh_v3_add(gveh_vec3 a, gveh_vec3 b)
{
    return gveh_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

gveh_vec3 gveh_v3_sub(gveh_vec3 a, gveh_vec3 b)
{
    return gveh_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

gveh_vec3 gveh_v3_scale(gveh_vec3 a, gveh_fx s)
{
    return gveh_v3(gveh_fx_mul(a.x, s), gveh_fx_mul(a.y, s), gveh_fx_mul(a.z, s));
}

gveh_fx gveh_v3_dot(gveh_vec3 a, gveh_vec3 b)
{
    return gveh_fx_mul(a.x, b.x) + gveh_fx_mul(a.y, b.y) + gveh_fx_mul(a.z, b.z);
}

gveh_vec3 gveh_v3_cross(gveh_vec3 a, gveh_vec3 b)
{
    gveh_vec3 r;
    r.x = gveh_fx_mul(a.y, b.z) - gveh_fx_mul(a.z, b.y);
    r.y = gveh_fx_mul(a.z, b.x) - gveh_fx_mul(a.x, b.z);
    r.z = gveh_fx_mul(a.x, b.y) - gveh_fx_mul(a.y, b.x);
    return r;
}

gveh_fx gveh_v3_len_approx(gveh_vec3 a)
{
    gveh_fx ax;
    gveh_fx ay;
    gveh_fx az;
    gveh_fx m1;
    gveh_fx m2;
    gveh_fx mn;

    ax = gveh_fx_abs(a.x);
    ay = gveh_fx_abs(a.y);
    az = gveh_fx_abs(a.z);
    m1 = ax;
    if (ay > m1) m1 = ay;
    if (az > m1) m1 = az;
    m2 = ax + ay + az - m1;
    mn = m2 >> 1;
    return m1 + mn;
}

gveh_vec3 gveh_v3_norm_approx(gveh_vec3 a)
{
    gveh_fx l;
    l = gveh_v3_len_approx(a);
    if (l <= GVEH_FX_EPS) return gveh_v3(0,0,0);
    return gveh_v3_scale(a, gveh_fx_div(GVEH_FX_ONE, l));
}

gveh_basis gveh_basis_yaw_pitch_roll(gveh_i32 yaw256, gveh_i32 pitch256, gveh_i32 roll256)
{
    gveh_basis b;
    gveh_vec3 base_right;
    gveh_vec3 base_up;
    gveh_fx cy;
    gveh_fx sy;
    gveh_fx cp;
    gveh_fx sp;
    gveh_fx cr;
    gveh_fx sr;

    cy = gveh_fx_cos256(yaw256);
    sy = gveh_fx_sin256(yaw256);
    cp = gveh_fx_cos256(pitch256);
    sp = gveh_fx_sin256(pitch256);
    cr = gveh_fx_cos256(roll256);
    sr = gveh_fx_sin256(roll256);

    b.fwd.x = gveh_fx_mul(sy, cp);
    b.fwd.y = sp;
    b.fwd.z = gveh_fx_mul(cy, cp);
    b.fwd = gveh_v3_norm_approx(b.fwd);

    base_right.x = cy;
    base_right.y = 0;
    base_right.z = -sy;
    base_right = gveh_v3_norm_approx(base_right);
    base_up = gveh_v3_cross(base_right, b.fwd);
    base_up = gveh_v3_norm_approx(base_up);

    /* Roll is a twist around local forward; this matters for aircraft/spacecraft. */
    b.right = gveh_v3_add(gveh_v3_scale(base_right, cr), gveh_v3_scale(base_up, sr));
    b.up = gveh_v3_sub(gveh_v3_scale(base_up, cr), gveh_v3_scale(base_right, sr));
    b.right = gveh_v3_norm_approx(b.right);
    b.up = gveh_v3_norm_approx(b.up);
    return b;
}

gveh_vec3 gveh_basis_local_to_world(gveh_basis b, gveh_vec3 local)
{
    gveh_vec3 r;
    r.x = gveh_fx_mul(b.right.x, local.x) + gveh_fx_mul(b.up.x, local.y) + gveh_fx_mul(b.fwd.x, local.z);
    r.y = gveh_fx_mul(b.right.y, local.x) + gveh_fx_mul(b.up.y, local.y) + gveh_fx_mul(b.fwd.y, local.z);
    r.z = gveh_fx_mul(b.right.z, local.x) + gveh_fx_mul(b.up.z, local.y) + gveh_fx_mul(b.fwd.z, local.z);
    return r;
}
