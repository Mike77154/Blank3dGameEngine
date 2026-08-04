#include "rpyl_fixed.h"

/*
    C89-safe 32 x 32 -> 64 unsigned multiply represented as two 32-bit words.
    No long long, floating point, heap allocation, or implementation-sized long
    is required.
*/
static void rpyl_u32_mul_wide(rpyl_u32 a, rpyl_u32 b, rpyl_u32* hi, rpyl_u32* lo) {
    rpyl_u32 a0;
    rpyl_u32 a1;
    rpyl_u32 b0;
    rpyl_u32 b1;
    rpyl_u32 w0;
    rpyl_u32 w1;
    rpyl_u32 w2;
    rpyl_u32 t;

    a0 = a & (rpyl_u32)0xFFFFU;
    a1 = a >> 16;
    b0 = b & (rpyl_u32)0xFFFFU;
    b1 = b >> 16;

    w0 = a0 * b0;
    t = (a1 * b0) + (w0 >> 16);
    w1 = t & (rpyl_u32)0xFFFFU;
    w2 = t >> 16;
    w1 = (a0 * b1) + w1;

    if (hi) *hi = (a1 * b1) + w2 + (w1 >> 16);
    if (lo) *lo = (w1 << 16) | (w0 & (rpyl_u32)0xFFFFU);
}

/* Unsigned 64/32 long division. Quotient is returned as two 32-bit words. */
static void rpyl_u64_div_u32(
    rpyl_u32 n_hi,
    rpyl_u32 n_lo,
    rpyl_u32 denominator,
    rpyl_u32* q_hi,
    rpyl_u32* q_lo
) {
    rpyl_u32 qh;
    rpyl_u32 ql;
    rpyl_u32 remainder;
    rpyl_u32 bit;
    rpyl_u32 carry;
    int i;

    qh = 0u;
    ql = 0u;
    remainder = 0u;

    for (i = 63; i >= 0; i--) {
        if (i >= 32) bit = (n_hi >> (i - 32)) & (rpyl_u32)1u;
        else bit = (n_lo >> i) & (rpyl_u32)1u;

        carry = remainder >> 31;
        remainder = (remainder << 1) | bit;
        if (carry || remainder >= denominator) {
            remainder = (rpyl_u32)(remainder - denominator);
            if (i >= 32) qh |= (rpyl_u32)1u << (i - 32);
            else ql |= (rpyl_u32)1u << i;
        }
    }

    if (q_hi) *q_hi = qh;
    if (q_lo) *q_lo = ql;
}

static rpyl_u32 rpyl_fx_magnitude(rpyl_fx v) {
    if (v < 0) return (rpyl_u32)(0u - (rpyl_u32)v);
    return (rpyl_u32)v;
}

static rpyl_fx rpyl_fx_apply_sign_saturated(rpyl_u32 magnitude, int negative, int overflowed) {
    rpyl_u32 limit;
    if (negative) limit = (rpyl_u32)0x80000000UL;
    else limit = (rpyl_u32)0x7FFFFFFFUL;

    if (overflowed || magnitude > limit) {
        return negative ? RPYL_I32_MIN : RPYL_I32_MAX;
    }
    if (!negative) return (rpyl_fx)magnitude;
    if (magnitude == (rpyl_u32)0x80000000UL) return RPYL_I32_MIN;
    return (rpyl_fx)(-((rpyl_i32)magnitude));
}

rpyl_fx rpyl_fx_from_int(long v) {
    if (v > 32767L) return RPYL_I32_MAX;
    if (v < -32768L) return RPYL_I32_MIN;
    return (rpyl_fx)(v * RPYL_FX_ONE);
}

long rpyl_fx_to_int(rpyl_fx v) {
    rpyl_u32 magnitude;
    long whole;
    magnitude = rpyl_fx_magnitude(v);
    whole = (long)(magnitude / (rpyl_u32)RPYL_FX_ONE);
    return v < 0 ? -whole : whole;
}

rpyl_fx rpyl_fx_add(rpyl_fx a, rpyl_fx b) {
    if (b > 0 && a > (rpyl_fx)(RPYL_I32_MAX - b)) return RPYL_I32_MAX;
    if (b < 0 && a < (rpyl_fx)(RPYL_I32_MIN - b)) return RPYL_I32_MIN;
    return (rpyl_fx)(a + b);
}

rpyl_fx rpyl_fx_sub(rpyl_fx a, rpyl_fx b) {
    if (b < 0 && a > (rpyl_fx)(RPYL_I32_MAX + b)) return RPYL_I32_MAX;
    if (b > 0 && a < (rpyl_fx)(RPYL_I32_MIN + b)) return RPYL_I32_MIN;
    return (rpyl_fx)(a - b);
}

