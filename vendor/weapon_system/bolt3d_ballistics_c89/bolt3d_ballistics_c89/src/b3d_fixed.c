#include "bolt3d/b3d_fixed.h"

static B3D_UFixed b3d_abs_to_u(B3D_Fixed value)
{
    B3D_UFixed result;

    if (value < 0) {
        if (value == B3D_FIXED_MIN) {
            result = ((B3D_UFixed)2147483647UL) + 1UL;
        } else {
            result = (B3D_UFixed)(-value);
        }
    } else {
        result = (B3D_UFixed)value;
    }

    return result;
}

static B3D_UFixed b3d_uadd_sat(B3D_UFixed a, B3D_UFixed b)
{
    B3D_UFixed max_value;

    max_value = (B3D_UFixed)B3D_FIXED_MAX;
    if (a > max_value) {
        return max_value;
    }
    if (b > max_value) {
        return max_value;
    }
    if (a > max_value - b) {
        return max_value;
    }
    return a + b;
}

static B3D_UFixed b3d_umul_shift_16_sat(B3D_UFixed a, B3D_UFixed b)
{
    B3D_UFixed ahi;
    B3D_UFixed alo;
    B3D_UFixed bhi;
    B3D_UFixed blo;
    B3D_UFixed result;
    B3D_UFixed term;
    B3D_UFixed max_value;

    max_value = (B3D_UFixed)B3D_FIXED_MAX;
    ahi = a >> B3D_FIXED_SHIFT;
    alo = a & B3D_FIXED_FRACTION_MASK;
    bhi = b >> B3D_FIXED_SHIFT;
    blo = b & B3D_FIXED_FRACTION_MASK;

    term = ahi * bhi;
    if (term > (max_value >> B3D_FIXED_SHIFT)) {
        return max_value;
    }

    result = term << B3D_FIXED_SHIFT;
    result = b3d_uadd_sat(result, ahi * blo);
    result = b3d_uadd_sat(result, bhi * alo);
    result = b3d_uadd_sat(result, (alo * blo) >> B3D_FIXED_SHIFT);

    return result;
}

static B3D_UFixed b3d_udiv_q16_sat(B3D_UFixed numerator, B3D_UFixed denominator)
{
    B3D_UFixed quotient;
    B3D_UFixed remainder;
    B3D_UFixed bit_value;
    B3D_UFixed max_value;
    int bit;

    max_value = (B3D_UFixed)B3D_FIXED_MAX;
    if (denominator == 0UL) {
        return max_value;
    }

    quotient = 0UL;
    remainder = 0UL;

    for (bit = 47; bit >= 0; --bit) {
        remainder = remainder << 1;
        if (bit >= B3D_FIXED_SHIFT) {
            bit_value = (numerator >> (bit - B3D_FIXED_SHIFT)) & 1UL;
            remainder |= bit_value;
        }

        if (remainder >= denominator) {
            remainder -= denominator;
            if (bit >= 31) {
                return max_value;
            }
            quotient |= (1UL << bit);
            if (quotient > max_value) {
                return max_value;
            }
        }
    }

    return quotient;
}

B3D_Fixed b3d_fixed_from_int(long value)
{
    long max_int;
    long min_int;

    max_int = B3D_FIXED_MAX >> B3D_FIXED_SHIFT;
    min_int = B3D_FIXED_MIN >> B3D_FIXED_SHIFT;

    if (value > max_int) {
        return B3D_FIXED_MAX;
    }
    if (value < min_int) {
        return B3D_FIXED_MIN;
    }
    return (B3D_Fixed)(value << B3D_FIXED_SHIFT);
}

long b3d_fixed_to_int(B3D_Fixed value)
{
    return value >> B3D_FIXED_SHIFT;
}

B3D_Fixed b3d_fixed_abs(B3D_Fixed value)
{
    if (value == B3D_FIXED_MIN) {
        return B3D_FIXED_MAX;
    }
    if (value < 0) {
        return -value;
    }
    return value;
}

B3D_Fixed b3d_fixed_neg(B3D_Fixed value)
{
    if (value == B3D_FIXED_MIN) {
        return B3D_FIXED_MAX;
    }
    return -value;
}

