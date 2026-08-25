#ifndef GF_OUTLINE_H
#define GF_OUTLINE_H
#include "gf_fixed.h"
#include "gf_config.h"

#define GF_TAG_ON     1
#define GF_TAG_QUAD   2
#define GF_TAG_CUBIC  3

typedef struct GF_Point { long x; long y; unsigned char tag; } GF_Point;
typedef struct GF_Contour { unsigned short start; unsigned short count; } GF_Contour;

typedef struct GF_Glyph {
    GF_Point points[GF_MAX_POINTS];
    GF_Contour contours[GF_MAX_CONTOURS];
    unsigned short point_count;
    unsigned short contour_count;
    unsigned short open_contour;
    long advance_x;
    long bearing_x;
    long bearing_y;
    long xmin, ymin, xmax, ymax;
    unsigned int codepoint;
} GF_Glyph;

typedef struct GF_Font {
    GF_Glyph glyphs[GF_MAX_GLYPHS];
    unsigned short glyph_count;
    long units_per_em;
    long ascender;
    long descender;
} GF_Font;

void gf_glyph_clear(GF_Glyph *g);
void gf_font_clear(GF_Font *f);
int gf_font_add_glyph(GF_Font *f, unsigned int codepoint, long advance_x);
GF_Glyph *gf_font_find(GF_Font *f, unsigned int codepoint);
int gf_font_find_index(const GF_Font *f, unsigned int codepoint);
const GF_Glyph *gf_font_find_const(const GF_Font *f, unsigned int codepoint);
int gf_glyph_begin_contour(GF_Glyph *g, long x, long y);
int gf_glyph_line_to(GF_Glyph *g, long x, long y);
int gf_glyph_quad_to(GF_Glyph *g, long cx, long cy, long x, long y);
int gf_glyph_close_contour(GF_Glyph *g);
void gf_glyph_bbox(GF_Glyph *g);

#endif
