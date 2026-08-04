/* gamlib3d_math.c - Implementación núcleo matemático fixed-point Q20.12 */

#include "gamlib3d_math.h"

/* ------------------------------------------------------------------------- */
/*  Helpers internos de enteros                                              */
/* ------------------------------------------------------------------------- */

#define G3D_U32_MASK   0xFFFFFFFFUL
#define G3D_U16_MASK   0xFFFFUL
#define G3D_SIGN_BIT   0x80000000UL

typedef struct G3D_U64 {
    unsigned long hi;
    unsigned long lo;
} G3D_U64;

static unsigned long g3d_fix_abs_u32(g3d_fix x)
{
    if (x >= 0) {
        return ((unsigned long)x) & G3D_U32_MASK;
    }

    if (x == G3D_FIX_MIN) {
        return G3D_SIGN_BIT;
    }

    return ((unsigned long)(-x)) & G3D_U32_MASK;
}

static g3d_fix g3d_fix_from_u32_sat(unsigned long mag, int negative)
{
    mag &= G3D_U32_MASK;

    if (!negative) {
        if (mag > 0x7FFFFFFFUL) {
            return G3D_FIX_MAX;
        }
        return (g3d_fix)mag;
    }

    if (mag >= G3D_SIGN_BIT) {
        return G3D_FIX_MIN;
    }

    return (g3d_fix)(-(g3d_fix)mag);
}

static G3D_U64 g3d_u64_zero(void)
{
    G3D_U64 v;
    v.hi = 0UL;
    v.lo = 0UL;
    return v;
}

static void g3d_u64_add_u32(G3D_U64* v, unsigned long addend)
{
    unsigned long old_lo;
    unsigned long new_lo;
    unsigned long carry;

    old_lo = v->lo & G3D_U32_MASK;
    addend &= G3D_U32_MASK;
    new_lo = (old_lo + addend) & G3D_U32_MASK;
    carry = (new_lo < old_lo) ? 1UL : 0UL;

    v->lo = new_lo;
    v->hi = (v->hi + carry) & G3D_U32_MASK;
}

static G3D_U64 g3d_u64_mul_u32(unsigned long a, unsigned long b)
{
    unsigned long a0;
    unsigned long a1;
    unsigned long b0;
    unsigned long b1;
    unsigned long p0;
    unsigned long p1;
    unsigned long p2;
    unsigned long p3;
    unsigned long mid;
    G3D_U64 out;

    a &= G3D_U32_MASK;
    b &= G3D_U32_MASK;

    a0 = a & G3D_U16_MASK;
    a1 = (a >> 16) & G3D_U16_MASK;
    b0 = b & G3D_U16_MASK;
    b1 = (b >> 16) & G3D_U16_MASK;

    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;

    mid = (p0 >> 16) + (p1 & G3D_U16_MASK) + (p2 & G3D_U16_MASK);

    out.lo = ((p0 & G3D_U16_MASK) | ((mid & G3D_U16_MASK) << 16)) & G3D_U32_MASK;
    out.hi = (p3 + (p1 >> 16) + (p2 >> 16) + (mid >> 16)) & G3D_U32_MASK;

    return out;
}

static G3D_U64 g3d_u64_from_u32_shift_left(unsigned long value, int shift)
{
    G3D_U64 out;

    value &= G3D_U32_MASK;
    out.hi = (value >> (32 - shift)) & G3D_U32_MASK;
    out.lo = (value << shift) & G3D_U32_MASK;

    return out;
}

static unsigned long g3d_u64_get_bit(const G3D_U64* v, int bit_index)
{
    if (bit_index < 32) {
        return (v->lo >> bit_index) & 1UL;
    }

    return (v->hi >> (bit_index - 32)) & 1UL;
}

static void g3d_u64_shl1(G3D_U64* v)
{
    unsigned long carry;

    carry = (v->lo >> 31) & 1UL;
    v->lo = (v->lo << 1) & G3D_U32_MASK;
    v->hi = ((v->hi << 1) | carry) & G3D_U32_MASK;
}

