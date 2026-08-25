#include "png_fixed89.h"

typedef unsigned int png_fixed89_u32;

static png_fixed89_u32 png_fixed89_abs_u32(png_fixed89 v)
{
    if (v >= 0)
        return (png_fixed89_u32)v;
    return (png_fixed89_u32)(-(v + 1)) + 1u;
}

static png_fixed89 png_fixed89_apply_sign(png_fixed89_u32 v, int neg)
{
    if (!neg)
    {
        if (v > 2147483647u)
            return 2147483647;
        return (png_fixed89)v;
    }
    if (v >= 2147483648u)
        return (-2147483647 - 1);
    return -(png_fixed89)v;
}

png_fixed89 png_fixed89_mul(png_fixed89 a, png_fixed89 b)
{
    png_fixed89_u32 ua;
    png_fixed89_u32 ub;
    png_fixed89_u32 ah;
    png_fixed89_u32 al;
    png_fixed89_u32 bh;
    png_fixed89_u32 bl;
    png_fixed89_u32 r;
    int neg;

    neg = ((a < 0) != (b < 0));
    ua = png_fixed89_abs_u32(a);
    ub = png_fixed89_abs_u32(b);
    ah = ua >> 16;
    al = ua & 65535u;
    bh = ub >> 16;
    bl = ub & 65535u;

    if (ah != 0u && bh > 32767u / ah)
        return neg ? (-2147483647 - 1) : 2147483647;

    r = (ah * bh) << 16;
    if (r > 0xFFFFFFFFu - ah * bl)
        return neg ? (-2147483647 - 1) : 2147483647;
    r += ah * bl;
    if (r > 0xFFFFFFFFu - al * bh)
        return neg ? (-2147483647 - 1) : 2147483647;
    r += al * bh;
    if (r > 0xFFFFFFFFu - ((al * bl) >> 16))
        return neg ? (-2147483647 - 1) : 2147483647;
    r += (al * bl) >> 16;
    return png_fixed89_apply_sign(r, neg);
}

png_fixed89 png_fixed89_from_ratio(signed int num, signed int den)
{
    png_fixed89_u32 un;
    png_fixed89_u32 ud;
    png_fixed89_u32 ip;
    png_fixed89_u32 rem;
    png_fixed89_u32 frac;
    unsigned int i;
    int neg;

    if (den == 0)
        return 0;
    neg = ((num < 0) != (den < 0));
    un = png_fixed89_abs_u32(num);
    ud = png_fixed89_abs_u32(den);
    ip = un / ud;
    rem = un % ud;
    if (ip > 32767u)
        return neg ? (-2147483647 - 1) : 2147483647;
    frac = 0u;
    for (i = 0u; i < 16u; ++i)
    {
        frac <<= 1;
        if (rem >= 0x80000000u)
            rem = ud;
        else
            rem <<= 1;
        if (rem >= ud)
        {
            rem -= ud;
            frac |= 1u;
        }
    }
    return png_fixed89_apply_sign((ip << 16) | frac, neg);
}

png_fixed89 png_fixed89_div(png_fixed89 a, png_fixed89 b)
{
    return png_fixed89_from_ratio(a, b);
}

int png_fixed89_parse_decimal(const char* s, png_fixed89* out_value)
{
    const char* p;
    png_fixed89_u32 whole;
    png_fixed89_u32 frac_num;
    png_fixed89_u32 frac_den;
    png_fixed89 result;
    png_fixed89 frac;
    int neg;
    int any;
    unsigned int frac_digits;

    if (!s || !s[0])
        return 0;
    p = s;
    neg = 0;
    if (*p == '+' || *p == '-')
    {
        neg = (*p == '-') ? 1 : 0;
        ++p;
    }
    whole = 0u;
    any = 0;
    while (*p >= '0' && *p <= '9')
    {
        png_fixed89_u32 d;
        d = (png_fixed89_u32)(*p - '0');
        if (whole > 32767u / 10u)
            return 0;
        whole = whole * 10u + d;
        if (whole > 32767u)
            return 0;
        any = 1;
        ++p;
    }
    frac_num = 0u;
    frac_den = 1u;
    frac_digits = 0u;
    if (*p == '.')
    {
        ++p;
        while (*p >= '0' && *p <= '9')
        {
            if (frac_digits < 6u)
            {
                frac_num = frac_num * 10u + (png_fixed89_u32)(*p - '0');
                frac_den *= 10u;
                ++frac_digits;
            }
            any = 1;
            ++p;
        }
    }
    if (!any || *p != '\0')
        return 0;
    result = (png_fixed89)(whole << 16);
    frac = png_fixed89_from_ratio((signed int)frac_num, (signed int)frac_den);
    if (result > 2147483647 - frac)
        return 0;
    result += frac;
    if (neg)
        result = -result;
    if (out_value)
        *out_value = result;
    return 1;
}

