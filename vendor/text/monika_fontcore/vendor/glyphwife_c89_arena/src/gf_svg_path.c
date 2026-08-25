#include "gf_svg_path.h"
#include <ctype.h>
#include <stdlib.h>

static void skip_ws(const char **p) {
    while (**p && (isspace((unsigned char)**p) || **p == ',')) (*p)++;
}
static int read_int(const char **p, long *out) {
    char *endp;
    long v;
    skip_ws(p);
    if (!**p) return -1;
    v = strtol(*p, &endp, 10);
    if (endp == *p) return -1;
    *out = v;
    *p = endp;
    return 0;
}

int gf_svg_path_to_glyph(GF_Glyph *g, const char *path, unsigned int codepoint, long advance_x, int canvas_h) {
    const char *p;
    char cmd;
    long x, y, cx, cy, x0, y0;
    int have_contour;
    if (!g || !path) return -1;
    gf_glyph_clear(g);
    g->codepoint = codepoint;
    g->advance_x = advance_x;
    p = path;
    cmd = 0;
    x0 = 0; y0 = 0; have_contour = 0;
    while (*p) {
        skip_ws(&p);
        if (!*p) break;
        if (isalpha((unsigned char)*p)) { cmd = *p; p++; }
        if (cmd == 'M' || cmd == 'm') {
            if (read_int(&p, &x) != 0 || read_int(&p, &y) != 0) return -2;
            if (cmd == 'm') { x += x0; y += y0; }
            if (have_contour) gf_glyph_close_contour(g);
            if (gf_glyph_begin_contour(g, GF_FX((int)x), GF_FX(canvas_h - (int)y)) != 0) return -3;
            x0 = x; y0 = y; have_contour = 1;
            cmd = (cmd == 'm') ? 'l' : 'L';
        } else if (cmd == 'L' || cmd == 'l') {
            if (!have_contour) return -4;
            if (read_int(&p, &x) != 0 || read_int(&p, &y) != 0) return -5;
            if (cmd == 'l') { x += x0; y += y0; }
            if (gf_glyph_line_to(g, GF_FX((int)x), GF_FX(canvas_h - (int)y)) != 0) return -6;
            x0 = x; y0 = y;
        } else if (cmd == 'Q' || cmd == 'q') {
            if (!have_contour) return -7;
            if (read_int(&p, &cx) != 0 || read_int(&p, &cy) != 0 || read_int(&p, &x) != 0 || read_int(&p, &y) != 0) return -8;
            if (cmd == 'q') { cx += x0; cy += y0; x += x0; y += y0; }
            if (gf_glyph_quad_to(g, GF_FX((int)cx), GF_FX(canvas_h - (int)cy), GF_FX((int)x), GF_FX(canvas_h - (int)y)) != 0) return -9;
            x0 = x; y0 = y;
        } else if (cmd == 'Z' || cmd == 'z') {
            if (have_contour) gf_glyph_close_contour(g);
            have_contour = 0;
            cmd = 0;
        } else return -10;
    }
    if (have_contour) gf_glyph_close_contour(g);
    gf_glyph_bbox(g);
    return 0;
}
