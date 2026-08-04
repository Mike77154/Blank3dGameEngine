#include "ccs_fixed.h"

/* ============================================================
   Internal helpers (C89)
   ============================================================ */

static ccs_u32 ccs_u32_abs_i32(ccs_i32 v)
{
    /*
        abs() seguro para INT_MIN:
        - Evita overflow de -INT_MIN.
    */
    if (v >= 0)
        return (ccs_u32)v;

    /* -(v+1) cabe, luego +1 en unsigned */
    return (ccs_u32)(-(v + 1)) + 1u;
}

static ccs_fixed ccs_fixed_from_mag(ccs_u32 mag, int neg)
{
    if (!neg) {
        if (mag > (ccs_u32)CCS_I32_MAX)
            return (ccs_fixed)CCS_I32_MAX;
        return (ccs_fixed)(ccs_i32)mag;
    }

    /* neg */
    if (mag >= 0x80000000u)
        return (ccs_fixed)CCS_I32_MIN;

    return (ccs_fixed)(-(ccs_i32)mag);
}

/* ============================================================
   Constructors & conversion
   ============================================================ */

ccs_fixed ccs_fixed_from_int(ccs_i32 i)
{
    int neg;
    ccs_u32 mag;

    neg = 0;
    if (i < 0)
        neg = 1;

    mag = ccs_u32_abs_i32(i);

    /* Q16.16 representable integer range is [-32768, 32767]. */
    if (!neg) {
        if (mag > 0x00007FFFu)
            return (ccs_fixed)CCS_I32_MAX;
    } else {
        if (mag > 0x00008000u)
            return (ccs_fixed)CCS_I32_MIN;
    }

    mag <<= CCS_FIXED_SHIFT;
    return ccs_fixed_from_mag(mag, neg);
}

ccs_i32 ccs_fixed_to_int(ccs_fixed f)
{
    if (f >= 0)
        return (ccs_i32)(((ccs_u32)f) >> CCS_FIXED_SHIFT);
    else {
        ccs_u32 mag;
        mag = ccs_u32_abs_i32((ccs_i32)f);
        return -(ccs_i32)(mag >> CCS_FIXED_SHIFT);
    }
}

ccs_i32 ccs_fixed_to_int_floor(ccs_fixed f)
{
    if (f >= 0)
        return (ccs_i32)(((ccs_u32)f) >> CCS_FIXED_SHIFT);
    else {
        ccs_u32 mag;
        mag = ccs_u32_abs_i32((ccs_i32)f);
        /* floor(-x) = -ceil(x) */
        return -(ccs_i32)((mag + 0xFFFFu) >> CCS_FIXED_SHIFT);
    }
}

ccs_i32 ccs_fixed_to_int_ceil(ccs_fixed f)
{
    if (f >= 0) {
        ccs_u32 uf;
        uf = (ccs_u32)f;
        return (ccs_i32)((uf + 0xFFFFu) >> CCS_FIXED_SHIFT);
    } else {
        ccs_u32 mag;
        mag = ccs_u32_abs_i32((ccs_i32)f);
        /* ceil(-x) = -floor(x) */
        return -(ccs_i32)(mag >> CCS_FIXED_SHIFT);
    }
}

/* ============================================================
   Multiplication
   (a*b)>>16, determinista, C89, sin 64-bit
   ============================================================ */

