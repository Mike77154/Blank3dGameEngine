#include "gmove89_math.h"

static const GMoveFx89 gmove89_sine_table[256] = {
    0L, 1608L, 3216L, 4821L, 6424L, 8022L, 9616L, 11204L,
    12785L, 14359L, 15924L, 17479L, 19024L, 20557L, 22078L, 23586L,
    25080L, 26558L, 28020L, 29466L, 30893L, 32303L, 33692L, 35062L,
    36410L, 37736L, 39040L, 40320L, 41576L, 42806L, 44011L, 45190L,
    46341L, 47464L, 48559L, 49624L, 50660L, 51665L, 52639L, 53581L,
    54491L, 55368L, 56212L, 57022L, 57798L, 58538L, 59244L, 59914L,
    60547L, 61145L, 61705L, 62228L, 62714L, 63162L, 63572L, 63944L,
    64277L, 64571L, 64827L, 65043L, 65220L, 65358L, 65457L, 65516L,
    65536L, 65516L, 65457L, 65358L, 65220L, 65043L, 64827L, 64571L,
    64277L, 63944L, 63572L, 63162L, 62714L, 62228L, 61705L, 61145L,
    60547L, 59914L, 59244L, 58538L, 57798L, 57022L, 56212L, 55368L,
    54491L, 53581L, 52639L, 51665L, 50660L, 49624L, 48559L, 47464L,
    46341L, 45190L, 44011L, 42806L, 41576L, 40320L, 39040L, 37736L,
    36410L, 35062L, 33692L, 32303L, 30893L, 29466L, 28020L, 26558L,
    25080L, 23586L, 22078L, 20557L, 19024L, 17479L, 15924L, 14359L,
    12785L, 11204L, 9616L, 8022L, 6424L, 4821L, 3216L, 1608L,
    0L, -1608L, -3216L, -4821L, -6424L, -8022L, -9616L, -11204L,
    -12785L, -14359L, -15924L, -17479L, -19024L, -20557L, -22078L, -23586L,
    -25080L, -26558L, -28020L, -29466L, -30893L, -32303L, -33692L, -35062L,
    -36410L, -37736L, -39040L, -40320L, -41576L, -42806L, -44011L, -45190L,
    -46341L, -47464L, -48559L, -49624L, -50660L, -51665L, -52639L, -53581L,
    -54491L, -55368L, -56212L, -57022L, -57798L, -58538L, -59244L, -59914L,
    -60547L, -61145L, -61705L, -62228L, -62714L, -63162L, -63572L, -63944L,
    -64277L, -64571L, -64827L, -65043L, -65220L, -65358L, -65457L, -65516L,
    -65536L, -65516L, -65457L, -65358L, -65220L, -65043L, -64827L, -64571L,
    -64277L, -63944L, -63572L, -63162L, -62714L, -62228L, -61705L, -61145L,
    -60547L, -59914L, -59244L, -58538L, -57798L, -57022L, -56212L, -55368L,
    -54491L, -53581L, -52639L, -51665L, -50660L, -49624L, -48559L, -47464L,
    -46341L, -45190L, -44011L, -42806L, -41576L, -40320L, -39040L, -37736L,
    -36410L, -35062L, -33692L, -32303L, -30893L, -29466L, -28020L, -26558L,
    -25080L, -23586L, -22078L, -20557L, -19024L, -17479L, -15924L, -14359L,
    -12785L, -11204L, -9616L, -8022L, -6424L, -4821L, -3216L, -1608L
};

static unsigned long gmove89_abs_u32(GMoveFx89 value)
{
    if (value >= 0L) {
        return (unsigned long)value;
    }

    return (unsigned long)(-(value + 1L)) + 1UL;
}

static GMoveFx89 gmove89_apply_sign(
    unsigned long magnitude,
    int negative
)
{
    if (magnitude > 2147483647UL) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }

    if (negative) {
        return -(GMoveFx89)magnitude;
    }

    return (GMoveFx89)magnitude;
}

GMoveFx89 gmove89_fx_clamp(
    GMoveFx89 value,
    GMoveFx89 low,
    GMoveFx89 high
)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

GMoveFx89 gmove89_fx_abs(GMoveFx89 value)
{
    if (value == GMOVE89_FX_MIN) {
        return GMOVE89_FX_MAX;
    }
    if (value < 0L) {
        return -value;
    }
    return value;
}

