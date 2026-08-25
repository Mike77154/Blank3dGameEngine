#ifndef GWP_UTILS_ARENA_H_
#define GWP_UTILS_ARENA_H_

#include "../webp/types.h"

typedef struct GWPArena {
  GWPu8* base;
  GWPu32 size;
  GWPu32 used;
} GWPArena;

void GWPArenaInit(GWPArena* arena, void* memory, GWPu32 size);
void* GWPArenaAlloc(GWPArena* arena, GWPu32 size, GWPu32 alignment);
GWPu32 GWPArenaUsed(const GWPArena* arena);

#endif  /* GWP_UTILS_ARENA_H_ */
