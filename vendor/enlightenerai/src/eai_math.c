#include "eai_math.h"

static unsigned long eai_fx_abs_u(EAI_Fixed v)
{
    if (v < 0)
    {
        return (unsigned long)(-(v + 1)) + 1ul;
    }
    return (unsigned long)v;
}

EAI_Fixed eai_fx_from_ratio(EAI_Fixed numerator, EAI_Fixed denominator)
{
    if (denominator == 0)
    {
        return (numerator < 0) ? EAI_FX_MIN : EAI_FX_MAX;
    }
    return eai_fx_div(eai_fx_from_int(numerator), eai_fx_from_int(denominator));
}

EAI_Fixed eai_fx_from_int(EAI_Fixed value)
{
    if (value > 32767L)
    {
        return EAI_FX_MAX;
    }
    if (value < -32768L)
    {
        return EAI_FX_MIN;
    }
    return (EAI_Fixed)(value * EAI_FX_ONE);
}

EAI_Fixed eai_fx_add_sat(EAI_Fixed a, EAI_Fixed b)
{
    if (b > 0 && a > (EAI_Fixed)(EAI_FX_MAX - b))
    {
        return EAI_FX_MAX;
    }
    if (b < 0 && a < (EAI_Fixed)(EAI_FX_MIN - b))
    {
        return EAI_FX_MIN;
    }
    return (EAI_Fixed)(a + b);
}

EAI_Fixed eai_fx_sub_sat(EAI_Fixed a, EAI_Fixed b)
{
    if (b > 0 && a < (EAI_Fixed)(EAI_FX_MIN + b))
    {
        return EAI_FX_MIN;
    }
    if (b < 0 && a > (EAI_Fixed)(EAI_FX_MAX + b))
    {
        return EAI_FX_MAX;
    }
    return (EAI_Fixed)(a - b);
}

EAI_Fixed eai_fx_madd_sat(EAI_Fixed acc, EAI_Fixed a, EAI_Fixed b)
{
    return eai_fx_add_sat(acc, eai_fx_mul(a, b));
}

EAI_Fixed eai_fx_mul(EAI_Fixed a, EAI_Fixed b)
{
    int neg;
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long part;
    unsigned long out;

    if (a == 0 || b == 0)
    {
        return 0;
    }

    neg = ((a < 0) != (b < 0));
    ua = eai_fx_abs_u(a);
    ub = eai_fx_abs_u(b);

    ah = ua >> 16;
    al = ua & 0xFFFFul;
    bh = ub >> 16;
    bl = ub & 0xFFFFul;

    part = ah * bh;
    if (part > 0x7FFFul)
    {
        return neg ? EAI_FX_MIN : EAI_FX_MAX;
    }

    out = part << 16;

#define EAI_FX_ADD_SAT(v) \
    do { \
        unsigned long add_value__; \
        add_value__ = (v); \
        if (out > 0x7FFFFFFFul - add_value__) { \
            return neg ? EAI_FX_MIN : EAI_FX_MAX; \
        } \
        out += add_value__; \
    } while (0)

    EAI_FX_ADD_SAT(ah * bl);
    EAI_FX_ADD_SAT(bh * al);
    EAI_FX_ADD_SAT((al * bl) >> 16);

#undef EAI_FX_ADD_SAT

    if (neg)
    {
        if (out >= 0x80000000ul)
        {
            return EAI_FX_MIN;
        }
        return (EAI_Fixed)(-(EAI_Fixed)out);
    }

    if (out > 0x7FFFFFFFul)
    {
        return EAI_FX_MAX;
    }
    return (EAI_Fixed)out;
}

EAI_Fixed eai_fx_div(EAI_Fixed a, EAI_Fixed b)
{
    int neg;
    unsigned long ua;
    unsigned long ub;
    unsigned long q;
    unsigned long rem;
    int i;

    if (b == 0)
    {
        return (a < 0) ? EAI_FX_MIN : EAI_FX_MAX;
    }
    if (a == 0)
    {
        return 0;
    }

    neg = ((a < 0) != (b < 0));
    ua = eai_fx_abs_u(a);
    ub = eai_fx_abs_u(b);
    q = 0ul;
    rem = 0ul;

    for (i = 47; i >= 0; --i)
    {
        rem <<= 1;
        if (i >= 16)
        {
            rem |= (ua >> (i - 16)) & 1ul;
        }

        if (rem >= ub)
        {
            rem -= ub;
            if (i >= 31)
            {
                return neg ? EAI_FX_MIN : EAI_FX_MAX;
            }
            q |= (1ul << i);
        }
    }

    if (neg)
    {
        if (q >= 0x80000000ul)
        {
            return EAI_FX_MIN;
        }
        return (EAI_Fixed)(-(EAI_Fixed)q);
    }

    if (q > 0x7FFFFFFFul)
    {
        return EAI_FX_MAX;
    }
    return (EAI_Fixed)q;
}

