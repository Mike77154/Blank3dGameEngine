#include "gf_freetype_compat.h"
void gf_ft_face_init(GF_FT_Face *face, GF_Font *font) { face->font = font; }
int gf_ft_load_char(GF_FT_Face *face, unsigned int codepoint, GF_FT_GlyphSlot *slot) { GF_Glyph *g = gf_font_find(face->font, codepoint); if (!g) return -1; slot->glyph = g; slot->advance_x = g->advance_x; return 0; }
int gf_ft_render_glyph(GF_FT_GlyphSlot *slot, int w, int h, long scale, long ox, long oy, GF_Arena *arena) { if (!slot->glyph) return -1; return gf_rasterize_1bit(slot->glyph, &slot->bitmap, w, h, scale, ox, oy, arena); }
