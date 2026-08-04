#include "util/util.h"

#include <string.h>
#include <ctype.h>

int ddsl_util_cstr_len(const char *s) {
    return s ? (int)strlen(s) : 0;
}

int ddsl_util_copy_cstr(char *dst, int dst_cap, const char *src) {
    int n;
    if (!dst || dst_cap <= 0) return 0;
    if (!src) src = "";
    n = (int)strlen(src);
    if (n > dst_cap - 1) n = dst_cap - 1;
    if (n > 0) memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
    return n;
}

int ddsl_util_sv_eq_ci(ddsl_strview sv, const char *cstr) {
    int i;
    int n;
    if (!cstr) cstr = "";
    n = (int)strlen(cstr);
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
