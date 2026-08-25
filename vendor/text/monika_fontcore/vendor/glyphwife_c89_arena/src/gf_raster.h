#ifndef GF_RASTER_H
#define GF_RASTER_H
#include "gf_outline.h"
#include "gf_bitmap.h"
#include "gf_arena.h"

int gf_rasterize_1bit(const GF_Glyph *g, GF_Bitmap *bmp, int px_w, int px_h, long scale_26_6, long ox, long oy, GF_Arena *arena);

#endif