static int g3d_u64_ge_u32(const G3D_U64* v, unsigned long rhs)
{
    rhs &= G3D_U32_MASK;

    if (v->hi != 0UL) {
        return 1;
    }

    return ((v->lo & G3D_U32_MASK) >= rhs) ? 1 : 0;
}

static void g3d_u64_sub_u32(G3D_U64* v, unsigned long rhs)
{
    unsigned long old_lo;

    rhs &= G3D_U32_MASK;
    old_lo = v->lo & G3D_U32_MASK;

    v->lo = (old_lo - rhs) & G3D_U32_MASK;
    if (old_lo < rhs) {
        v->hi = (v->hi - 1UL) & G3D_U32_MASK;
    }
}

static unsigned long g3d_u64_shr_to_u32_sat(const G3D_U64* v, int shift)
{
    unsigned long overflow_mask;
    unsigned long out;

    overflow_mask = ~((1UL << shift) - 1UL);

    if ((v->hi & overflow_mask) != 0UL) {
        return G3D_U32_MASK;
    }

    out = ((v->hi << (32 - shift)) | (v->lo >> shift)) & G3D_U32_MASK;
    return out;
}

/* ------------------------------------------------------------------------- */
/*  Fixed básico                                                             */
/* ------------------------------------------------------------------------- */

g3d_fix g3d_fix_add_sat(g3d_fix a, g3d_fix b)
{
    if (b > 0) {
        if (a > (G3D_FIX_MAX - b)) {
            return G3D_FIX_MAX;
        }
    } else if (b < 0) {
        if (b == G3D_FIX_MIN) {
            if (a <= 0) {
                return G3D_FIX_MIN;
            }
        } else if (a < (G3D_FIX_MIN - b)) {
            return G3D_FIX_MIN;
        }
    }

    return a + b;
}

g3d_fix g3d_fix_sub_sat(g3d_fix a, g3d_fix b)
{
    if (b == G3D_FIX_MIN) {
        if (a >= 0) {
            return G3D_FIX_MAX;
        }
        return a - b;
    }

    return g3d_fix_add_sat(a, (g3d_fix)(-b));
}

g3d_fix g3d_fix_neg_sat(g3d_fix x)
{
    if (x == G3D_FIX_MIN) {
        return G3D_FIX_MAX;
    }

    return (g3d_fix)(-x);
}

g3d_fix g3d_fix_mul(g3d_fix a, g3d_fix b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long result_abs;
    G3D_U64 product;
    int negative;

    negative = ((a < 0) != (b < 0)) ? 1 : 0;
    ua = g3d_fix_abs_u32(a);
    ub = g3d_fix_abs_u32(b);

    product = g3d_u64_mul_u32(ua, ub);
    g3d_u64_add_u32(&product, (1UL << (G3D_FIX_FRAC - 1)));
    result_abs = g3d_u64_shr_to_u32_sat(&product, G3D_FIX_FRAC);

    return g3d_fix_from_u32_sat(result_abs, negative);
}

g3d_fix g3d_fix_div(g3d_fix a, g3d_fix b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long quotient;
    unsigned long half_den;
    G3D_U64 numerator;
    G3D_U64 remainder;
    int negative;
    int bit;
    int overflow;

    if (b == 0) {
        return 0;
    }

    negative = ((a < 0) != (b < 0)) ? 1 : 0;
    ua = g3d_fix_abs_u32(a);
    ub = g3d_fix_abs_u32(b);

    numerator = g3d_u64_from_u32_shift_left(ua, G3D_FIX_FRAC);
    remainder = g3d_u64_zero();
    quotient = 0UL;
    overflow = 0;

    for (bit = 31 + G3D_FIX_FRAC; bit >= 0; --bit) {
        g3d_u64_shl1(&remainder);
        remainder.lo |= g3d_u64_get_bit(&numerator, bit);

        if (g3d_u64_ge_u32(&remainder, ub)) {
            g3d_u64_sub_u32(&remainder, ub);

            if (bit >= 32) {
                overflow = 1;
            } else {
                quotient |= (1UL << bit);
            }
        }
    }

    if (overflow) {
        return negative ? G3D_FIX_MIN : G3D_FIX_MAX;
    }

    half_den = (ub >> 1);
    if ((ub & 1UL) != 0UL) {
        half_den += 1UL;
    }

    if (g3d_u64_ge_u32(&remainder, half_den)) {
        if (quotient == G3D_U32_MASK) {
            return negative ? G3D_FIX_MIN : G3D_FIX_MAX;
        }
        quotient += 1UL;
    }

    return g3d_fix_from_u32_sat(quotient, negative);
}

