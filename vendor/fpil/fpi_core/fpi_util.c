#include "fpi_util.h"
#include <limits.h>

char fpi_ascii_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c + ('a' - 'A'));
    return c;
}

int fpi_ascii_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int fpi_ascii_is_digit(char c) {
    return c >= '0' && c <= '9';
}

int fpi_ascii_is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int fpi_ascii_is_ident_continue(char c) {
    return fpi_ascii_is_ident_start(c) || fpi_ascii_is_digit(c) || c == '-';
}

int fpi_str_ieq(const char* a, const char* b) {
    int i;
    if (!a || !b) return 0;
    i = 0;
    while (a[i] && b[i]) {
        if (fpi_ascii_tolower(a[i]) != fpi_ascii_tolower(b[i])) return 0;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

int fpi_str_copy(char* dst, int cap, const char* src) {
    int i;
    if (!dst || cap <= 0) return -1;
    if (!src) { dst[0] = '\0'; return 0; }
    i = 0;
    while (src[i]) {
        if (i >= cap - 1) { dst[cap - 1] = '\0'; return -1; }
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

int fpi_str_copy_lower(char* dst, int cap, const char* src) {
    int i;
    if (!dst || cap <= 0) return -1;
    if (!src) { dst[0] = '\0'; return 0; }
    i = 0;
    while (src[i]) {
        if (i >= cap - 1) { dst[cap - 1] = '\0'; return -1; }
        dst[i] = fpi_ascii_tolower(src[i]);
        i++;
    }
    dst[i] = '\0';
    return i;
}

FPI_U32 fpi_hash_lower(const char* text) {
    FPI_U32 h;
    int i;
    h = 2166136261UL;
    if (!text) return 0UL;
    i = 0;
    while (text[i]) {
        h ^= (FPI_U32)(unsigned char)fpi_ascii_tolower(text[i]);
        h *= 16777619UL;
        i++;
    }
    return h;
}

long fpi_long_abs_sat(long value) {
    if (value == LONG_MIN) return LONG_MAX;
    return value < 0 ? -value : value;
}
