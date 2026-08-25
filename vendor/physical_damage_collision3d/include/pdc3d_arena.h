#ifndef PDC3D_ARENA_H
#define PDC3D_ARENA_H

#include "pdc3d_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pdc3d_arena_s {
    unsigned char *mem;
    int capacity;
    int offset;
    int high_water;
    int overflowed;
} pdc3d_arena;

void pdc3d_arena_init(pdc3d_arena *a, void *mem, int capacity);
void pdc3d_arena_reset(pdc3d_arena *a);
void *pdc3d_arena_alloc_bytes(pdc3d_arena *a, int byte_count);
int pdc3d_arena_used(const pdc3d_arena *a);
int pdc3d_arena_remaining(const pdc3d_arena *a);
int pdc3d_arena_overflowed(const pdc3d_arena *a);

#ifdef __cplusplus
}
#endif

#endif
