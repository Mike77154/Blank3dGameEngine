#include "value/value.h"

#include <string.h>
#include <ctype.h>

static int cstr_len(const char *s) {
    return s ? (int)strlen(s) : 0;
}

static int sv_eq_ci(ddsl_strview sv, const char *cstr) {
    int i;
    int n;
    if (!cstr) cstr = "";
    n = cstr_len(cstr);
    if (sv.len != n) return 0;
    for (i = 0; i < n; ++i) {
        char a;
        char b;
        a = (char)tolower((unsigned char)sv.data[i]);
        b = (char)tolower((unsigned char)cstr[i]);
        if (a != b) return 0;
    }
    return 1;
}

ddsl_value ddsl_v_null(void) {
    ddsl_value v;
    v.kind = DDSL_VAL_NULL;
    v.num = DDSL_FIXED_ZERO;
    v.boolean = 0;
    v.str.data = "";
    v.str.len = 0;
    return v;
}

ddsl_value ddsl_v_num(ddsl_fixed x) {
    ddsl_value v;
    v.kind = DDSL_VAL_NUM;
    v.num = x;
    v.boolean = 0;
    v.str.data = "";
    v.str.len = 0;
    return v;
}

ddsl_value ddsl_v_bool(int b) {
    ddsl_value v;
    v.kind = DDSL_VAL_BOOL;
    v.num = DDSL_FIXED_ZERO;
    v.boolean = b ? 1 : 0;
    v.str.data = "";
    v.str.len = 0;
    return v;
}

ddsl_value ddsl_v_str(ddsl_strview sv) {
    ddsl_value v;
    v.kind = DDSL_VAL_STR;
    v.num = DDSL_FIXED_ZERO;
    v.boolean = 0;
    v.str = sv;
    if (!v.str.data) {
        v.str.data = "";
        v.str.len = 0;
    }
    return v;
}

static int str_is_true(ddsl_strview sv) {
    return (sv_eq_ci(sv, "1") || sv_eq_ci(sv, "true") || sv_eq_ci(sv, "yes") || sv_eq_ci(sv, "on") ||
            sv_eq_ci(sv, "si") || sv_eq_ci(sv, "sí") || sv_eq_ci(sv, "enabled") || sv_eq_ci(sv, "ready") ||
            sv_eq_ci(sv, "done") || sv_eq_ci(sv, "ok") || sv_eq_ci(sv, "verdadero") || sv_eq_ci(sv, "cierto")) ? 1 : 0;
}

static int str_is_false(ddsl_strview sv) {
    return (sv_eq_ci(sv, "0") || sv_eq_ci(sv, "false") || sv_eq_ci(sv, "no") || sv_eq_ci(sv, "off") ||
            sv_eq_ci(sv, "disabled") || sv_eq_ci(sv, "notready") || sv_eq_ci(sv, "undone") ||
            sv_eq_ci(sv, "none") || sv_eq_ci(sv, "null") || sv_eq_ci(sv, "falso") || sv_eq_ci(sv, "falsa")) ? 1 : 0;
}

int ddsl_value_truthy(ddsl_value v) {
    if (v.kind == DDSL_VAL_BOOL) return v.boolean ? 1 : 0;
    if (v.kind == DDSL_VAL_NUM) return (v.num != DDSL_FIXED_ZERO) ? 1 : 0;
    if (v.kind == DDSL_VAL_STR) {
        if (str_is_true(v.str)) return 1;
        if (str_is_false(v.str)) return 0;
        return (v.str.len > 0) ? 1 : 0;
    }
    return 0;
}

ddsl_fixed ddsl_value_to_num(ddsl_value v, int *ok) {
    ddsl_fixed d;

    if (ok) *ok = 0;
    if (v.kind == DDSL_VAL_NUM) {
        if (ok) *ok = 1;
        return v.num;
    }
    if (v.kind == DDSL_VAL_BOOL) {
        if (ok) *ok = 1;
        return v.boolean ? DDSL_FIXED_ONE : DDSL_FIXED_ZERO;
    }
    if (v.kind == DDSL_VAL_STR) {
        if (ddsl_fixed_parse_sv(v.str, &d)) {
            if (ok) *ok = 1;
            return d;
        }
    }
    return DDSL_FIXED_ZERO;
}

int ddsl_value_to_cstr(ddsl_value v, char *dst, int dst_cap) {
    int n;
    if (!dst || dst_cap <= 0) return 0;

    if (v.kind == DDSL_VAL_STR) {
        n = v.str.len;
        if (n < 0) n = 0;
        if (n > dst_cap - 1) n = dst_cap - 1;
        if (n > 0 && v.str.data) memcpy(dst, v.str.data, (size_t)n);
        dst[n] = '\0';
        return n;
    }

    if (v.kind == DDSL_VAL_NUM) {
        return ddsl_fixed_to_cstr(v.num, dst, dst_cap);
    }

    if (v.kind == DDSL_VAL_BOOL) {
        const char *s;
        s = v.boolean ? "true" : "false";
        n = cstr_len(s);
        if (n > dst_cap - 1) n = dst_cap - 1;
        memcpy(dst, s, (size_t)n);
        dst[n] = '\0';
        return n;
    }

    dst[0] = '\0';
    return 0;
}