ccs_fixed ccs_fixed_mul(ccs_fixed a, ccs_fixed b)
{
    int neg;
    ccs_u32 ua;
    ccs_u32 ub;
    ccs_u32 a_hi;
    ccs_u32 a_lo;
    ccs_u32 b_hi;
    ccs_u32 b_lo;

    ccs_u32 term_hi;
    ccs_u32 term_mid1;
    ccs_u32 term_mid2;
    ccs_u32 term_low;

    ccs_u32 sum;
    int overflow;

    neg = 0;
    if (a < 0)
        neg = !neg;
    if (b < 0)
        neg = !neg;

    ua = ccs_u32_abs_i32((ccs_i32)a);
    ub = ccs_u32_abs_i32((ccs_i32)b);

    a_hi = ua >> 16;
    a_lo = ua & 0xFFFFu;
    b_hi = ub >> 16;
    b_lo = ub & 0xFFFFu;

    term_hi   = a_hi * b_hi;
    term_mid1 = a_hi * b_lo;
    term_mid2 = a_lo * b_hi;
    term_low  = a_lo * b_lo;

    /*
        (ua*ub)>>16 = (term_hi<<16) + term_mid1 + term_mid2 + (term_low>>16)

        Detectamos overflow y saturamos.
    */

    overflow = 0;

    /* term_hi<<16 pierde bits si term_hi > 0x0000FFFF */
    if (term_hi > 0x0000FFFFu)
        overflow = 1;

    sum = (term_hi << 16);

    {
        ccs_u32 tmp;

        tmp = sum + term_mid1;
        if (tmp < sum)
            overflow = 1;
        sum = tmp;

        tmp = sum + term_mid2;
        if (tmp < sum)
            overflow = 1;
        sum = tmp;

        tmp = sum + (term_low >> 16);
        if (tmp < sum)
            overflow = 1;
        sum = tmp;
    }

    if (overflow) {
        /* saturar por signo */
        return ccs_fixed_from_mag(0xFFFFFFFFu, neg);
    }

    return ccs_fixed_from_mag(sum, neg);
}

/* ============================================================
   Division
   (a<<16)/b, determinista, C89, sin 64-bit
   ============================================================ */

ccs_fixed ccs_fixed_div(ccs_fixed a, ccs_fixed b)
{
    int neg;
    ccs_u32 ua;
    ccs_u32 ub;

    /* 48-bit numerator = ua << 16 */
    ccs_u32 num_hi16;
    ccs_u32 num_lo32;

    /* remainder as (hi, lo) where hi is 0/1 (33-bit remainder) */
    ccs_u32 rem_hi;
    ccs_u32 rem_lo;

    ccs_u32 q;
    int overflow;

    int i;

    if (b == 0)
        return 0;

    neg = 0;
    if (a < 0)
        neg = !neg;
    if (b < 0)
        neg = !neg;

    ua = ccs_u32_abs_i32((ccs_i32)a);
    ub = ccs_u32_abs_i32((ccs_i32)b);

    if (ub == 0)
        return 0;

    /* 48-bit numerator split */
    num_hi16 = ua >> 16;
    num_lo32 = ua << 16;

    rem_hi = 0;
    rem_lo = 0;
    q = 0;
    overflow = 0;

    for (i = 47; i >= 0; --i) {
        ccs_u32 bit;

        /* extract bit i from numerator */
        if (i >= 32)
            bit = (num_hi16 >> (i - 32)) & 1u;
        else
            bit = (num_lo32 >> i) & 1u;

        /* remainder <<= 1; remainder |= bit */
        rem_hi = (rem_hi << 1) | (rem_lo >> 31);
        rem_lo = (rem_lo << 1) | bit;

        /* if remainder >= divisor: remainder -= divisor; set quotient bit */
        if (rem_hi != 0u || rem_lo >= ub) {
            rem_lo = rem_lo - ub;
            rem_hi = 0;

            if (i >= 32) {
                overflow = 1;
            } else {
                q |= (1u << i);
            }
        }

        if (i == 0)
            break; /* evita underflow de i (C89) */
    }

    if (overflow)
        return ccs_fixed_from_mag(0xFFFFFFFFu, neg);

    return ccs_fixed_from_mag(q, neg);
}

/* ============================================================
   Absolute
   ============================================================ */

ccs_fixed ccs_fixed_abs(ccs_fixed v)
{
    if (v >= 0)
        return v;

    if (v == (ccs_fixed)CCS_I32_MIN)
        return (ccs_fixed)CCS_I32_MAX;

    return (ccs_fixed)(-v);
}

/* ============================================================
   Clamp
   ============================================================ */

ccs_fixed ccs_fixed_clamp(ccs_fixed v, ccs_fixed min, ccs_fixed max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

/* ============================================================
   Integer square root (deterministic)
   Input/output in fixed-point
   ============================================================ */

ccs_fixed ccs_fixed_sqrt(ccs_fixed v)
{
    ccs_u32 x;
    ccs_u32 result;
    ccs_u32 bit;

    if (v <= 0)
        return 0;

    x = (ccs_u32)v;
    result = 0;
    bit = 1u << 30;

    while (bit > x)
        bit >>= 2;

    while (bit != 0) {
        if (x >= result + bit) {
            x -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    /* adjust back to Q16.16 */
    return (ccs_fixed)(result << (CCS_FIXED_SHIFT / 2));
}