g3d_fix g3d_fix_sqrt(g3d_fix x)
{
    g3d_fix current;
    g3d_fix next;
    g3d_fix half;
    int i;

    if (x <= 0) {
        return 0;
    }

    if (x < G3D_FIX_ONE) {
        current = G3D_FIX_ONE;
    } else {
        current = x;
    }

    half = G3D_FIX_HALF;

    for (i = 0; i < 12; ++i) {
        next = g3d_fix_mul(g3d_fix_add_sat(current, g3d_fix_div(x, current)), half);

        if (next == current) {
            break;
        }

        if (g3d_fix_abs(g3d_fix_sub_sat(next, current)) <= G3D_FIX_EPSILON) {
            current = next;
            break;
        }

        current = next;
    }

    return current;
}

g3d_fix g3d_fix_abs(g3d_fix x)
{
    if (x == G3D_FIX_MIN) {
        return G3D_FIX_MAX;
    }

    return (x < 0) ? (g3d_fix)(-x) : x;
}

g3d_fix g3d_fix_min(g3d_fix a, g3d_fix b)
{
    return (a < b) ? a : b;
}

g3d_fix g3d_fix_max(g3d_fix a, g3d_fix b)
{
    return (a > b) ? a : b;
}

g3d_fix g3d_fix_clamp(g3d_fix v, g3d_fix minv, g3d_fix maxv)
{
    if (minv <= maxv) {
        if (v < minv) {
            v = minv;
        }
        if (v > maxv) {
            v = maxv;
        }
    }
    return v;
}

/* ------------------------------------------------------------------------- */
/*  Helpers de ángulo                                                        */
/* ------------------------------------------------------------------------- */

g3d_fix g3d_fix_wrap_angle_deg(g3d_fix a)
{
    g3d_fix full;
    g3d_fix half;

    full = G3D_FIX_FROM_INT(360);
    half = G3D_FIX_FROM_INT(180);

    if ((a >= full) || (a <= -full)) {
        a %= full;
    }

    if (a > half) {
        a -= full;
    }
    if (a < -half) {
        a += full;
    }

    return a;
}

g3d_fix g3d_fix_wrap_angle_rad(g3d_fix a)
{
    g3d_fix full;
    g3d_fix half;

    full = G3D_FIX_TWO_PI;
    half = G3D_FIX_PI;

    if ((a >= full) || (a <= -full)) {
        a %= full;
    }

    if (a > half) {
        a -= full;
    }
    if (a < -half) {
        a += full;
    }

    return a;
}

/* ------------------------------------------------------------------------- */
/*  Trigonometría                                                            */
/* ------------------------------------------------------------------------- */

#define G3D_SIN_LUT_SIZE 256

