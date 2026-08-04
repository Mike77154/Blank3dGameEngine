#include "fpi_value.h"
#include "fpi_util.h"
#include <limits.h>
#include <stdio.h>

#define FPI_FIXED_MAX_VALUE 2147483647L
#define FPI_FIXED_MIN_VALUE (-2147483647L - 1L)
#define FPI_FIXED_NEG_LIMIT 2147483648UL

static unsigned long fixed_abs_u(FPI_Fixed value) {
    if (value >= 0L) return (unsigned long)value;
    if (value == FPI_FIXED_MIN_VALUE) return FPI_FIXED_NEG_LIMIT;
    return (unsigned long)(-value);
}

static FPI_Fixed fixed_apply_sign(unsigned long magnitude, int negative) {
    if (negative) {
        if (magnitude >= FPI_FIXED_NEG_LIMIT) return FPI_FIXED_MIN_VALUE;
        return -(FPI_Fixed)magnitude;
    }
    if (magnitude > (unsigned long)FPI_FIXED_MAX_VALUE) return FPI_FIXED_MAX_VALUE;
    return (FPI_Fixed)magnitude;
}

FPI_Fixed fpi_fixed_from_int(long value) {
    if (value > 32767L) return FPI_FIXED_MAX_VALUE;
    if (value < -32768L) return FPI_FIXED_MIN_VALUE;
    return value * FPI_FIXED_ONE;
}

long fpi_fixed_to_int(FPI_Fixed value) {
    unsigned long magnitude;
    if (value >= 0L) return value / FPI_FIXED_ONE;
    magnitude = fixed_abs_u(value);
    return -(long)(magnitude / (unsigned long)FPI_FIXED_ONE);
}

int fpi_fixed_parse(const char* text, FPI_Fixed* out_value) {
    int i;
    int negative;
    int digits;
    int frac_digits;
    unsigned long integer_part;
    unsigned long frac_num;
    unsigned long frac_den;
    unsigned long frac_bits;
    unsigned long magnitude;
    int bit;

    if (!text || !out_value) return FPI_ERR_ARGUMENT;
    i = 0;
    while (text[i] == ' ' || text[i] == '\t') i++;
    negative = 0;
    if (text[i] == '-' || text[i] == '+') {
        negative = text[i] == '-';
        i++;
    }
    digits = 0;
    integer_part = 0UL;
    while (fpi_ascii_is_digit(text[i])) {
        unsigned long digit;
        digit = (unsigned long)(text[i] - '0');
        if (integer_part > 32768UL / 10UL ||
            (integer_part == 32768UL / 10UL && digit > 32768UL % 10UL))
            return FPI_ERR_NUMBER_OVERFLOW;
        integer_part = integer_part * 10UL + digit;
        digits++;
        i++;
    }

    frac_num = 0UL;
    frac_den = 1UL;
    frac_digits = 0;
    if (text[i] == '.') {
        i++;
        while (fpi_ascii_is_digit(text[i])) {
            if (frac_digits >= 9) return FPI_ERR_INVALID_NUMBER;
            frac_num = frac_num * 10UL + (unsigned long)(text[i] - '0');
            frac_den *= 10UL;
            frac_digits++;
            digits++;
            i++;
        }
    }

    while (text[i] == ' ' || text[i] == '\t') i++;
    if (digits == 0 || text[i] != '\0') return FPI_ERR_INVALID_NUMBER;

    if ((!negative && integer_part > 32767UL) ||
        (negative && integer_part > 32768UL)) return FPI_ERR_NUMBER_OVERFLOW;

    frac_bits = 0UL;
    if (frac_digits > 0) {
        for (bit = 0; bit < FPI_FIXED_FRAC_BITS; bit++) {
            frac_bits <<= 1;
            if (frac_num >= frac_den - frac_num) {
                frac_bits |= 1UL;
                frac_num = frac_num - (frac_den - frac_num);
            } else {
                frac_num += frac_num;
            }
        }
    }

    magnitude = integer_part * (unsigned long)FPI_FIXED_ONE + frac_bits;
    if (!negative && magnitude > (unsigned long)FPI_FIXED_MAX_VALUE)
        return FPI_ERR_NUMBER_OVERFLOW;
    if (negative && magnitude > FPI_FIXED_NEG_LIMIT)
        return FPI_ERR_NUMBER_OVERFLOW;
    *out_value = fixed_apply_sign(magnitude, negative);
    return FPI_OK;
}

static int append_char(char* out, int cap, int* pos, char c) {
    if (!out || !pos || *pos >= cap - 1) return 0;
    out[*pos] = c;
    (*pos)++;
    out[*pos] = '\0';
    return 1;
}

