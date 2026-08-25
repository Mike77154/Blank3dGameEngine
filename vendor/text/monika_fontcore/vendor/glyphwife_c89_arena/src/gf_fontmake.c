#include "gf_fontmake.h"

static GF_Glyph *add(GF_Font *font, unsigned int cp, int adv) { int idx = gf_font_add_glyph(font, cp, GF_FX(adv)); return &font->glyphs[idx]; }

void gf_build_demo_font(GF_Font *font) {
    GF_Glyph *g; gf_font_clear(font);
    g = add(font, 'A', 18);
    gf_glyph_begin_contour(g, GF_FX(1), GF_FX(0)); gf_glyph_line_to(g, GF_FX(8), GF_FX(18)); gf_glyph_line_to(g, GF_FX(15), GF_FX(0)); gf_glyph_line_to(g, GF_FX(12), GF_FX(0)); gf_glyph_line_to(g, GF_FX(10), GF_FX(5)); gf_glyph_line_to(g, GF_FX(6), GF_FX(5)); gf_glyph_line_to(g, GF_FX(4), GF_FX(0)); gf_glyph_close_contour(g);
    g = add(font, 'V', 18);
    gf_glyph_begin_contour(g, GF_FX(1), GF_FX(18)); gf_glyph_line_to(g, GF_FX(7), GF_FX(0)); gf_glyph_line_to(g, GF_FX(14), GF_FX(18)); gf_glyph_line_to(g, GF_FX(11), GF_FX(18)); gf_glyph_line_to(g, GF_FX(7), GF_FX(6)); gf_glyph_line_to(g, GF_FX(4), GF_FX(18)); gf_glyph_close_contour(g);
    g = add(font, 'O', 18);
    gf_glyph_begin_contour(g, GF_FX(8), GF_FX(0)); gf_glyph_quad_to(g, GF_FX(0), GF_FX(0), GF_FX(0), GF_FX(9)); gf_glyph_quad_to(g, GF_FX(0), GF_FX(18), GF_FX(8), GF_FX(18)); gf_glyph_quad_to(g, GF_FX(16), GF_FX(18), GF_FX(16), GF_FX(9)); gf_glyph_quad_to(g, GF_FX(16), GF_FX(0), GF_FX(8), GF_FX(0)); gf_glyph_close_contour(g);
    gf_glyph_begin_contour(g, GF_FX(8), GF_FX(4)); gf_glyph_quad_to(g, GF_FX(4), GF_FX(4), GF_FX(4), GF_FX(9)); gf_glyph_quad_to(g, GF_FX(4), GF_FX(14), GF_FX(8), GF_FX(14)); gf_glyph_quad_to(g, GF_FX(12), GF_FX(14), GF_FX(12), GF_FX(9)); gf_glyph_quad_to(g, GF_FX(12), GF_FX(4), GF_FX(8), GF_FX(4)); gf_glyph_close_contour(g);
    g = add(font, 'f', 10); gf_glyph_begin_contour(g, GF_FX(4), GF_FX(0)); gf_glyph_line_to(g, GF_FX(4), GF_FX(12)); gf_glyph_quad_to(g, GF_FX(4), GF_FX(18), GF_FX(9), GF_FX(18)); gf_glyph_line_to(g, GF_FX(9), GF_FX(15)); gf_glyph_quad_to(g, GF_FX(7), GF_FX(15), GF_FX(7), GF_FX(12)); gf_glyph_line_to(g, GF_FX(10), GF_FX(12)); gf_glyph_line_to(g, GF_FX(10), GF_FX(9)); gf_glyph_line_to(g, GF_FX(7), GF_FX(9)); gf_glyph_line_to(g, GF_FX(7), GF_FX(0)); gf_glyph_close_contour(g);
    g = add(font, 'i', 7); gf_glyph_begin_contour(g, GF_FX(2), GF_FX(0)); gf_glyph_line_to(g, GF_FX(5), GF_FX(0)); gf_glyph_line_to(g, GF_FX(5), GF_FX(12)); gf_glyph_line_to(g, GF_FX(2), GF_FX(12)); gf_glyph_close_contour(g); gf_glyph_begin_contour(g, GF_FX(2), GF_FX(15)); gf_glyph_line_to(g, GF_FX(5), GF_FX(15)); gf_glyph_line_to(g, GF_FX(5), GF_FX(18)); gf_glyph_line_to(g, GF_FX(2), GF_FX(18)); gf_glyph_close_contour(g);
    g = add(font, 0xFB01u, 15); gf_glyph_begin_contour(g, GF_FX(3), GF_FX(0)); gf_glyph_line_to(g, GF_FX(6), GF_FX(0)); gf_glyph_line_to(g, GF_FX(6), GF_FX(9)); gf_glyph_line_to(g, GF_FX(12), GF_FX(9)); gf_glyph_line_to(g, GF_FX(12), GF_FX(0)); gf_glyph_line_to(g, GF_FX(15), GF_FX(0)); gf_glyph_line_to(g, GF_FX(15), GF_FX(12)); gf_glyph_line_to(g, GF_FX(6), GF_FX(12)); gf_glyph_quad_to(g, GF_FX(6), GF_FX(16), GF_FX(10), GF_FX(16)); gf_glyph_line_to(g, GF_FX(10), GF_FX(18)); gf_glyph_quad_to(g, GF_FX(3), GF_FX(18), GF_FX(3), GF_FX(12)); gf_glyph_close_contour(g);
}
