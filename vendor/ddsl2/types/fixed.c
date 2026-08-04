#include "types/fixed.h"
#include "common/strview.h"

#include <limits.h>
#include <string.h>

static unsigned long ddsl_abs_long_to_ul(long v) {
    if (v >= 0L) return (unsigned long)v;
    if (v == LONG_MIN) return ((unsigned long)LONG_MAX) + 1UL;
    return (unsigned long)(-v);
}

static long ddsl_saturate_pos(unsigned long v) {
    if (v > (unsigned long)LONG_MAX) return LONG_MAX;
    return (long)v;
}

static ddsl_fixed ddsl_apply_sign_sat(unsigned long mag, int neg) {
    if (!neg) return ddsl_saturate_pos(mag);
    if (mag > ((unsigned long)LONG_MAX) + 1UL) return LONG_MIN;
    if (mag == ((unsigned long)LONG_MAX) + 1UL) return LONG_MIN;
    return (ddsl_fixed)(-(long)mag);
}

static ddsl_fixed ddsl_sat_add(ddsl_fixed a, ddsl_fixed b) {
    if (b > 0L && a > LONG_MAX - b) return LONG_MAX;
    if (b < 0L && a < LONG_MIN - b) return LONG_MIN;
    return a + b;
}

static ddsl_fixed ddsl_sat_mul_small(ddsl_fixed a, long b) {
    int neg;
    unsigned long ua;
    unsigned long ub;
    unsigned long maxv;

    if (a == 0L || b == 0L) return 0L;
    neg = ((a < 0L) != (b < 0L)) ? 1 : 0;
    ua = ddsl_abs_long_to_ul(a);
    ub = ddsl_abs_long_to_ul(b);
    maxv = neg ? (((unsigned long)LONG_MAX) + 1UL) : (unsigned long)LONG_MAX;
    if (ub != 0UL && ua > maxv / ub) {
        return neg ? LONG_MIN : LONG_MAX;
    }
    return ddsl_apply_sign_sat(ua * ub, neg);
}

static void ddsl_put_char(char *dst, int cap, int *idx, char c) {
    if (!dst || !idx || cap <= 0) return;
    if (*idx < cap - 1) {
        dst[*idx] = c;
        *idx = *idx + 1;
        dst[*idx] = '\0';
    }
}

static void ddsl_put_ulong(char *dst, int cap, int *idx, unsigned long v) {
    char tmp[32];
    int n;

    n = 0;
    if (v == 0UL) {
        ddsl_put_char(dst, cap, idx, '0');
        return;
    }
    while (v > 0UL && n < (int)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (int)(v % 10UL));
        v /= 10UL;
    }
    while (n > 0) {
        ddsl_put_char(dst, cap, idx, tmp[--n]);
    }
}

ddsl_fixed ddsl_fixed_from_int(long v) {
    return ddsl_sat_mul_small((ddsl_fixed)v, (long)DDSL_FIXED_SCALE);
}

long ddsl_fixed_to_int(ddsl_fixed v) {
    return v / (ddsl_fixed)DDSL_FIXED_SCALE;
}

ddsl_fixed ddsl_fixed_neg(ddsl_fixed a) {
    if (a == LONG_MIN) return LONG_MAX;
    return -a;
}

ddsl_fixed ddsl_fixed_add(ddsl_fixed a, ddsl_fixed b) {
    return ddsl_sat_add(a, b);
}

ddsl_fixed ddsl_fixed_sub(ddsl_fixed a, ddsl_fixed b) {
    return ddsl_sat_add(a, ddsl_fixed_neg(b));
}

ddsl_fixed ddsl_fixed_mul(ddsl_fixed a, ddsl_fixed b) {
    int neg;
    unsigned long ua;
    unsigned long ub;
    unsigned long ai;
    unsigned long af;
    unsigned long bi;
    unsigned long bf;
    unsigned long scale;
    ddsl_fixed r;
    ddsl_fixed term;

    scale = (unsigned long)DDSL_FIXED_SCALE;
    if (a == 0L || b == 0L) return 0L;
    neg = ((a < 0L) != (b < 0L)) ? 1 : 0;
    ua = ddsl_abs_long_to_ul(a);
    ub = ddsl_abs_long_to_ul(b);

    ai = ua / scale;
    af = ua % scale;
    bi = ub / scale;
    bf = ub % scale;

    r = 0L;
    term = ddsl_sat_mul_small((ddsl_fixed)ai, (long)(bi * scale));
    r = ddsl_fixed_add(r, term);
    term = ddsl_sat_mul_small((ddsl_fixed)ai, (long)bf);
    r = ddsl_fixed_add(r, term);
    term = ddsl_sat_mul_small((ddsl_fixed)bi, (long)af);
    r = ddsl_fixed_add(r, term);
    term = (ddsl_fixed)((af * bf) / scale);
    r = ddsl_fixed_add(r, term);

    if (neg) r = ddsl_fixed_neg(r);
    return r;
}

