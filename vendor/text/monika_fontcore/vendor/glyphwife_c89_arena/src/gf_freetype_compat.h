#ifndef GF_FREETYPE_COMPAT_H
#define GF_FREETYPE_COMPAT_H
#include "gf_outline.h"
#include "gf_bitmap.h"
#include "gf_raster.h"
#include "gf_arena.h"

typedef struct GF_FT_Face { GF_Font *font; } GF_FT_Face;
typedef struct GF_FT_GlyphSlot { GF_Glyph *glyph; GF_Bitmap bitmap; long advance_x; } GF_FT_GlyphSlot;

void gf_ft_face_init(GF_FT_Face *face, GF_Font *font);
int gf_ft_load_char(GF_FT_Face *face, unsigned int codepoint, GF_FT_GlyphSlot *slot);
int gf_ft_render_glyph(GF_FT_GlyphSlot *slot, int w, int h, long scale, long ox, long oy, GF_Arena *arena);
#endif
