/* ============================================================
 * SICOL - Fixed Point Math
 * ============================================================ */

#include "sicol_math_fx.h"

static uint64_t fx_isqrt_u64(uint64_t x)
{
    uint64_t op = x;
    uint64_t res = 0;
    uint64_t one = (uint64_t)1 << 62;

    while (one > op) {
        one >>= 2;
    }

    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res = res + (one << 1);
        }
        res >>= 1;
        one >>= 2;
    }

    return res;
}

fx fx_clamp(fx v, fx min_value, fx max_value)
{
    if (v < min_value) return min_value;
    if (v > max_value) return max_value;
    return v;
}

fx fx_sqrt(fx v)
{
    uint64_t n;
    if (v <= 0) return 0;
    n = ((uint64_t)(uint32_t)v) << FX_SHIFT;
    return (fx)fx_isqrt_u64(n);
}

void fx_zero3(fx out[3])
{
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
}

void fx_set3(fx out[3], fx x, fx y, fx z)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void fx_copy3(fx out[3], const fx v[3])
{
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
}

fx fx_dot3(const fx a[3], const fx b[3])
{
    fx r = 0;
    r += FX_MUL(a[0], b[0]);
    r += FX_MUL(a[1], b[1]);
    r += FX_MUL(a[2], b[2]);
    return r;
}

void fx_cross3(fx out[3], const fx a[3], const fx b[3])
{
    fx x = FX_MUL(a[1], b[2]) - FX_MUL(a[2], b[1]);
    fx y = FX_MUL(a[2], b[0]) - FX_MUL(a[0], b[2]);
    fx z = FX_MUL(a[0], b[1]) - FX_MUL(a[1], b[0]);
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void fx_add3(fx out[3], const fx a[3], const fx b[3])
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

void fx_sub3(fx out[3], const fx a[3], const fx b[3])
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

void fx_neg3(fx out[3], const fx v[3])
{
    out[0] = -v[0];
    out[1] = -v[1];
    out[2] = -v[2];
}

void fx_scale3(fx out[3], const fx v[3], fx s)
{
    out[0] = FX_MUL(v[0], s);
    out[1] = FX_MUL(v[1], s);
    out[2] = FX_MUL(v[2], s);
}

void fx_madd3(fx out[3], const fx a[3], const fx b[3], fx s)
{
    out[0] = a[0] + FX_MUL(b[0], s);
    out[1] = a[1] + FX_MUL(b[1], s);
    out[2] = a[2] + FX_MUL(b[2], s);
}

fx fx_len_sq3(const fx v[3])
{
    return fx_dot3(v, v);
}

fx fx_len3(const fx v[3])
{
    return fx_sqrt(fx_len_sq3(v));
}

fx fx_distance_sq3(const fx a[3], const fx b[3])
{
    fx d[3];
    fx_sub3(d, a, b);
    return fx_len_sq3(d);
}

fx fx_distance3(const fx a[3], const fx b[3])
{
    return fx_sqrt(fx_distance_sq3(a, b));
}

int fx_normalize3(fx out[3], const fx v[3])
{
    fx len = fx_len3(v);
    if (len <= FX_EPSILON) {
        fx_zero3(out);
        return 0;
    }

    out[0] = FX_DIV(v[0], len);
    out[1] = FX_DIV(v[1], len);
    out[2] = FX_DIV(v[2], len);
    return 1;
}

void fx_identity_mat3(fx out[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            out[i][j] = (i == j) ? FX_ONE : 0;
        }
    }
}

void fx_transpose_mat3(fx out[3][3], const fx in_m[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            out[j][i] = in_m[i][j];
        }
    }
}

void fx_mul_mat3_vec3(fx out[3], const fx m[3][3], const fx v[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        out[i] = FX_MUL(m[i][0], v[0]) +
                 FX_MUL(m[i][1], v[1]) +
                 FX_MUL(m[i][2], v[2]);
    }
}

void fx_basis_to_local3(fx out[3], const fx basis[3][3], const fx v[3])
{
    out[0] = fx_dot3(basis[0], v);
    out[1] = fx_dot3(basis[1], v);
    out[2] = fx_dot3(basis[2], v);
}

void fx_basis_to_world3(fx out[3], const fx basis[3][3], const fx v[3])
{
    fx tmp0[3];
    fx tmp1[3];
    fx tmp2[3];
    fx_scale3(tmp0, basis[0], v[0]);
    fx_scale3(tmp1, basis[1], v[1]);
    fx_scale3(tmp2, basis[2], v[2]);
    fx_add3(out, tmp0, tmp1);
    fx_add3(out, out, tmp2);
}

fx fx_closest_point_segment3(
    fx out[3],
    const fx a[3],
    const fx b[3],
    const fx p[3]
)
{
    fx ab[3];
    fx ap[3];
    fx denom;
    fx t;

    fx_sub3(ab, b, a);
    fx_sub3(ap, p, a);

    denom = fx_len_sq3(ab);
    if (denom <= FX_EPSILON) {
        fx_copy3(out, a);
        return 0;
    }

    t = FX_DIV(fx_dot3(ap, ab), denom);
    t = fx_clamp(t, 0, FX_ONE);

    fx_madd3(out, a, ab, t);
    return t;
}

void fx_closest_points_segment_segment3(
    fx out_a[3],
    fx out_b[3],
    const fx p1[3],
    const fx q1[3],
    const fx p2[3],
    const fx q2[3],
    fx* s_out,
    fx* t_out
)
{
    fx d1[3];
    fx d2[3];
    fx r[3];
    fx a;
    fx e;
    fx f;
    fx c;
    fx b;
    fx denom;
    fx s = 0;
    fx t = 0;

    fx_sub3(d1, q1, p1);
    fx_sub3(d2, q2, p2);
    fx_sub3(r, p1, p2);

    a = fx_dot3(d1, d1);
    e = fx_dot3(d2, d2);
    f = fx_dot3(d2, r);

    if (a <= FX_EPSILON && e <= FX_EPSILON) {
        fx_copy3(out_a, p1);
        fx_copy3(out_b, p2);
        if (s_out) *s_out = 0;
        if (t_out) *t_out = 0;
        return;
    }

    if (a <= FX_EPSILON) {
        s = 0;
        t = fx_clamp(FX_DIV(f, e), 0, FX_ONE);
    } else {
        c = fx_dot3(d1, r);

        if (e <= FX_EPSILON) {
            t = 0;
            s = fx_clamp(-FX_DIV(c, a), 0, FX_ONE);
        } else {
            b = fx_dot3(d1, d2);
            denom = a - FX_MUL(b, FX_DIV(b, e));

            if (FX_ABS(denom) > FX_EPSILON) {
                s = fx_clamp(FX_DIV(FX_MUL(b, FX_DIV(f, e)) - c, denom), 0, FX_ONE);
            } else {
                s = 0;
            }

            t = FX_DIV(FX_MUL(b, s) + f, e);

            if (t < 0) {
                t = 0;
                s = fx_clamp(-FX_DIV(c, a), 0, FX_ONE);
            } else if (t > FX_ONE) {
                t = FX_ONE;
                s = fx_clamp(FX_DIV(b - c, a), 0, FX_ONE);
            }
        }
    }

    fx_madd3(out_a, p1, d1, s);
    fx_madd3(out_b, p2, d2, t);

    if (s_out) *s_out = s;
    if (t_out) *t_out = t;
}
