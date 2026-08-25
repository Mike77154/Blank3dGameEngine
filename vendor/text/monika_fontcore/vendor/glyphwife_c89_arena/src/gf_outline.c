#include "gf_outline.h"

void gf_glyph_clear(GF_Glyph *g) {
    int i;
    g->point_count = 0; g->contour_count = 0; g->open_contour = 0;
    g->advance_x = GF_FX(8); g->bearing_x = 0; g->bearing_y = 0;
    g->xmin = g->ymin = g->xmax = g->ymax = 0; g->codepoint = 0;
    for (i = 0; i < GF_MAX_POINTS; ++i) { g->points[i].x = 0; g->points[i].y = 0; g->points[i].tag = 0; }
}

void gf_font_clear(GF_Font *f) {
    int i;
    f->glyph_count = 0; f->units_per_em = GF_FX(16); f->ascender = GF_FX(14); f->descender = -GF_FX(3);
    for (i = 0; i < GF_MAX_GLYPHS; ++i) gf_glyph_clear(&f->glyphs[i]);
}

int gf_font_add_glyph(GF_Font *f, unsigned int codepoint, long advance_x) {
    GF_Glyph *g;
    if (f->glyph_count >= GF_MAX_GLYPHS) return -1;
    g = &f->glyphs[f->glyph_count];
    gf_glyph_clear(g); g->codepoint = codepoint; g->advance_x = advance_x;
    f->glyph_count++;
    return (int)(f->glyph_count - 1);
}


int gf_font_find_index(const GF_Font *f, unsigned int codepoint) {
    unsigned short i;
    for (i = 0; i < f->glyph_count; ++i) if (f->glyphs[i].codepoint == codepoint) return (int)i;
    return -1;
}

GF_Glyph *gf_font_find(GF_Font *f, unsigned int codepoint) {
    unsigned short i;
    for (i = 0; i < f->glyph_count; ++i) if (f->glyphs[i].codepoint == codepoint) return &f->glyphs[i];
    return (GF_Glyph*)0;
}
const GF_Glyph *gf_font_find_const(const GF_Font *f, unsigned int codepoint) {
    unsigned short i;
    for (i = 0; i < f->glyph_count; ++i) if (f->glyphs[i].codepoint == codepoint) return &f->glyphs[i];
    return (const GF_Glyph*)0;
}

static int gf_add_point(GF_Glyph *g, long x, long y, unsigned char tag) {
    GF_Point *p;
    if (g->point_count >= GF_MAX_POINTS) return -1;
    p = &g->points[g->point_count++]; p->x = x; p->y = y; p->tag = tag;
    return 0;
}

int gf_glyph_begin_contour(GF_Glyph *g, long x, long y) {
    GF_Contour *c;
    if (g->open_contour) gf_glyph_close_contour(g);
    if (g->contour_count >= GF_MAX_CONTOURS) return -1;
    c = &g->contours[g->contour_count++];
    c->start = g->point_count; c->count = 0; g->open_contour = 1;
    if (gf_add_point(g, x, y, GF_TAG_ON) != 0) return -1;
    c->count++;
    return 0;
}
int gf_glyph_line_to(GF_Glyph *g, long x, long y) {
    if (!g->open_contour) return -1;
    if (gf_add_point(g, x, y, GF_TAG_ON) != 0) return -1;
    g->contours[g->contour_count - 1].count++;
    return 0;
}
int gf_glyph_quad_to(GF_Glyph *g, long cx, long cy, long x, long y) {
    if (!g->open_contour) return -1;
    if (g->point_count + 2 >= GF_MAX_POINTS) return -1;
    gf_add_point(g, cx, cy, GF_TAG_QUAD); gf_add_point(g, x, y, GF_TAG_ON);
    g->contours[g->contour_count - 1].count += 2;
    return 0;
}
int gf_glyph_close_contour(GF_Glyph *g) { g->open_contour = 0; gf_glyph_bbox(g); return 0; }

void gf_glyph_bbox(GF_Glyph *g) {
    unsigned short i;
    if (g->point_count == 0) { g->xmin = g->ymin = g->xmax = g->ymax = 0; return; }
    g->xmin = g->xmax = g->points[0].x; g->ymin = g->ymax = g->points[0].y;
    for (i = 1; i < g->point_count; ++i) {
        if (g->points[i].x < g->xmin) g->xmin = g->points[i].x;
        if (g->points[i].x > g->xmax) g->xmax = g->points[i].x;
        if (g->points[i].y < g->ymin) g->ymin = g->points[i].y;
        if (g->points[i].y > g->ymax) g->ymax = g->points[i].y;
    }
}
