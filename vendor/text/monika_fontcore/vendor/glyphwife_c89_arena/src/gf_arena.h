#ifndef GF_ARENA_H
#define GF_ARENA_H
#include "gf_config.h"

typedef struct GF_Arena {
    unsigned char data[GF_ARENA_SIZE];
    long used;
} GF_Arena;

void gf_arena_init(GF_Arena *a);
void *gf_arena_push(GF_Arena *a, long size, long align);
void gf_arena_reset(GF_Arena *a);
void gf_arena_pop(GF_Arena *a, long mark);
void *gf_arena_push_zero(GF_Arena *a, long size, long align);
long gf_arena_used(const GF_Arena *a);
long gf_arena_remaining(const GF_Arena *a);

#endif