static const unsigned short g3d_sin_quarter_lut[G3D_SIN_LUT_SIZE + 1] = {
       0,   25,   50,   75,  101,  126,  151,  176,
     201,  226,  251,  276,  301,  326,  351,  376,
     401,  426,  451,  476,  501,  526,  551,  576,
     601,  626,  651,  675,  700,  725,  750,  774,
     799,  824,  848,  873,  897,  922,  946,  971,
     995, 1020, 1044, 1068, 1092, 1117, 1141, 1165,
    1189, 1213, 1237, 1261, 1285, 1309, 1332, 1356,
    1380, 1404, 1427, 1451, 1474, 1498, 1521, 1544,
    1567, 1591, 1614, 1637, 1660, 1683, 1706, 1729,
    1751, 1774, 1797, 1819, 1842, 1864, 1886, 1909,
    1931, 1953, 1975, 1997, 2019, 2041, 2062, 2084,
    2106, 2127, 2149, 2170, 2191, 2213, 2234, 2255,
    2276, 2296, 2317, 2338, 2359, 2379, 2399, 2420,
    2440, 2460, 2480, 2500, 2520, 2540, 2559, 2579,
    2598, 2618, 2637, 2656, 2675, 2694, 2713, 2732,
    2751, 2769, 2788, 2806, 2824, 2843, 2861, 2878,
    2896, 2914, 2932, 2949, 2967, 2984, 3001, 3018,
    3035, 3052, 3068, 3085, 3102, 3118, 3134, 3150,
    3166, 3182, 3198, 3214, 3229, 3244, 3260, 3275,
    3290, 3305, 3320, 3334, 3349, 3363, 3378, 3392,
    3406, 3420, 3433, 3447, 3461, 3474, 3487, 3500,
    3513, 3526, 3539, 3551, 3564, 3576, 3588, 3600,
    3612, 3624, 3636, 3647, 3659, 3670, 3681, 3692,
    3703, 3713, 3724, 3734, 3745, 3755, 3765, 3775,
    3784, 3794, 3803, 3812, 3822, 3831, 3839, 3848,
    3857, 3865, 3873, 3881, 3889, 3897, 3905, 3912,
    3920, 3927, 3934, 3941, 3948, 3954, 3961, 3967,
    3973, 3979, 3985, 3991, 3996, 4002, 4007, 4012,
    4017, 4022, 4027, 4031, 4036, 4040, 4044, 4048,
    4052, 4055, 4059, 4062, 4065, 4068, 4071, 4074,
    4076, 4079, 4081, 4083, 4085, 4087, 4088, 4090,
    4091, 4092, 4093, 4094, 4095, 4095, 4096, 4096,
    4096
};

static g3d_fix g3d_sin_quarter_interp(g3d_fix angle_rad)
{
    unsigned long scaled_num;
    unsigned long idx;
    unsigned long rem;
    unsigned long base;
    unsigned long next;
    unsigned long delta;
    unsigned long interp;
    unsigned long denom;

    if (angle_rad <= 0) {
        return 0;
    }
    if (angle_rad >= G3D_FIX_HALF_PI) {
        return G3D_FIX_ONE;
    }

    scaled_num = ((unsigned long)angle_rad * (unsigned long)G3D_SIN_LUT_SIZE);
    denom = (unsigned long)G3D_FIX_HALF_PI;

    idx = scaled_num / denom;
    rem = scaled_num % denom;

    base = (unsigned long)g3d_sin_quarter_lut[idx];
    next = (unsigned long)g3d_sin_quarter_lut[idx + 1UL];
    delta = next - base;

    interp = base + ((delta * rem + (denom >> 1)) / denom);
    return (g3d_fix)interp;
}

g3d_fix gamlib_deg2rad(g3d_fix deg)
{
    g3d_fix num;

    num = g3d_fix_mul(deg, G3D_FIX_PI);
    return g3d_fix_div(num, G3D_FIX_FROM_INT(180));
}

g3d_fix gamlib_rad2deg(g3d_fix rad)
{
    g3d_fix num;

    num = g3d_fix_mul(rad, G3D_FIX_FROM_INT(180));
    return g3d_fix_div(num, G3D_FIX_PI);
}

g3d_fix gamlib_sin(g3d_fix rad)
{
    g3d_fix wrapped;
    g3d_fix local;
    int negative;

    wrapped = g3d_fix_wrap_angle_rad(rad);
    if (wrapped < 0) {
        wrapped = g3d_fix_add_sat(wrapped, G3D_FIX_TWO_PI);
    }

    negative = 0;
    local = wrapped;

    if (local > G3D_FIX_PI) {
        local = g3d_fix_sub_sat(local, G3D_FIX_PI);
        negative = 1;
    }

    if (local > G3D_FIX_HALF_PI) {
        local = g3d_fix_sub_sat(G3D_FIX_PI, local);
    }

    local = g3d_sin_quarter_interp(local);

    if (negative) {
        return g3d_fix_neg_sat(local);
    }

    return local;
}

g3d_fix gamlib_cos(g3d_fix rad)
{
    return gamlib_sin(g3d_fix_add_sat(rad, G3D_FIX_HALF_PI));
}