B3D_Fixed b3d_fixed_add_sat(B3D_Fixed a, B3D_Fixed b)
{
    if (b > 0 && a > B3D_FIXED_MAX - b) {
        return B3D_FIXED_MAX;
    }
    if (b < 0 && a < B3D_FIXED_MIN - b) {
        return B3D_FIXED_MIN;
    }
    return a + b;
}

B3D_Fixed b3d_fixed_sub_sat(B3D_Fixed a, B3D_Fixed b)
{
    if (b == B3D_FIXED_MIN) {
        if (a >= 0) {
            return B3D_FIXED_MAX;
        }
        return b3d_fixed_add_sat(a, B3D_FIXED_MAX);
    }
    return b3d_fixed_add_sat(a, -b);
}

B3D_Fixed b3d_fixed_mul(B3D_Fixed a, B3D_Fixed b)
{
    int negative;
    B3D_UFixed ua;
    B3D_UFixed ub;
    B3D_UFixed mag;

    negative = 0;
    if (a < 0) {
        negative = !negative;
    }
    if (b < 0) {
        negative = !negative;
    }

    ua = b3d_abs_to_u(a);
    ub = b3d_abs_to_u(b);
    mag = b3d_umul_shift_16_sat(ua, ub);

    if (negative) {
        if (mag > (B3D_UFixed)B3D_FIXED_MAX) {
            return B3D_FIXED_MIN;
        }
        return (B3D_Fixed)(-(B3D_Fixed)mag);
    }

    if (mag > (B3D_UFixed)B3D_FIXED_MAX) {
        return B3D_FIXED_MAX;
    }
    return (B3D_Fixed)mag;
}

B3D_Fixed b3d_fixed_div(B3D_Fixed a, B3D_Fixed b)
{
    int negative;
    B3D_UFixed ua;
    B3D_UFixed ub;
    B3D_UFixed mag;

    if (b == 0) {
        if (a < 0) {
            return B3D_FIXED_MIN;
        }
        return B3D_FIXED_MAX;
    }

    negative = 0;
    if (a < 0) {
        negative = !negative;
    }
    if (b < 0) {
        negative = !negative;
    }

    ua = b3d_abs_to_u(a);
    ub = b3d_abs_to_u(b);
    mag = b3d_udiv_q16_sat(ua, ub);

    if (negative) {
        if (mag > (B3D_UFixed)B3D_FIXED_MAX) {
            return B3D_FIXED_MIN;
        }
        return (B3D_Fixed)(-(B3D_Fixed)mag);
    }

    if (mag > (B3D_UFixed)B3D_FIXED_MAX) {
        return B3D_FIXED_MAX;
    }
    return (B3D_Fixed)mag;
}

B3D_Fixed b3d_fixed_sqrt(B3D_Fixed value)
{
    B3D_Fixed guess;
    B3D_Fixed next_guess;
    B3D_Fixed div_value;
    int i;

    if (value <= 0) {
        return 0;
    }

    if (value >= B3D_FIXED_ONE) {
        guess = value;
    } else {
        guess = B3D_FIXED_ONE;
    }

    for (i = 0; i < 16; ++i) {
        div_value = b3d_fixed_div(value, guess);
        next_guess = (guess + div_value) >> 1;
        if (next_guess == guess) {
            break;
        }
        if (next_guess <= 0) {
            break;
        }
        guess = next_guess;
    }

    return guess;
}

B3D_Fixed b3d_fixed_clamp(B3D_Fixed value, B3D_Fixed min_value, B3D_Fixed max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

B3D_Fixed b3d_fixed_lerp(B3D_Fixed a, B3D_Fixed b, B3D_Fixed t)
{
    B3D_Fixed delta;

    delta = b3d_fixed_sub_sat(b, a);
    return b3d_fixed_add_sat(a, b3d_fixed_mul(delta, t));
}

B3D_Fixed b3d_fixed_mul_int(B3D_Fixed value, long scale)
{
    B3D_Fixed result;
    long i;
    long count;
    int negative;

    if (scale == 0 || value == 0) {
        return 0;
    }

    negative = 0;
    if (scale < 0) {
        negative = 1;
        count = -scale;
    } else {
        count = scale;
    }

    result = 0;
    for (i = 0; i < count; ++i) {
        result = b3d_fixed_add_sat(result, value);
    }

    if (negative) {
        result = b3d_fixed_neg(result);
    }

    return result;
}
