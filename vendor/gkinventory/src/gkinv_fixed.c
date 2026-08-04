#include "gkinv_fixed.h"

gkinv_fx gkinv_fx_from_int(gkinv_i32 value)
{
    if (value > (GKINV_FX_MAX >> GKINV_FX_SHIFT)) {
        return GKINV_FX_MAX;
    }
    if (value < (GKINV_FX_MIN >> GKINV_FX_SHIFT)) {
        return GKINV_FX_MIN;
    }
    return (gkinv_fx)(value << GKINV_FX_SHIFT);
}

gkinv_i32 gkinv_fx_to_int_floor(gkinv_fx value)
{
    return (gkinv_i32)(value >> GKINV_FX_SHIFT);
}

gkinv_fx gkinv_fx_from_ratio(gkinv_i32 numerator, gkinv_i32 denominator)
{
    gkinv_i32 whole;
    gkinv_i32 rem;
    gkinv_i32 scaled;
    gkinv_i32 sign;

    if (denominator == 0) {
        if (numerator < 0) {
            return GKINV_FX_MIN;
        }
        return GKINV_FX_MAX;
    }

    sign = 1;
    if (numerator < 0) {
        numerator = -numerator;
        sign = -sign;
    }
    if (denominator < 0) {
        denominator = -denominator;
        sign = -sign;
    }

    whole = numerator / denominator;
    rem = numerator % denominator;

    if (whole > (GKINV_FX_MAX >> GKINV_FX_SHIFT)) {
        return (sign < 0) ? GKINV_FX_MIN : GKINV_FX_MAX;
    }

    scaled = (gkinv_i32)((whole << GKINV_FX_SHIFT) + ((rem << GKINV_FX_SHIFT) / denominator));
    if (sign < 0) {
        scaled = -scaled;
    }
    return (gkinv_fx)scaled;
}

gkinv_fx gkinv_fx_add_sat(gkinv_fx a, gkinv_fx b)
{
    if (b > 0 && a > (GKINV_FX_MAX - b)) {
        return GKINV_FX_MAX;
    }
    if (b < 0 && a < (GKINV_FX_MIN - b)) {
        return GKINV_FX_MIN;
    }
    return (gkinv_fx)(a + b);
}

gkinv_fx gkinv_fx_sub_sat(gkinv_fx a, gkinv_fx b)
{
    if (b < 0 && a > (GKINV_FX_MAX + b)) {
        return GKINV_FX_MAX;
    }
    if (b > 0 && a < (GKINV_FX_MIN + b)) {
        return GKINV_FX_MIN;
    }
    return (gkinv_fx)(a - b);
}

gkinv_fx gkinv_fx_mul_int_sat(gkinv_fx value, gkinv_u16 count)
{
    gkinv_fx result;
    gkinv_u16 i;

    result = GKINV_FX_ZERO;
    i = 0u;
    while (i < count) {
        result = gkinv_fx_add_sat(result, value);
        if (result == GKINV_FX_MAX || result == GKINV_FX_MIN) {
            return result;
        }
        i = (gkinv_u16)(i + 1u);
    }
    return result;
}
