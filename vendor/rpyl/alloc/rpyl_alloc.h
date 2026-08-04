#ifndef RPYL_ALLOC_H
#define RPYL_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void rpyl_mem_clear(void* ptr, size_t bytes);
void rpyl_mem_copy(void* dst, const void* src, size_t bytes);
int rpyl_mem_is_zero(const void* ptr, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