EAI_Fixed eai_fx_sqrt(EAI_Fixed value)
{
    EAI_Fixed x;
    EAI_Fixed last;
    int i;

    if (value <= 0)
    {
        return 0;
    }

    if (value <= EAI_FX_ONE)
    {
        x = EAI_FX_ONE;
    }
    else
    {
        x = value;
    }

    last = 0;
    for (i = 0; i < 16; ++i)
    {
        EAI_Fixed div_value;
        div_value = eai_fx_div(value, x);
        last = x;
        x = (EAI_Fixed)(eai_fx_add_sat(x, div_value) / 2);
        if (x == last || x == last + 1 || x + 1 == last)
        {
            break;
        }
    }

    return x;
}

EAI_Fixed eai_fx_clamp(EAI_Fixed value, EAI_Fixed min_value, EAI_Fixed max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

void eai_vec3_set(EAI_Vec3* v, EAI_Fixed x, EAI_Fixed y, EAI_Fixed z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

void eai_vec3_add(EAI_Vec3* out_v, const EAI_Vec3* a, const EAI_Vec3* b)
{
    out_v->x = eai_fx_add_sat(a->x, b->x);
    out_v->y = eai_fx_add_sat(a->y, b->y);
    out_v->z = eai_fx_add_sat(a->z, b->z);
}

void eai_vec3_sub(EAI_Vec3* out_v, const EAI_Vec3* a, const EAI_Vec3* b)
{
    out_v->x = eai_fx_sub_sat(a->x, b->x);
    out_v->y = eai_fx_sub_sat(a->y, b->y);
    out_v->z = eai_fx_sub_sat(a->z, b->z);
}

void eai_vec3_scale(EAI_Vec3* out_v, const EAI_Vec3* a, EAI_Fixed s)
{
    out_v->x = eai_fx_mul(a->x, s);
    out_v->y = eai_fx_mul(a->y, s);
    out_v->z = eai_fx_mul(a->z, s);
}

EAI_Fixed eai_vec3_dot(const EAI_Vec3* a, const EAI_Vec3* b)
{
    EAI_Fixed out_value;

    out_value = eai_fx_mul(a->x, b->x);
    out_value = eai_fx_madd_sat(out_value, a->y, b->y);
    out_value = eai_fx_madd_sat(out_value, a->z, b->z);
    return out_value;
}

EAI_Fixed eai_vec3_len_sq(const EAI_Vec3* a)
{
    return eai_vec3_dot(a, a);
}

EAI_Fixed eai_vec3_len(const EAI_Vec3* a)
{
    return eai_fx_sqrt(eai_vec3_len_sq(a));
}

EAI_Fixed eai_vec3_dist_sq(const EAI_Vec3* a, const EAI_Vec3* b)
{
    EAI_Vec3 d;
    eai_vec3_sub(&d, a, b);
    return eai_vec3_len_sq(&d);
}

EAI_Fixed eai_vec3_dist(const EAI_Vec3* a, const EAI_Vec3* b)
{
    return eai_fx_sqrt(eai_vec3_dist_sq(a, b));
}

void eai_vec3_normalize(EAI_Vec3* out_v, const EAI_Vec3* a)
{
    EAI_Fixed len;

    len = eai_vec3_len(a);
    if (len > EAI_FX_EPSILON)
    {
        out_v->x = eai_fx_div(a->x, len);
        out_v->y = eai_fx_div(a->y, len);
        out_v->z = eai_fx_div(a->z, len);
    }
    else
    {
        out_v->x = EAI_FX_ONE;
        out_v->y = EAI_FX_ZERO;
        out_v->z = EAI_FX_ZERO;
    }
}

EAI_Fixed eai_clampf(EAI_Fixed value, EAI_Fixed min_value, EAI_Fixed max_value)
{
    return eai_fx_clamp(value, min_value, max_value);
}
