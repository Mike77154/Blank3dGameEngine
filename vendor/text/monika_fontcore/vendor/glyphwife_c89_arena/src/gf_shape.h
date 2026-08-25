#ifndef GF_SHAPE_H
#define GF_SHAPE_H
#include "gf_outline.h"

typedef struct GF_ShapeItem {
    unsigned int codepoint;
    unsigned short glyph_index;
    long x_advance;
    long x_offset;
    long y_offset;
} GF_ShapeItem;

typedef struct GF_Ligature { unsigned int a, b, out; } GF_Ligature;
typedef struct GF_KernPair { unsigned int left, right; long kern_x; } GF_KernPair;

typedef struct GF_ShapePlan {
    GF_Ligature ligatures[64];
    unsigned short ligature_count;
    GF_KernPair kerns[128];
    unsigned short kern_count;
} GF_ShapePlan;

void gf_shape_plan_clear(GF_ShapePlan *p);
int gf_shape_add_ligature(GF_ShapePlan *p, unsigned int a, unsigned int b, unsigned int out);
int gf_shape_add_kern(GF_ShapePlan *p, unsigned int left, unsigned int right, long kern_x);
int gf_shape_text(const GF_Font *font, const GF_ShapePlan *plan, const unsigned int *text, int text_count, GF_ShapeItem *out, int max_out);

#endif