ddsl_fixed ddsl_fixed_div(ddsl_fixed a, ddsl_fixed b) {
    int neg;
    unsigned long ua;
    unsigned long ub;
    unsigned long scale;
    unsigned long q;
    unsigned long rem;
    unsigned long mag;

    if (b == 0L) return 0L;
    if (a == 0L) return 0L;

    neg = ((a < 0L) != (b < 0L)) ? 1 : 0;
    ua = ddsl_abs_long_to_ul(a);
    ub = ddsl_abs_long_to_ul(b);
    scale = (unsigned long)DDSL_FIXED_SCALE;

    q = ua / ub;
    rem = ua % ub;

    if (q > (unsigned long)LONG_MAX / scale) {
        return neg ? LONG_MIN : LONG_MAX;
    }
    mag = q * scale;

    if (rem > 0UL) {
        unsigned long frac;
        if (rem > (unsigned long)LONG_MAX / scale) {
            frac = (rem / ub) * scale;
        } else {
            frac = (rem * scale) / ub;
        }
        if (mag > (unsigned long)LONG_MAX - frac) mag = (unsigned long)LONG_MAX;
        else mag += frac;
    }

    return ddsl_apply_sign_sat(mag, neg);
}

int ddsl_fixed_parse_cstr(const char *s, ddsl_fixed *out) {
    int neg;
    int saw_digit;
    unsigned long ip;
    unsigned long fp;
    unsigned long place;
    unsigned long scale;
    unsigned long max_mag;
    unsigned long mag;

    if (out) *out = 0L;
    if (!s) return 0;

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;

    neg = 0;
    if (*s == '+' || *s == '-') {
        if (*s == '-') neg = 1;
        s++;
    }

    ip = 0UL;
    fp = 0UL;
    place = (unsigned long)DDSL_FIXED_SCALE / 10UL;
    scale = (unsigned long)DDSL_FIXED_SCALE;
    saw_digit = 0;
    max_mag = neg ? (((unsigned long)LONG_MAX) + 1UL) : (unsigned long)LONG_MAX;

    while (*s >= '0' && *s <= '9') {
        unsigned long d;
        saw_digit = 1;
        d = (unsigned long)(*s - '0');
        if (ip > max_mag / 10UL) return 0;
        ip = ip * 10UL + d;
        s++;
    }

    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            saw_digit = 1;
            if (place > 0UL) {
                fp += ((unsigned long)(*s - '0')) * place;
                place /= 10UL;
            }
            s++;
        }
    }

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    if (*s != '\0' || !saw_digit) return 0;

    if (ip > max_mag / scale) return 0;
    mag = ip * scale;
    if (mag > max_mag - fp) return 0;
    mag += fp;

    if (out) *out = ddsl_apply_sign_sat(mag, neg);
    return 1;
}

int ddsl_fixed_parse_sv(ddsl_strview sv, ddsl_fixed *out) {
    char buf[64];
    int n;

    if (out) *out = 0L;
    n = sv.len;
    if (n < 0) n = 0;
    if (n > (int)sizeof(buf) - 1) return 0;
    if (n > 0 && sv.data) memcpy(buf, sv.data, (size_t)n);
    buf[n] = '\0';
    return ddsl_fixed_parse_cstr(buf, out);
}

int ddsl_fixed_to_cstr(ddsl_fixed v, char *dst, int dst_cap) {
    int idx;
    int neg;
    unsigned long mag;
    unsigned long scale;
    unsigned long ip;
    unsigned long fp;
    unsigned long place;
    int frac_start;
    int i;

    if (!dst || dst_cap <= 0) return 0;
    dst[0] = '\0';
    idx = 0;
    scale = (unsigned long)DDSL_FIXED_SCALE;
    neg = (v < 0L) ? 1 : 0;
    mag = ddsl_abs_long_to_ul((long)v);
    ip = mag / scale;
    fp = mag % scale;

    if (neg) ddsl_put_char(dst, dst_cap, &idx, '-');
    ddsl_put_ulong(dst, dst_cap, &idx, ip);

    if (fp != 0UL) {
        ddsl_put_char(dst, dst_cap, &idx, '.');
        frac_start = idx;
        place = scale / 10UL;
        while (place > 0UL) {
            ddsl_put_char(dst, dst_cap, &idx, (char)('0' + (int)(fp / place)));
            fp %= place;
            place /= 10UL;
        }
        i = idx - 1;
        while (i >= frac_start && dst[i] == '0') {
            dst[i] = '\0';
            idx--;
            i--;
        }
        if (idx > 0 && dst[idx - 1] == '.') {
            dst[idx - 1] = '\0';
            idx--;
        }
    }

    return idx;
}
