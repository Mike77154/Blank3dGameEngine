/* flags_fx.c - fixed-point helpers (Q16.16) - C89 */
#include "flags_fx.h"

static long _pow10_i(int n)
{
    long p;
    int i;
    p = 1;
    for (i = 0; i < n; ++i) {
        p *= 10L;
    }
    return p;
}

flags_fx_t flags_fx_from_scaled(long value, int scale)
{
    /* value / 10^scale */
    long denom;
    flags_fx_t fx;
    denom = _pow10_i(scale);
    if (denom <= 0) denom = 1;

    /* Multiply first to preserve precision, then divide.
     * (value * 2^16) / denom
     * Beware overflow; for typical small numbers it's fine.
     */
    fx = (flags_fx_t)((value * FLAGS_FX_ONE) / denom);
    return fx;
}

int flags_fx_parse(const char *s, int len, flags_fx_t *out_fx)
{
    int i;
    int sign;
    long ipart;
    long fpart;
    int fscale;
    int saw_digit;
    int saw_dot;

    if (!s || len <= 0 || !out_fx) return 0;

    i = 0;
    sign = 1;
    ipart = 0;
    fpart = 0;
    fscale = 0;
    saw_digit = 0;
    saw_dot = 0;

    /* sign */
    if (i < len && (s[i] == '+' || s[i] == '-')) {
        if (s[i] == '-') sign = -1;
        i++;
    }

    /* integer digits */
    while (i < len) {
        char c;
        c = s[i];
        if (c >= '0' && c <= '9') {
            saw_digit = 1;
            ipart = (ipart * 10L) + (long)(c - '0');
            i++;
            continue;
        }
        break;
    }

    /* optional fractional */
    if (i < len && s[i] == '.') {
        saw_dot = 1;
        i++;
        while (i < len) {
            char c;
            c = s[i];
            if (c >= '0' && c <= '9') {
                /* cap fractional precision to avoid overflow */
                if (fscale < 6) {
                    fpart = (fpart * 10L) + (long)(c - '0');
                    fscale++;
                }
                saw_digit = 1;
                i++;
                continue;
            }
            break;
        }
    }

    /* must have at least one digit */
    if (!saw_digit) return 0;

    /* allow trailing spaces (caller may pass trimmed already) */
    while (i < len) {
        char c;
        c = s[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            i++;
            continue;
        }
        /* anything else is invalid */
        return 0;
    }

    /* build fixed-point */
    {
        flags_fx_t fx;
        fx = FLAGS_FX_FROM_INT(ipart);
        if (saw_dot && fscale > 0) {
            fx += flags_fx_from_scaled(fpart, fscale);
        }
        if (sign < 0) fx = -fx;
        *out_fx = fx;
    }

    return 1;
}

int flags_fx_lt(flags_fx_t a, flags_fx_t b) { return (a < b) ? 1 : 0; }
int flags_fx_gt(flags_fx_t a, flags_fx_t b) { return (a > b) ? 1 : 0; }

flags_fx_t flags_fx_clamp(flags_fx_t x, int has_lo, flags_fx_t lo, int has_hi, flags_fx_t hi)
{
    if (has_lo && x < lo) x = lo;
    if (has_hi && x > hi) x = hi;
    return x;
}
