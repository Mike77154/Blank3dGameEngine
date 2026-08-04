#include "rpyl_common.h"

const char* rpyl_common_build_profile(void) {
    return "rpyl-strict-c89-noheap";
}

const char* rpyl_common_version(void) {
    return "0.4.0-crown-modular";
}

size_t rpyl_common_strlen(const char* s) {
    size_t n;
    n = 0u;
    if (!s) return 0u;
    while (s[n] != '\0') n++;
    return n;
}

int rpyl_common_streq(const char* a, const char* b) {
    size_t i;
    if (a == b) return 1;
    if (!a || !b) return 0;
    i = 0u;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i] ? 1 : 0;
}

static int lower_ascii(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

int rpyl_common_streq_nocase_ascii(const char* a, const char* b) {
    size_t i;
    if (a == b) return 1;
    if (!a || !b) return 0;
    i = 0u;
    while (a[i] != '\0' && b[i] != '\0') {
        if (lower_ascii((int)a[i]) != lower_ascii((int)b[i])) return 0;
        i++;
    }
    return a[i] == b[i] ? 1 : 0;
}

int rpyl_common_copy(char* dst, size_t dst_size, const char* src) {
    size_t i;
    int truncated;
    if (!dst || dst_size == 0u) return 0;
    if (!src) src = "";
    i = 0u;
    truncated = 0;
    while (src[i] != '\0' && i + 1u < dst_size) {
        dst[i] = src[i];
        i++;
    }
    if (src[i] != '\0') truncated = 1;
    dst[i] = '\0';
    return truncated ? 0 : 1;
}

int rpyl_common_append(char* dst, size_t dst_size, const char* src) {
    size_t used;
    size_t i;
    int complete;
    if (!dst || dst_size == 0u) return 0;
    if (!src) src = "";
    used = rpyl_common_strlen(dst);
    if (used >= dst_size) {
        dst[dst_size - 1u] = '\0';
        return 0;
    }
    i = 0u;
    complete = 1;
    while (src[i] != '\0') {
        if (used + 1u >= dst_size) {
            complete = 0;
            break;
        }
        dst[used] = src[i];
        used++;
        i++;
    }
    dst[used] = '\0';
    return complete;
}

int rpyl_common_append_char(char* dst, size_t dst_size, char ch) {
    size_t used;
    if (!dst || dst_size == 0u) return 0;
    used = rpyl_common_strlen(dst);
    if (used + 1u >= dst_size) {
        if (used < dst_size) dst[used] = '\0';
        else dst[dst_size - 1u] = '\0';
        return 0;
    }
    dst[used] = ch;
    dst[used + 1u] = '\0';
    return 1;
}

unsigned long rpyl_common_hash(const char* s) {
    unsigned long h;
    size_t i;
    h = 2166136261UL;
    if (!s) return h;
    i = 0u;
    while (s[i] != '\0') {
        h ^= (unsigned long)(unsigned char)s[i];
        h *= 16777619UL;
        i++;
    }
    return h;
}

size_t rpyl_common_align_up(size_t value, size_t align, int* ok) {
    size_t rem;
    size_t add;
    if (ok) *ok = 1;
    if (align == 0u || align == 1u) return value;
    rem = value % align;
    if (rem == 0u) return value;
    add = align - rem;
    if (value + add < value) {
        if (ok) *ok = 0;
        return value;
    }
    return value + add;
}

int rpyl_common_is_space(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') ? 1 : 0;
}

int rpyl_common_is_digit(int c) {
    return (c >= '0' && c <= '9') ? 1 : 0;
}

int rpyl_common_is_ident_start(int c) {
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c == '_' || c == '.') return 1;
    return 0;
}

int rpyl_common_is_ident_char(int c) {
    if (rpyl_common_is_ident_start(c)) return 1;
    if (rpyl_common_is_digit(c)) return 1;
    return 0;
}

int rpyl_common_is_identifier(const char* s) {
    size_t i;
    if (!s || !s[0]) return 0;
    if (!rpyl_common_is_ident_start((int)s[0])) return 0;
    i = 1u;
    while (s[i] != '\0') {
        if (!rpyl_common_is_ident_char((int)s[i])) return 0;
        i++;
    }
    return 1;
}

int rpyl_common_parse_ulong(const char* s, unsigned long* out_value) {
    unsigned long v;
    size_t i;
    if (!s || !s[0]) return 0;
    v = 0UL;
    i = 0u;
    while (s[i] != '\0') {
        unsigned long nv;
        if (!rpyl_common_is_digit((int)s[i])) return 0;
        nv = (v * 10UL) + (unsigned long)(s[i] - '0');
        if (nv < v) return 0;
        v = nv;
        i++;
    }
    if (out_value) *out_value = v;
    return 1;
}

int rpyl_common_parse_long(const char* s, long* out_value) {
    unsigned long uv;
    int sign;
    const char* p;
    if (!s || !s[0]) return 0;
    sign = 1;
    p = s;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    if (!rpyl_common_parse_ulong(p, &uv)) return 0;
    if (out_value) {
        if (sign < 0) *out_value = -(long)uv;
        else *out_value = (long)uv;
    }
    return 1;
}
