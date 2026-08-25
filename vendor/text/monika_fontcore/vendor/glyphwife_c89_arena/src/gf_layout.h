#ifndef GF_LAYOUT_H
#define GF_LAYOUT_H
#include "gf_shape.h"
#include "gf_bitmap.h"
#include "gf_raster.h"
#include "gf_arena.h"

#define GF_TEXT_ALIGN_LEFT   0
#define GF_TEXT_ALIGN_CENTER 1
#define GF_TEXT_ALIGN_RIGHT  2

typedef struct GF_TextGlyph {
    unsigned int codepoint;
    unsigned short glyph_index;
    long pen_x;
    long pen_y;
    long x_offset;
    long y_offset;
    long x_advance;
} GF_TextGlyph;

typedef struct GF_TextRun {
    GF_TextGlyph glyphs[GF_MAX_SHAPE_ITEMS];
    unsigned short count;
    long width;
    long ascender;
    long descender;
} GF_TextRun;

int gf_text_layout(const GF_Font *font, const GF_ShapePlan *plan, const unsigned int *text, int text_count, GF_TextRun *run);
int gf_text_render_1bit(const GF_Font *font, const GF_TextRun *run, GF_Bitmap *dst, int w, int h, long scale, long base_x, long base_y, GF_Arena *arena);
int gf_text_measure(const GF_TextRun *run, long *out_w);

#endif