static int append_ulong(char* out, int cap, int* pos, unsigned long value) {
    char digits[16];
    int n;
    int i;
    n = 0;
    do {
        digits[n++] = (char)('0' + (value % 10UL));
        value /= 10UL;
    } while (value && n < (int)sizeof(digits));
    for (i = n - 1; i >= 0; i--) {
        if (!append_char(out, cap, pos, digits[i])) return 0;
    }
    return 1;
}

int fpi_fixed_format(FPI_Fixed value, char* out, int cap, int decimals) {
    unsigned long magnitude;
    unsigned long integer_part;
    unsigned long fraction;
    int negative;
    int pos;
    int i;
    unsigned long digit;

    if (!out || cap <= 0) return FPI_ERR_ARGUMENT;
    if (decimals < 0) decimals = 0;
    if (decimals > 6) decimals = 6;
    out[0] = '\0';
    pos = 0;
    negative = value < 0L;
    magnitude = fixed_abs_u(value);
    integer_part = magnitude >> FPI_FIXED_FRAC_BITS;
    fraction = magnitude & (unsigned long)(FPI_FIXED_ONE - 1L);
    if (negative && !append_char(out, cap, &pos, '-')) return FPI_ERR_VALUE_TOO_LONG;
    if (!append_ulong(out, cap, &pos, integer_part)) return FPI_ERR_VALUE_TOO_LONG;
    if (decimals == 0) return pos;
    if (!append_char(out, cap, &pos, '.')) return FPI_ERR_VALUE_TOO_LONG;
    for (i = 0; i < decimals; i++) {
        fraction *= 10UL;
        digit = fraction / (unsigned long)FPI_FIXED_ONE;
        fraction %= (unsigned long)FPI_FIXED_ONE;
        if (!append_char(out, cap, &pos, (char)('0' + digit))) return FPI_ERR_VALUE_TOO_LONG;
    }
    return pos;
}

FPI_Fixed fpi_fixed_add_sat(FPI_Fixed a, FPI_Fixed b) {
    if (b > 0L && a > FPI_FIXED_MAX_VALUE - b) return FPI_FIXED_MAX_VALUE;
    if (b < 0L && a < FPI_FIXED_MIN_VALUE - b) return FPI_FIXED_MIN_VALUE;
    return a + b;
}

FPI_Fixed fpi_fixed_sub_sat(FPI_Fixed a, FPI_Fixed b) {
    if (b == FPI_FIXED_MIN_VALUE) return a >= 0L ? FPI_FIXED_MAX_VALUE : a - b;
    return fpi_fixed_add_sat(a, -b);
}

static int add_u_sat(unsigned long* value, unsigned long add, unsigned long limit) {
    if (*value > limit - add) { *value = limit; return 0; }
    *value += add;
    return 1;
}

FPI_Fixed fpi_fixed_mul_sat(FPI_Fixed a, FPI_Fixed b) {
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long result;
    unsigned long term;
    unsigned long limit;
    int negative;

    negative = (a < 0L) != (b < 0L);
    ua = fixed_abs_u(a);
    ub = fixed_abs_u(b);
    ah = ua >> 16;
    al = ua & 65535UL;
    bh = ub >> 16;
    bl = ub & 65535UL;
    limit = negative ? FPI_FIXED_NEG_LIMIT : (unsigned long)FPI_FIXED_MAX_VALUE;
    if (ah != 0UL && bh > (limit >> 16) / ah) return fixed_apply_sign(limit, negative);
    result = (ah * bh) << 16;
    term = ah * bl;
    if (!add_u_sat(&result, term, limit)) return fixed_apply_sign(limit, negative);
    term = al * bh;
    if (!add_u_sat(&result, term, limit)) return fixed_apply_sign(limit, negative);
    term = (al * bl) >> 16;
    if (!add_u_sat(&result, term, limit)) return fixed_apply_sign(limit, negative);
    return fixed_apply_sign(result, negative);
}