GMoveFx89 gmove89_fx_add(GMoveFx89 a, GMoveFx89 b)
{
    if (b > 0L && a > GMOVE89_FX_MAX - b) {
        return GMOVE89_FX_MAX;
    }
    if (b < 0L && a < GMOVE89_FX_MIN - b) {
        return GMOVE89_FX_MIN;
    }
    return a + b;
}

GMoveFx89 gmove89_fx_sub(GMoveFx89 a, GMoveFx89 b)
{
    if (b == GMOVE89_FX_MIN) {
        if (a >= 0L) {
            return GMOVE89_FX_MAX;
        }
        return gmove89_fx_add(a, GMOVE89_FX_MAX);
    }
    return gmove89_fx_add(a, -b);
}

GMoveFx89 gmove89_fx_mul(GMoveFx89 a, GMoveFx89 b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long ahi;
    unsigned long alo;
    unsigned long bhi;
    unsigned long blo;
    unsigned long high;
    unsigned long term;
    unsigned long result;
    int negative;

    if (a == 0L || b == 0L) {
        return 0L;
    }

    negative = ((a < 0L) != (b < 0L));
    ua = gmove89_abs_u32(a);
    ub = gmove89_abs_u32(b);

    if (ua > 2147483648UL) {
        ua = 2147483648UL;
    }
    if (ub > 2147483648UL) {
        ub = 2147483648UL;
    }

    ahi = ua >> 16;
    alo = ua & 65535UL;
    bhi = ub >> 16;
    blo = ub & 65535UL;

    high = ahi * bhi;
    if (high > 32767UL) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }

    result = high << 16;

    term = ahi * blo;
    if (result > 2147483647UL - term) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }
    result += term;

    term = alo * bhi;
    if (result > 2147483647UL - term) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }
    result += term;

    term = (alo * blo) >> 16;
    if (result > 2147483647UL - term) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }
    result += term;

    return gmove89_apply_sign(result, negative);
}

GMoveFx89 gmove89_fx_div(GMoveFx89 a, GMoveFx89 b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long integer_part;
    unsigned long remainder;
    unsigned long fraction;
    unsigned long result;
    int bit;
    int negative;

    if (b == 0L) {
        return a < 0L ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }
    if (a == 0L) {
        return 0L;
    }

    negative = ((a < 0L) != (b < 0L));
    ua = gmove89_abs_u32(a);
    ub = gmove89_abs_u32(b);

    if (ub == 0UL) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }

    integer_part = ua / ub;
    remainder = ua % ub;

    if (integer_part > 32767UL) {
        return negative ? GMOVE89_FX_MIN : GMOVE89_FX_MAX;
    }

    fraction = 0UL;
    for (bit = 0; bit < 16; ++bit) {
        fraction <<= 1;
        if (remainder >= ub - remainder) {
            remainder = remainder - (ub - remainder);
            fraction |= 1UL;
        } else {
            remainder += remainder;
        }
    }

    result = (integer_part << 16) | fraction;
    return gmove89_apply_sign(result, negative);
}

