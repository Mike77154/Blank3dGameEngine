#ifndef ZRAGF_DEFLATE_PLAN_H_INCLUDED
#define ZRAGF_DEFLATE_PLAN_H_INCLUDED

#include "../zragflib_internal.h"

#define ZRAGF_DEFLATE_PLAN_MAX_SPLITS 8

typedef struct {
    zragf_size_t offsets[ZRAGF_DEFLATE_PLAN_MAX_SPLITS];
    int count;
} zragf_deflate_split_plan;

void zragf_deflate_plan_reset(zragf_deflate_split_plan *plan);
int zragf_deflate_plan_build(const zragf_u8 *src,
                             zragf_size_t src_size,
                             zragf_deflate_split_plan *plan);

#endif
