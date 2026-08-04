#include "common/strview.h"

#include <string.h>

static int ddsl_cstr_len(const char *s) {
    if (!s) return 0;
    return (int)strlen(s);
}

ddsl_strview ddsl_sv_from_cstr(const char *s) {
    ddsl_strview sv;
    sv.data = s ? s : "";
    sv.len = ddsl_cstr_len(s);
    return sv;
}

int ddsl_sv_eq_cstr(ddsl_strview a, const char *b) {
    int blen;
    if (!b) b = "";
    blen = ddsl_cstr_len(b);
    if (a.len != blen) return 0;
    if (a.len == 0) return 1;
    if (!a.data) return 0;
    return (memcmp(a.data, b, (size_t)a.len) == 0) ? 1 : 0;
}

int ddsl_sv_to_cstr(ddsl_strview sv, char *dst, int dst_cap) {
    int n;
    if (!dst || dst_cap <= 0) return 0;
    n = sv.len;
    if (n < 0) n = 0;
    if (n > dst_cap - 1) n = dst_cap - 1;
    if (n > 0 && sv.data) memcpy(dst, sv.data, (size_t)n);
    dst[n] = '\0';
    return n;
}
