#include "rpyl_alloc.h"

void rpyl_mem_clear(void* ptr, size_t bytes) {
    unsigned char* p;
    size_t i;
    if (!ptr) return;
    p = (unsigned char*)ptr;
    for (i = 0; i < bytes; i++) p[i] = 0;
}

void rpyl_mem_copy(void* dst, const void* src, size_t bytes) {
    unsigned char* d;
    const unsigned char* s;
    size_t i;
    if (!dst || !src) return;
    d = (unsigned char*)dst;
    s = (const unsigned char*)src;
    for (i = 0; i < bytes; i++) d[i] = s[i];
}

int rpyl_mem_is_zero(const void* ptr, size_t bytes) {
    const unsigned char* p;
    size_t i;
    if (!ptr) return 1;
    p = (const unsigned char*)ptr;
    for (i = 0; i < bytes; i++) {
        if (p[i] != 0) return 0;
    }
    return 1;
}
