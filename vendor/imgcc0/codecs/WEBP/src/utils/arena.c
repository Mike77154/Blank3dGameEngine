#include "arena.h"

void GWPArenaInit(GWPArena* arena, void* memory, GWPu32 size) {
  if (arena == 0) return;
  arena->base = (GWPu8*)memory;
  arena->size = size;
  arena->used = 0u;
}

void* GWPArenaAlloc(GWPArena* arena, GWPu32 size, GWPu32 alignment) {
  GWPu32 pos;
  GWPu32 mask;
  if (arena == 0 || arena->base == 0) return 0;
  if (alignment == 0u) alignment = 1u;
  pos = arena->used;
  mask = alignment - 1u;
  pos = (pos + mask) & ~mask;
  if (pos > arena->size) return 0;
  if (size > arena->size - pos) return 0;
  arena->used = pos + size;
  return arena->base + pos;
}

GWPu32 GWPArenaUsed(const GWPArena* arena) {
  if (arena == 0) return 0u;
  return arena->used;
}
