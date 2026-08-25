#include "gf_raster.h"

typedef struct GF_FlatPt { long x; long y; } GF_FlatPt;

typedef struct GF_FlatContour { unsigned short start; unsigned short count; } GF_FlatContour;

static int gf_flat_add(GF_FlatPt *pts, int *n, long x, long y) {
    if (*n >= GF_MAX_FLAT_POINTS) return -1;
    pts[*n].x = x; pts[*n].y = y; (*n)++; return 0;
}

static int gf_flatten_quad(GF_FlatPt *pts, int *n, long x0, long y0, long cx, long cy, long x1, long y1) {
    int i; long t, mt, x, y;
    for (i = 1; i <= 8; ++i) {
        t = (long)((i * GF_FP_ONE) / 8); mt = GF_FP_ONE - t;
        x = gf_mul(gf_mul(mt, mt), x0) + gf_mul(gf_mul((2 * mt), t), cx) + gf_mul(gf_mul(t, t), x1);
        y = gf_mul(gf_mul(mt, mt), y0) + gf_mul(gf_mul((2 * mt), t), cy) + gf_mul(gf_mul(t, t), y1);
        if (gf_flat_add(pts, n, x, y) != 0) return -1;
    }
    return 0;
}

static int gf_flatten(const GF_Glyph *g, GF_FlatPt *pts, GF_FlatContour *contours, int *out_np, int *out_nc, long scale, long ox, long oy) {
    int ci, j, idx, start, end, pn, cn; GF_Point p0, p1, p2; long lx, ly;
    pn = 0; cn = 0;
    for (ci = 0; ci < (int)g->contour_count; ++ci) {
        start = g->contours[ci].start; end = start + g->contours[ci].count;
        if (start >= end) continue;
        contours[cn].start = (unsigned short)pn; contours[cn].count = 0;
        p0 = g->points[start]; lx = gf_mul(p0.x, scale) + ox; ly = gf_mul(p0.y, scale) + oy;
        gf_flat_add(pts, &pn, lx, ly); contours[cn].count++;
        j = start + 1;
        while (j < end) {
            p1 = g->points[j];
            if (p1.tag == GF_TAG_ON) {
                lx = gf_mul(p1.x, scale) + ox; ly = gf_mul(p1.y, scale) + oy;
                gf_flat_add(pts, &pn, lx, ly); contours[cn].count++; j++;
            } else if (p1.tag == GF_TAG_QUAD && j + 1 < end) {
                p2 = g->points[j + 1];
                gf_flatten_quad(pts, &pn, lx, ly, gf_mul(p1.x, scale) + ox, gf_mul(p1.y, scale) + oy, gf_mul(p2.x, scale) + ox, gf_mul(p2.y, scale) + oy);
                lx = gf_mul(p2.x, scale) + ox; ly = gf_mul(p2.y, scale) + oy; contours[cn].count += 8; j += 2;
            } else j++;
        }
        idx = contours[cn].start;
        if (contours[cn].count > 1 && (pts[pn - 1].x != pts[idx].x || pts[pn - 1].y != pts[idx].y)) { gf_flat_add(pts, &pn, pts[idx].x, pts[idx].y); contours[cn].count++; }
        cn++;
    }
    *out_np = pn; *out_nc = cn; return 0;
}

static int gf_point_inside(const GF_FlatPt *pts, const GF_FlatContour *contours, int nc, long x, long y) {
    int ci, i, j, inside; long xi, yi, xj, yj; inside = 0;
    for (ci = 0; ci < nc; ++ci) {
        int s = contours[ci].start; int c = contours[ci].count;
        for (i = 0, j = c - 1; i < c; j = i++) {
            xi = pts[s + i].x; yi = pts[s + i].y; xj = pts[s + j].x; yj = pts[s + j].y;
            if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / ((yj - yi) == 0 ? 1 : (yj - yi)) + xi)) inside = !inside;
        }
    }
    return inside;
}

int gf_rasterize_1bit(const GF_Glyph *g, GF_Bitmap *bmp, int px_w, int px_h, long scale_26_6, long ox, long oy, GF_Arena *arena) {
    GF_FlatPt *pts; GF_FlatContour *cons; int np, nc, x, y; long sx, sy;
    pts = (GF_FlatPt*)gf_arena_push(arena, sizeof(GF_FlatPt) * GF_MAX_FLAT_POINTS, 4);
    cons = (GF_FlatContour*)gf_arena_push(arena, sizeof(GF_FlatContour) * GF_MAX_CONTOURS, 4);
    if (!pts || !cons) return -1;
    gf_bitmap_clear(bmp, (unsigned short)px_w, (unsigned short)px_h);
    if (gf_flatten(g, pts, cons, &np, &nc, scale_26_6, ox, oy) != 0) return -1;
    for (y = 0; y < px_h; ++y) for (x = 0; x < px_w; ++x) {
        sx = ((long)x << GF_FP_SHIFT) + GF_FP_HALF; sy = ((long)y << GF_FP_SHIFT) + GF_FP_HALF;
        if (gf_point_inside(pts, cons, nc, sx, sy)) gf_bitmap_set(bmp, x, y, 1);
    }
    return 0;
}
