#ifndef VP_FIXED_H
#define VP_FIXED_H

#include "vp_types.h"
#include "vp_config.h"

#ifndef VP_HEADER_FN
#if defined(__GNUC__) || defined(__clang__)
#define VP_HEADER_FN static __attribute__((unused))
#else
#define VP_HEADER_FN static
#endif
#endif

typedef vp_i32 vp_fx;

#define VP_FX_ONE   ((vp_fx)(1L << VP_FRAC_BITS))
#define VP_FX_HALF  ((vp_fx)(VP_FX_ONE >> 1))
#define VP_FX_MAX   ((vp_fx)2147483647L)
#define VP_FX_MIN   ((vp_fx)(-2147483647L - 1L))

/* Convert a signed value to magnitude without ever evaluating -INT_MIN. */
VP_HEADER_FN vp_u32 vp_i32_magnitude(vp_i32 x)
{
    vp_u32 ux = (vp_u32)x;
    return (x < 0) ? (vp_u32)(0UL - ux) : ux;
}

VP_HEADER_FN vp_fx vp_fx_from_magnitude(vp_u32 mag, vp_u32 negative)
{
    if (negative) {
        if (mag >= 0x80000000UL) return VP_FX_MIN;
        return (vp_fx)(-(vp_i32)mag);
    }
    if (mag > 0x7FFFFFFFUL) return VP_FX_MAX;
    return (vp_fx)((vp_i32)mag);
}

VP_HEADER_FN vp_fx vp_fx_from_int(vp_i32 x)
{
    vp_u32 mag = vp_i32_magnitude(x);
    vp_u32 limit = (vp_u32)(0x7FFFFFFFUL >> VP_FRAC_BITS);
    if (mag > limit) return (x < 0) ? VP_FX_MIN : VP_FX_MAX;
    mag <<= VP_FRAC_BITS;
    return vp_fx_from_magnitude(mag, (vp_u32)(x < 0));
}

VP_HEADER_FN vp_i32 vp_fx_to_int(vp_fx x)
{
    vp_u32 mag = vp_i32_magnitude(x) >> VP_FRAC_BITS;
    if (x < 0) return (vp_i32)(-(vp_i32)mag);
    return (vp_i32)mag;
}


VP_HEADER_FN vp_fx vp_fx_abs(vp_fx a)
{
    if (a == VP_FX_MIN) return VP_FX_MAX;
    return (a < 0) ? (vp_fx)(-a) : a;
}