rpyl_fx rpyl_fx_mul(rpyl_fx a, rpyl_fx b) {
    rpyl_u32 aa;
    rpyl_u32 bb;
    rpyl_u32 hi;
    rpyl_u32 lo;
    rpyl_u32 magnitude;
    int negative;
    int overflowed;

    negative = ((a < 0) != (b < 0)) ? 1 : 0;
    aa = rpyl_fx_magnitude(a);
    bb = rpyl_fx_magnitude(b);
    rpyl_u32_mul_wide(aa, bb, &hi, &lo);

    overflowed = (hi > (rpyl_u32)0xFFFFU) ? 1 : 0;
    magnitude = (hi << 16) | (lo >> 16);
    return rpyl_fx_apply_sign_saturated(magnitude, negative, overflowed);
}

rpyl_fx rpyl_fx_div(rpyl_fx a, rpyl_fx b, int* ok) {
    rpyl_u32 aa;
    rpyl_u32 bb;
    rpyl_u32 n_hi;
    rpyl_u32 n_lo;
    rpyl_u32 q_hi;
    rpyl_u32 q_lo;
    int negative;

    if (b == 0) {
        if (ok) *ok = 0;
        return 0;
    }

    negative = ((a < 0) != (b < 0)) ? 1 : 0;
    aa = rpyl_fx_magnitude(a);
    bb = rpyl_fx_magnitude(b);
    n_hi = aa >> 16;
    n_lo = aa << 16;
    rpyl_u64_div_u32(n_hi, n_lo, bb, &q_hi, &q_lo);

    if (ok) *ok = 1;
    return rpyl_fx_apply_sign_saturated(q_lo, negative, q_hi != 0u);
}

rpyl_fx rpyl_fx_from_decimal_text(const char* text, int* ok) {
    const char* p;
    int sign;
    int whole_digits;
    int frac_digits;
    int whole_overflow;
    rpyl_u32 whole;
    rpyl_u32 frac;
    rpyl_u32 scale;
    rpyl_u32 frac_hi;
    rpyl_u32 frac_lo;
    rpyl_u32 frac_q_hi;
    rpyl_u32 frac_q_lo;
    rpyl_u32 magnitude;

    if (ok) *ok = 0;
    if (!text || !text[0]) return 0;

    p = text;
    sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    whole = 0u;
    whole_digits = 0;
    whole_overflow = 0;
    while (*p >= '0' && *p <= '9') {
        rpyl_u32 digit;
        digit = (rpyl_u32)(*p - '0');
        if (whole > 3276u || (whole == 3276u && digit > 8u)) whole_overflow = 1;
        else whole = whole * 10u + digit;
        whole_digits++;
        p++;
    }

    frac = 0u;
    scale = 1u;
    frac_digits = 0;
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9' && scale < 1000000u) {
            frac = frac * 10u + (rpyl_u32)(*p - '0');
            scale *= 10u;
            frac_digits++;
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            frac_digits++;
            p++;
        }
    }

    if (whole_digits == 0 && frac_digits == 0) return 0;
    if (*p != '\0') return 0;
    if (ok) *ok = 1;

    if (whole_overflow) return sign < 0 ? RPYL_I32_MIN : RPYL_I32_MAX;

    frac_q_hi = 0u;
    frac_q_lo = 0u;
    if (scale > 1u && frac > 0u) {
        rpyl_u32_mul_wide(frac, (rpyl_u32)RPYL_FX_ONE, &frac_hi, &frac_lo);
        rpyl_u64_div_u32(frac_hi, frac_lo, scale, &frac_q_hi, &frac_q_lo);
    }

    if (whole > 32768u) return sign < 0 ? RPYL_I32_MIN : RPYL_I32_MAX;
    if (whole == 32768u) {
        if (sign < 0 && frac_q_hi == 0u && frac_q_lo == 0u) return RPYL_I32_MIN;
        return sign < 0 ? RPYL_I32_MIN : RPYL_I32_MAX;
    }

    magnitude = (whole << 16) + frac_q_lo;
    return rpyl_fx_apply_sign_saturated(magnitude, sign < 0, frac_q_hi != 0u);
}

int rpyl_fx_parse(const char* text, rpyl_fx* out_value) {
    int ok;
    rpyl_fx v;
    v = rpyl_fx_from_decimal_text(text, &ok);
    if (!ok) return 0;
    if (out_value) *out_value = v;
    return 1;
}
