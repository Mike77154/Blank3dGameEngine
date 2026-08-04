/* sff_map.h - fixed-capacity u32->u32 hash map (no heap) */
#ifndef SFF_MAP_H
#define SFF_MAP_H

#include "sff_types.h"

typedef struct SffMapSlot {
    sff_u32 key;
    sff_u32 val;
    sff_u8  used;
} SffMapSlot;

typedef struct SffMap {
    SffMapSlot *slots;
    sff_u32 cap;
    sff_u32 len;
} SffMap;

int  sff_map_init_fixed(SffMap *m, SffMapSlot *slots, sff_u32 cap);
void sff_map_clear(SffMap *m);
int  sff_map_set(SffMap *m, sff_u32 key, sff_u32 val);
int  sff_map_get(const SffMap *m, sff_u32 key, sff_u32 *out_val);
int  sff_map_has(const SffMap *m, sff_u32 key);

#endif /* SFF_MAP_H */
