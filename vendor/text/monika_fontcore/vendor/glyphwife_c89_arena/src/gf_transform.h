#ifndef GF_TRANSFORM_H
#define GF_TRANSFORM_H
#include "gf_outline.h"

int gf_glyph_copy(GF_Glyph *dst, const GF_Glyph *src);
void gf_glyph_translate(GF_Glyph *g, long dx, long dy);
void gf_glyph_scale_xy(GF_Glyph *g, long sx, long sy);
void gf_glyph_embolden(GF_Glyph *g, long amount_x, long amount_y);

#endif