GMoveVec3_89 gmove89_vec3(
    GMoveFx89 x,
    GMoveFx89 y,
    GMoveFx89 z
)
{
    GMoveVec3_89 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

GMoveVec3_89 gmove89_vec3_zero(void)
{
    return gmove89_vec3(0L, 0L, 0L);
}

GMoveVec3_89 gmove89_vec3_add(
    GMoveVec3_89 a,
    GMoveVec3_89 b
)
{
    return gmove89_vec3(
        gmove89_fx_add(a.x, b.x),
        gmove89_fx_add(a.y, b.y),
        gmove89_fx_add(a.z, b.z)
    );
}

GMoveVec3_89 gmove89_vec3_sub(
    GMoveVec3_89 a,
    GMoveVec3_89 b
)
{
    return gmove89_vec3(
        gmove89_fx_sub(a.x, b.x),
        gmove89_fx_sub(a.y, b.y),
        gmove89_fx_sub(a.z, b.z)
    );
}

GMoveVec3_89 gmove89_vec3_scale(
    GMoveVec3_89 value,
    GMoveFx89 scalar
)
{
    return gmove89_vec3(
        gmove89_fx_mul(value.x, scalar),
        gmove89_fx_mul(value.y, scalar),
        gmove89_fx_mul(value.z, scalar)
    );
}

GMoveFx89 gmove89_vec3_dot(
    GMoveVec3_89 a,
    GMoveVec3_89 b
)
{
    GMoveFx89 result;
    result = gmove89_fx_mul(a.x, b.x);
    result = gmove89_fx_add(result, gmove89_fx_mul(a.y, b.y));
    result = gmove89_fx_add(result, gmove89_fx_mul(a.z, b.z));
    return result;
}

GMoveVec3_89 gmove89_vec3_cross(
    GMoveVec3_89 a,
    GMoveVec3_89 b
)
{
    return gmove89_vec3(
        gmove89_fx_sub(
            gmove89_fx_mul(a.y, b.z),
            gmove89_fx_mul(a.z, b.y)
        ),
        gmove89_fx_sub(
            gmove89_fx_mul(a.z, b.x),
            gmove89_fx_mul(a.x, b.z)
        ),
        gmove89_fx_sub(
            gmove89_fx_mul(a.x, b.y),
            gmove89_fx_mul(a.y, b.x)
        )
    );
}

GMoveFx89 gmove89_vec3_length_approx(GMoveVec3_89 value)
{
    GMoveFx89 a;
    GMoveFx89 b;
    GMoveFx89 c;
    GMoveFx89 temp;
    GMoveFx89 result;

    a = gmove89_fx_abs(value.x);
    b = gmove89_fx_abs(value.y);
    c = gmove89_fx_abs(value.z);

    if (a < b) {
        temp = a;
        a = b;
        b = temp;
    }
    if (b < c) {
        temp = b;
        b = c;
        c = temp;
    }
    if (a < b) {
        temp = a;
        a = b;
        b = temp;
    }

    result = a;
    result = gmove89_fx_add(result, (b >> 2) + (b >> 3));
    result = gmove89_fx_add(result, (c >> 3) + (c >> 4));
    return result;
}

GMoveVec3_89 gmove89_vec3_normalize_approx(GMoveVec3_89 value)
{
    GMoveFx89 length;

    length = gmove89_vec3_length_approx(value);
    if (length <= 0L) {
        return gmove89_vec3_zero();
    }

    return gmove89_vec3(
        gmove89_fx_div(value.x, length),
        gmove89_fx_div(value.y, length),
        gmove89_fx_div(value.z, length)
    );
}

int gmove89_vec3_is_near_zero(
    GMoveVec3_89 value,
    GMoveFx89 epsilon
)
{
    if (gmove89_fx_abs(value.x) > epsilon) {
        return GMOVE89_FALSE;
    }
    if (gmove89_fx_abs(value.y) > epsilon) {
        return GMOVE89_FALSE;
    }
    if (gmove89_fx_abs(value.z) > epsilon) {
        return GMOVE89_FALSE;
    }
    return GMOVE89_TRUE;
}

GMoveFx89 gmove89_wrap_turn(GMoveFx89 phase)
{
    phase %= GMOVE89_FX_ONE;
    if (phase < 0L) {
        phase += GMOVE89_FX_ONE;
    }
    return phase;
}

GMoveFx89 gmove89_sin_turn(GMoveFx89 phase)
{
    unsigned long index;
    phase = gmove89_wrap_turn(phase);
    index = ((unsigned long)phase >> 8) & 255UL;
    return gmove89_sine_table[index];
}

GMoveFx89 gmove89_cos_turn(GMoveFx89 phase)
{
    return gmove89_sin_turn(
        gmove89_fx_add(phase, GMOVE89_FX_QUARTER)
    );
}

GMoveFx89 gmove89_triangle_turn(GMoveFx89 phase)
{
    phase = gmove89_wrap_turn(phase);

    if (phase < GMOVE89_FX_QUARTER) {
        return phase << 2;
    }

    if (phase < GMOVE89_FX_HALF) {
        return GMOVE89_FX_ONE -
            ((phase - GMOVE89_FX_QUARTER) << 2);
    }

    if (phase < (GMOVE89_FX_HALF + GMOVE89_FX_QUARTER)) {
        return -((phase - GMOVE89_FX_HALF) << 2);
    }

    return -GMOVE89_FX_ONE +
        ((phase - (GMOVE89_FX_HALF + GMOVE89_FX_QUARTER)) << 2);
}
