#include "bvh_common.h"

void bvh_error_set(BVH_Error *err, int line, int col, const char *msg) {
    unsigned long n;
    if (!err) return;
    err->line = line;
    err->col = col;
    if (!msg) msg = "error";
    n = bvh_cstr_len(msg);
    if (n >= (unsigned long)BVH_ERRMSG_MAX) n = (unsigned long)BVH_ERRMSG_MAX - 1UL;
    if (n) memcpy(err->message, msg, (size_t)n);
    err->message[n] = '\0';
}

int bvh_slice_eq_lit(const char *s, unsigned long n, const char *lit) {
    unsigned long m;
    m = bvh_cstr_len(lit);
    if (n != m) return 0;
    if (n == 0UL) return 1;
    return memcmp(s, lit, (size_t)n) == 0 ? 1 : 0;
}

int bvh_lit_eq(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    return strcmp(a, b) == 0 ? 1 : 0;
}

unsigned long bvh_cstr_len(const char *s) {
    if (!s) return 0UL;
    return (unsigned long)strlen(s);
}

void bvh_copy_slice(char *dst, unsigned long cap, const char *s, unsigned long n) {
    unsigned long k;
    if (!dst || cap == 0UL) return;
    if (!s) s = "";
    k = n;
    if (k + 1UL > cap) k = cap - 1UL;
    if (k) memcpy(dst, s, (size_t)k);
    dst[k] = '\0';
}

int bvh_is_space_no_eol(int c) {
    return (c == ' ' || c == '\t' || c == '\r') ? 1 : 0;
}

int bvh_is_digit_ch(int c) {
    return (c >= '0' && c <= '9') ? 1 : 0;
}

int bvh_is_hex_ch(int c) {
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'a' && c <= 'f') return 1;
    if (c >= 'A' && c <= 'F') return 1;
    return 0;
}

int bvh_hex_value(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}