static png_fixed89 png_fixed89_log2_positive(png_fixed89 x)
{
    png_fixed89 y;
    png_fixed89 frac;
    unsigned int bit;
    int exponent;

    if (x <= 0)
        return (-2147483647 - 1);
    y = x;
    exponent = 0;
    while (y < PNG_FIXED89_ONE && exponent > -32767)
    {
        y <<= 1;
        --exponent;
    }
    while (y >= (PNG_FIXED89_ONE * 2) && exponent < 32767)
    {
        y >>= 1;
        ++exponent;
    }
    frac = 0;
    bit = 0x8000u;
    while (bit != 0u)
    {
        y = png_fixed89_mul(y, y);
        if (y >= (PNG_FIXED89_ONE * 2))
        {
            y >>= 1;
            frac |= (png_fixed89)bit;
        }
        bit >>= 1;
    }
    return (png_fixed89)(exponent * PNG_FIXED89_ONE) + frac;
}

static png_fixed89 png_fixed89_exp2(png_fixed89 x)
{
    static const png_fixed89 roots[16] = {
        92682, 77936, 71468, 68438,
        66971, 66250, 65892, 65714,
        65625, 65580, 65558, 65547,
        65542, 65539, 65537, 65537
    };
    signed int whole;
    png_fixed89 frac;
    png_fixed89 r;
    unsigned int bit;
    unsigned int i;

    whole = x / PNG_FIXED89_ONE;
    frac = x % PNG_FIXED89_ONE;
    if (frac < 0)
    {
        frac += PNG_FIXED89_ONE;
        --whole;
    }
    r = PNG_FIXED89_ONE;
    bit = 0x8000u;
    for (i = 0u; i < 16u; ++i)
    {
        if (((unsigned int)frac & bit) != 0u)
            r = png_fixed89_mul(r, roots[i]);
        bit >>= 1;
    }
    if (whole < 0)
    {
        signed int sh;
        sh = -whole;
        if (sh >= 31)
            return 0;
        r >>= sh;
    }
    else if (whole > 0)
    {
        if (whole >= 15)
            return 2147483647;
        r <<= whole;
    }
    return r;
}

png_fixed89 png_fixed89_pow_unit(png_fixed89 x, png_fixed89 exponent)
{
    png_fixed89 lg;
    png_fixed89 y;
    if (x <= 0)
        return 0;
    if (x >= PNG_FIXED89_ONE)
        return PNG_FIXED89_ONE;
    lg = png_fixed89_log2_positive(x);
    y = png_fixed89_mul(lg, exponent);
    return png_fixed89_exp2(y);
}


png_fixed89 png_fixed89_pow_positive(png_fixed89 base, png_fixed89 exponent)
{
    png_fixed89 lg;
    png_fixed89 y;
    if (base <= 0)
        return 0;
    lg = png_fixed89_log2_positive(base);
    y = png_fixed89_mul(lg, exponent);
    return png_fixed89_exp2(y);
}

png_fixed89 png_fixed89_exp(png_fixed89 x)
{
    /* log2(e) in Q16.16. */
    return png_fixed89_exp2(png_fixed89_mul(x, 94548));
}

png_fixed89 png_fixed89_sinh(png_fixed89 x)
{
    png_fixed89 ep;
    png_fixed89 en;
    ep = png_fixed89_exp(x);
    en = png_fixed89_exp(-x);
    if (ep >= en)
        return (ep - en) / 2;
    return -((en - ep) / 2);
}

unsigned int png_fixed89_unit_to_u16(png_fixed89 x)
{
    unsigned int ux;
    if (x <= 0)
        return 0u;
    if (x >= PNG_FIXED89_ONE)
        return 65535u;
    ux = (unsigned int)x;
    if (ux > 32768u)
        return ux - 1u;
    return ux;
}

unsigned int png_fixed89_unit_to_u8(png_fixed89 x)
{
    unsigned int ux;
    unsigned int v;
    if (x <= 0)
        return 0u;
    if (x >= PNG_FIXED89_ONE)
        return 255u;
    ux = (unsigned int)x;
    v = (ux * 255u + 32768u) >> 16;
    if (v > 255u)
        v = 255u;
    return v;
}