g3d_fix gamlib_tan(g3d_fix rad)
{
    g3d_fix s;
    g3d_fix c;
    int negative;

    s = gamlib_sin(rad);
    c = gamlib_cos(rad);

    if (g3d_fix_abs(c) <= G3D_FIX_EPSILON) {
        negative = ((s < 0) != (c < 0)) ? 1 : 0;
        return negative ? G3D_FIX_MIN : G3D_FIX_MAX;
    }

    return g3d_fix_div(s, c);
}

/* ------------------------------------------------------------------------- */
/*  Vec3                                                                     */
/* ------------------------------------------------------------------------- */

Vec3 gamlib_vec3(g3d_fix x, g3d_fix y, g3d_fix z)
{
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

void gamlib_vec3_set(Vec3* v, g3d_fix x, g3d_fix y, g3d_fix z)
{
    if (!v) {
        return;
    }

    v->x = x;
    v->y = y;
    v->z = z;
}

void gamlib_vec3_add(Vec3* out, const Vec3* a, const Vec3* b)
{
    if (!out || !a || !b) {
        return;
    }

    out->x = g3d_fix_add_sat(a->x, b->x);
    out->y = g3d_fix_add_sat(a->y, b->y);
    out->z = g3d_fix_add_sat(a->z, b->z);
}

void gamlib_vec3_sub(Vec3* out, const Vec3* a, const Vec3* b)
{
    if (!out || !a || !b) {
        return;
    }

    out->x = g3d_fix_sub_sat(a->x, b->x);
    out->y = g3d_fix_sub_sat(a->y, b->y);
    out->z = g3d_fix_sub_sat(a->z, b->z);
}

void gamlib_vec3_scale(Vec3* out, const Vec3* v, g3d_fix s)
{
    if (!out || !v) {
        return;
    }

    out->x = g3d_fix_mul(v->x, s);
    out->y = g3d_fix_mul(v->y, s);
    out->z = g3d_fix_mul(v->z, s);
}

g3d_fix gamlib_vec3_dot(const Vec3* a, const Vec3* b)
{
    g3d_fix x;
    g3d_fix y;
    g3d_fix z;
    g3d_fix sum;

    if (!a || !b) {
        return 0;
    }

    x = g3d_fix_mul(a->x, b->x);
    y = g3d_fix_mul(a->y, b->y);
    z = g3d_fix_mul(a->z, b->z);

    sum = g3d_fix_add_sat(x, y);
    sum = g3d_fix_add_sat(sum, z);

    return sum;
}

void gamlib_vec3_cross(Vec3* out, const Vec3* a, const Vec3* b)
{
    g3d_fix cx;
    g3d_fix cy;
    g3d_fix cz;

    if (!out || !a || !b) {
        return;
    }

    cx = g3d_fix_sub_sat(g3d_fix_mul(a->y, b->z), g3d_fix_mul(a->z, b->y));
    cy = g3d_fix_sub_sat(g3d_fix_mul(a->z, b->x), g3d_fix_mul(a->x, b->z));
    cz = g3d_fix_sub_sat(g3d_fix_mul(a->x, b->y), g3d_fix_mul(a->y, b->x));

    out->x = cx;
    out->y = cy;
    out->z = cz;
}

g3d_fix gamlib_vec3_length(const Vec3* v)
{
    g3d_fix xx;
    g3d_fix yy;
    g3d_fix zz;
    g3d_fix sum;

    if (!v) {
        return 0;
    }

    xx = g3d_fix_mul(v->x, v->x);
    yy = g3d_fix_mul(v->y, v->y);
    zz = g3d_fix_mul(v->z, v->z);

    sum = g3d_fix_add_sat(xx, yy);
    sum = g3d_fix_add_sat(sum, zz);

    return g3d_fix_sqrt(sum);
}

void gamlib_vec3_normalize(Vec3* out, const Vec3* v)
{
    g3d_fix len;

    if (!out || !v) {
        return;
    }

    len = gamlib_vec3_length(v);
    if (len <= G3D_FIX_EPSILON) {
        out->x = 0;
        out->y = 0;
        out->z = 0;
        return;
    }

    out->x = g3d_fix_div(v->x, len);
    out->y = g3d_fix_div(v->y, len);
    out->z = g3d_fix_div(v->z, len);
}
