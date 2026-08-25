#include "gf_transform.h"

int gf_glyph_copy(GF_Glyph *dst, const GF_Glyph *src) {
    unsigned short i;
    if (!dst || !src) return -1;
    gf_glyph_clear(dst);
    dst->point_count = src->point_count;
    dst->contour_count = src->contour_count;
    dst->open_contour = src->open_contour;
    dst->advance_x = src->advance_x;
    dst->bearing_x = src->bearing_x;
    dst->bearing_y = src->bearing_y;
    dst->xmin = src->xmin; dst->ymin = src->ymin; dst->xmax = src->xmax; dst->ymax = src->ymax;
    dst->codepoint = src->codepoint;
    for (i = 0; i < src->point_count; ++i) dst->points[i] = src->points[i];
    for (i = 0; i < src->contour_count; ++i) dst->contours[i] = src->contours[i];
    return 0;
}

void gf_glyph_translate(GF_Glyph *g, long dx, long dy) {
    unsigned short i;
    if (!g) return;
    for (i = 0; i < g->point_count; ++i) { g->points[i].x += dx; g->points[i].y += dy; }
    g->bearing_x += dx; g->bearing_y += dy;
    gf_glyph_bbox(g);
}

void gf_glyph_scale_xy(GF_Glyph *g, long sx, long sy) {
    unsigned short i;
    if (!g) return;
    for (i = 0; i < g->point_count; ++i) { g->points[i].x = gf_mul(g->points[i].x, sx); g->points[i].y = gf_mul(g->points[i].y, sy); }
    g->advance_x = gf_mul(g->advance_x, sx);
    g->bearing_x = gf_mul(g->bearing_x, sx);
    g->bearing_y = gf_mul(g->bearing_y, sy);
    gf_glyph_bbox(g);
}

void gf_glyph_embolden(GF_Glyph *g, long amount_x, long amount_y) {
    unsigned short i;
    long cx, cy;
    if (!g || g->point_count == 0) return;
    gf_glyph_bbox(g);
    cx = (g->xmin + g->xmax) / 2;
    cy = (g->ymin + g->ymax) / 2;
    for (i = 0; i < g->point_count; ++i) {
        if (g->points[i].x >= cx) g->points[i].x += amount_x; else g->points[i].x -= amount_x;
        if (g->points[i].y >= cy) g->points[i].y += amount_y; else g->points[i].y -= amount_y;
    }
    g->advance_x += amount_x * 2;
    gf_glyph_bbox(g);
}
