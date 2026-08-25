#include "gf_layout.h"

int gf_text_layout(const GF_Font *font, const GF_ShapePlan *plan, const unsigned int *text, int text_count, GF_TextRun *run) {
    GF_ShapeItem items[GF_MAX_SHAPE_ITEMS];
    int n, i;
    long pen;
    if (!font || !plan || !text || !run) return -1;
    n = gf_shape_text(font, plan, text, text_count, items, GF_MAX_SHAPE_ITEMS);
    run->count = 0;
    run->width = 0;
    run->ascender = font->ascender;
    run->descender = font->descender;
    pen = 0;
    for (i = 0; i < n && i < GF_MAX_SHAPE_ITEMS; ++i) {
        run->glyphs[i].codepoint = items[i].codepoint;
        run->glyphs[i].glyph_index = items[i].glyph_index;
        run->glyphs[i].pen_x = pen;
        run->glyphs[i].pen_y = 0;
        run->glyphs[i].x_offset = items[i].x_offset;
        run->glyphs[i].y_offset = items[i].y_offset;
        run->glyphs[i].x_advance = items[i].x_advance;
        pen += items[i].x_offset + items[i].x_advance;
        run->count++;
    }
    run->width = pen;
    return (int)run->count;
}

int gf_text_render_1bit(const GF_Font *font, const GF_TextRun *run, GF_Bitmap *dst, int w, int h, long scale, long base_x, long base_y, GF_Arena *arena) {
    unsigned short i;
    long mark;
    GF_Bitmap tmp;
    if (!font || !run || !dst || !arena) return -1;
    gf_bitmap_clear(dst, (unsigned short)w, (unsigned short)h);
    for (i = 0; i < run->count; ++i) {
        const GF_TextGlyph *tg = &run->glyphs[i];
        const GF_Glyph *g;
        long ox, oy;
        int bx, by;
        if (tg->glyph_index >= font->glyph_count) continue;
        g = &font->glyphs[tg->glyph_index];
        mark = gf_arena_used(arena);
        ox = base_x + gf_mul(tg->pen_x + tg->x_offset, scale);
        oy = base_y + gf_mul(tg->pen_y + tg->y_offset, scale);
        if (gf_rasterize_1bit(g, &tmp, GF_MAX_BITMAP_W, GF_MAX_BITMAP_H, scale, ox, oy, arena) == 0) {
            bx = 0; by = 0;
            gf_bitmap_blit_or(dst, &tmp, bx, by);
        }
        gf_arena_pop(arena, mark);
    }
    return 0;
}

int gf_text_measure(const GF_TextRun *run, long *out_w) {
    if (!run || !out_w) return -1;
    *out_w = run->width;
    return 0;
}