VP_HEADER_FN vp_fx vp_fx_clamp(vp_fx x, vp_fx lo, vp_fx hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* --- 64-bit emulation via 32-bit parts --- */
typedef struct vp_u64pair { vp_u32 lo; vp_u32 hi; } vp_u64pair;

VP_HEADER_FN vp_u64pair vp_u64_make(vp_u32 hi, vp_u32 lo) { vp_u64pair r; r.hi=hi; r.lo=lo; return r; }

VP_HEADER_FN int vp_u64_cmp(vp_u64pair a, vp_u64pair b)
{
    if (a.hi < b.hi) return -1;
    if (a.hi > b.hi) return 1;
    if (a.lo < b.lo) return -1;
    if (a.lo > b.lo) return 1;
    return 0;
}

VP_HEADER_FN vp_u64pair vp_u64_add(vp_u64pair a, vp_u64pair b)
{
    vp_u64pair r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi;
    if (r.lo < a.lo) r.hi += 1;
    return r;
}

VP_HEADER_FN vp_u64pair vp_u64_sub(vp_u64pair a, vp_u64pair b)
{
    vp_u64pair r;
    r.hi = a.hi - b.hi;
    if (a.lo < b.lo) r.hi -= 1;
    r.lo = a.lo - b.lo;
    return r;
}

/* Exact unsigned 32x32 -> 64 using 16-bit limbs, including both carries. */
VP_HEADER_FN vp_u64pair vp_umul_32x32(vp_u32 a, vp_u32 b)
{
    vp_u32 a0 = a & 0xFFFFUL;
    vp_u32 a1 = a >> 16;
    vp_u32 b0 = b & 0xFFFFUL;
    vp_u32 b1 = b >> 16;
    vp_u32 w0 = a0 * b0;
    vp_u32 t = a1 * b0 + (w0 >> 16);
    vp_u32 w1 = t & 0xFFFFUL;
    vp_u32 w2 = t >> 16;
    vp_u64pair r;

    w1 += a0 * b1;
    r.hi = a1 * b1 + w2 + (w1 >> 16);
    r.lo = (w1 << 16) | (w0 & 0xFFFFUL);
    return r;
}

VP_HEADER_FN vp_u64pair vp_shr_u64(vp_u64pair v, vp_u32 s)
{
    vp_u64pair r;
    if (s == 0) return v;
    if (s < 32) {
        r.lo = (v.lo >> s) | (v.hi << (32 - s));
        r.hi = (v.hi >> s);
        return r;
    }
    if (s < 64) {
        r.lo = (v.hi >> (s - 32));
        r.hi = 0;
        return r;
    }
    r.lo = 0; r.hi = 0; return r;
}

/* Unsigned 64 / 32 -> saturated 32-bit quotient (shift-subtract). */
VP_HEADER_FN vp_u32 vp_udiv_u64_u32(vp_u64pair num, vp_u32 den)
{
    vp_u32 q, i;
    vp_u64pair r;
    if (den == 0) return 0;
    q = 0;
    r.hi = 0; r.lo = 0;

    for (i = 0; i < 64; ++i) {
        vp_u32 bit;
        if (i < 32) bit = (num.hi >> (31 - i)) & 1UL;
        else bit = (num.lo >> (63 - i)) & 1UL;

        r.hi = (r.hi << 1) | (r.lo >> 31);
        r.lo = (r.lo << 1) | bit;

        q <<= 1;

        if (r.hi != 0 || r.lo >= den) {
            if (r.lo < den) r.hi -= 1;
            r.lo -= den;
            q |= 1UL;
        }
    }
    return q;
}

/* fixed mul (a*b)>>FRAC, saturating at the signed fixed range */
VP_HEADER_FN vp_fx vp_fx_mul(vp_fx a, vp_fx b)
{
    vp_u32 ua = vp_i32_magnitude(a);
    vp_u32 ub = vp_i32_magnitude(b);
    vp_u32 sign = (vp_u32)((a < 0) != (b < 0));
    vp_u64pair p = vp_umul_32x32(ua, ub);
    vp_u64pair s = vp_shr_u64(p, (vp_u32)VP_FRAC_BITS);
    if (s.hi != 0) return sign ? VP_FX_MIN : VP_FX_MAX;
    return vp_fx_from_magnitude(s.lo, sign);
}

VP_HEADER_FN vp_fx vp_fx_lerp(vp_fx a, vp_fx b, vp_fx t)
{
    return a + vp_fx_mul((b - a), t);
}

/* fixed div (a<<FRAC)/b, saturating */
VP_HEADER_FN vp_fx vp_fx_div(vp_fx a, vp_fx b)
{
    vp_u32 ua, ub, sign;
    vp_u64pair num;
    vp_u32 q;
    if (b == 0) return 0;

    sign = (vp_u32)((a < 0) != (b < 0));
    ua = vp_i32_magnitude(a);
    ub = vp_i32_magnitude(b);

#if (VP_FRAC_BITS < 32)
    num.hi = (ua >> (32 - VP_FRAC_BITS));
    num.lo = (ua << VP_FRAC_BITS);
#else
    num.hi = (ua << (VP_FRAC_BITS - 32));
    num.lo = 0;
#endif

    {
        vp_u32 maxMag = sign ? 0x80000000UL : 0x7FFFFFFFUL;
        vp_u64pair maxNum = vp_umul_32x32(ub, maxMag);
        if (vp_u64_cmp(num, maxNum) > 0) return sign ? VP_FX_MIN : VP_FX_MAX;
    }
    q = vp_udiv_u64_u32(num, ub);
    return vp_fx_from_magnitude(q, sign);
}

VP_HEADER_FN vp_u32 vp_isqrt_u32(vp_u32 x)
{
    vp_u32 op, res, one;
    op = x; res = 0; one = 1UL << 30;
    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) { op -= res + one; res = (res >> 1) + one; }
        else res >>= 1;
        one >>= 2;
    }
    return res;
}

VP_HEADER_FN vp_u32 vp_isqrt_u64(vp_u64pair x)
{
    vp_u64pair op, res, one;
    op = x; res = vp_u64_make(0,0); one = vp_u64_make(1UL<<30, 0);
    while (vp_u64_cmp(one, op) > 0) one = vp_shr_u64(one, 2);
    while (one.hi != 0 || one.lo != 0) {
        vp_u64pair rpo = vp_u64_add(res, one);
        if (vp_u64_cmp(op, rpo) >= 0) {
            op = vp_u64_sub(op, rpo);
            res = vp_u64_add(vp_shr_u64(res, 1), one);
        } else res = vp_shr_u64(res, 1);
        one = vp_shr_u64(one, 2);
    }
    return res.lo;
}

VP_HEADER_FN vp_fx vp_fx_sqrt(vp_fx a)
{
    vp_u32 ua;
    vp_u64pair num;
    vp_u32 root;
    if (a <= 0) return 0;
    ua = (vp_u32)a;
#if (VP_FRAC_BITS < 32)
    num.hi = (ua >> (32 - VP_FRAC_BITS));
    num.lo = (ua << VP_FRAC_BITS);
#else
    num.hi = (ua << (VP_FRAC_BITS - 32));
    num.lo = 0;
#endif
    root = vp_isqrt_u64(num);
    return vp_fx_from_magnitude(root, 0);
}

#endif