FPI_Fixed fpi_fixed_div_sat(FPI_Fixed a, FPI_Fixed b, int* ok) {
    unsigned long ua;
    unsigned long ub;
    unsigned long integer_part;
    unsigned long remainder;
    unsigned long fraction;
    unsigned long result;
    unsigned long limit;
    int negative;
    int i;

    if (ok) *ok = 0;
    if (b == 0L) return a < 0L ? FPI_FIXED_MIN_VALUE : FPI_FIXED_MAX_VALUE;
    negative = (a < 0L) != (b < 0L);
    ua = fixed_abs_u(a);
    ub = fixed_abs_u(b);
    limit = negative ? FPI_FIXED_NEG_LIMIT : (unsigned long)FPI_FIXED_MAX_VALUE;
    integer_part = ua / ub;
    if (integer_part > (limit >> 16)) return fixed_apply_sign(limit, negative);
    remainder = ua % ub;
    fraction = 0UL;
    for (i = 0; i < 16; i++) {
        fraction <<= 1;
        if (remainder >= ub - remainder) {
            fraction |= 1UL;
            remainder = remainder - (ub - remainder);
        } else {
            remainder += remainder;
        }
    }
    result = (integer_part << 16) | fraction;
    if (result > limit) result = limit;
    if (ok) *ok = 1;
    return fixed_apply_sign(result, negative);
}

FPI_Fixed fpi_fixed_mod(FPI_Fixed a, FPI_Fixed b, int* ok) {
    if (ok) *ok = 0;
    if (b == 0L) return 0L;
    if (a == FPI_FIXED_MIN_VALUE && b == -1L) {
        if (ok) *ok = 1;
        return 0L;
    }
    if (ok) *ok = 1;
    return a % b;
}

static const FPI_Fixed fpi_sin_table[91] = {
0L, 1144L, 2287L, 3430L, 4572L, 5712L, 6850L, 7987L, 9121L, 10252L, 11380L, 12505L, 13626L, 14742L, 15855L, 16962L, 18064L, 19161L, 20252L, 21336L, 22415L, 23486L, 24550L, 25607L, 26656L, 27697L, 28729L, 29753L, 30767L, 31772L, 32768L, 33754L, 34729L, 35693L, 36647L, 37590L, 38521L, 39441L, 40348L, 41243L, 42126L, 42995L, 43852L, 44695L, 45525L, 46341L, 47143L, 47930L, 48703L, 49461L, 50203L, 50931L, 51643L, 52339L, 53020L, 53684L, 54332L, 54963L, 55578L, 56175L, 56756L, 57319L, 57865L, 58393L, 58903L, 59396L, 59870L, 60326L, 60764L, 61183L, 61584L, 61966L, 62328L, 62672L, 62997L, 63303L, 63589L, 63856L, 64104L, 64332L, 64540L, 64729L, 64898L, 65048L, 65177L, 65287L, 65376L, 65446L, 65496L, 65526L, 65536L
};

static FPI_Fixed sin_lookup_0_90(FPI_Fixed angle) {
    long index;
    unsigned long fraction;
    FPI_Fixed base;
    FPI_Fixed next;
    FPI_Fixed delta;
    FPI_Fixed interp;
    if (angle <= 0L) return 0L;
    if (angle >= 90L * FPI_FIXED_ONE) return FPI_FIXED_ONE;
    index = angle >> 16;
    fraction = (unsigned long)angle & 65535UL;
    base = fpi_sin_table[index];
    next = fpi_sin_table[index + 1L];
    delta = next - base;
    interp = (FPI_Fixed)(((unsigned long)delta * fraction) >> 16);
    return base + interp;
}

FPI_Fixed fpi_fixed_sin_deg(FPI_Fixed degrees) {
    FPI_Fixed full;
    FPI_Fixed d;
    FPI_Fixed angle;
    full = 360L * FPI_FIXED_ONE;
    d = degrees % full;
    if (d < 0L) d += full;
    if (d <= 90L * FPI_FIXED_ONE) return sin_lookup_0_90(d);
    if (d <= 180L * FPI_FIXED_ONE) {
        angle = 180L * FPI_FIXED_ONE - d;
        return sin_lookup_0_90(angle);
    }
    if (d <= 270L * FPI_FIXED_ONE) {
        angle = d - 180L * FPI_FIXED_ONE;
        return -sin_lookup_0_90(angle);
    }
    angle = 360L * FPI_FIXED_ONE - d;
    return -sin_lookup_0_90(angle);
}

FPI_Fixed fpi_fixed_cos_deg(FPI_Fixed degrees) {
    FPI_Fixed full;
    FPI_Fixed d;
    full = 360L * FPI_FIXED_ONE;
    d = degrees % full;
    if (d < 0L) d += full;
    d += 90L * FPI_FIXED_ONE;
    if (d >= full) d -= full;
    return fpi_fixed_sin_deg(d);
}

void fpi_value_clear(FPI_Value* value) {
    if (!value) return;
    value->has = 0;
    value->kind = FPI_VALUE_NONE;
    value->fixed = 0L;
    value->is_int = 0;
    value->i = 0;
    value->s = "";
    value->len = 0;
}
