#include "gf_shape.h"

void gf_shape_plan_clear(GF_ShapePlan *p) {
    if (!p) return;
    p->ligature_count = 0;
    p->kern_count = 0;
}

int gf_shape_add_ligature(GF_ShapePlan *p, unsigned int a, unsigned int b, unsigned int out) {
    if (!p) return -1;
    if (p->ligature_count >= 64) return -1;
    p->ligatures[p->ligature_count].a = a;
    p->ligatures[p->ligature_count].b = b;
    p->ligatures[p->ligature_count].out = out;
    p->ligature_count++;
    return 0;
}

int gf_shape_add_kern(GF_ShapePlan *p, unsigned int left, unsigned int right, long kern_x) {
    if (!p) return -1;
    if (p->kern_count >= 128) return -1;
    p->kerns[p->kern_count].left = left;
    p->kerns[p->kern_count].right = right;
    p->kerns[p->kern_count].kern_x = kern_x;
    p->kern_count++;
    return 0;
}

static long gf_kern(const GF_ShapePlan *p, unsigned int l, unsigned int r) {
    unsigned short i;
    if (!p) return 0;
    for (i = 0; i < p->kern_count; i++) {
        if (p->kerns[i].left == l && p->kerns[i].right == r) return p->kerns[i].kern_x;
    }
    return 0;
}

static int gf_lig(const GF_ShapePlan *p, unsigned int a, unsigned int b, unsigned int *out) {
    unsigned short i;
    if (!p) return 0;
    for (i = 0; i < p->ligature_count; i++) {
        if (p->ligatures[i].a == a && p->ligatures[i].b == b) {
            *out = p->ligatures[i].out;
            return 1;
        }
    }
    return 0;
}

int gf_shape_text(const GF_Font *font, const GF_ShapePlan *plan, const unsigned int *text, int text_count, GF_ShapeItem *out, int max_out) {
    int i, n, idx;
    unsigned int cp, prev;
    if (!font || !text || !out || max_out <= 0) return -1;
    n = 0;
    i = 0;
    prev = 0;
    while (i < text_count && n < max_out) {
        cp = text[i];
        if (i + 1 < text_count) {
            unsigned int lig;
            if (gf_lig(plan, text[i], text[i + 1], &lig)) {
                cp = lig;
                i++;
            }
        }
        idx = gf_font_find_index(font, cp);
        out[n].codepoint = cp;
        out[n].glyph_index = (unsigned short)(idx < 0 ? 0 : idx);
        out[n].x_offset = (n > 0) ? gf_kern(plan, prev, cp) : 0;
        out[n].y_offset = 0;
        out[n].x_advance = (idx >= 0) ? font->glyphs[idx].advance_x : GF_FX(8);
        prev = cp;
        n++;
        i++;
    }
    return n;
}
