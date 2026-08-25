#ifndef GF_HB_COMPAT_H
#define GF_HB_COMPAT_H
#include "gf_shape.h"

typedef GF_ShapePlan gf_hb_font_t;
typedef struct gf_hb_buffer_t {
    unsigned int text[GF_MAX_RUNES];
    int text_count;
    GF_ShapeItem items[GF_MAX_SHAPE_ITEMS];
    int item_count;
} gf_hb_buffer_t;

void gf_hb_buffer_clear(gf_hb_buffer_t *b);
int gf_hb_buffer_add_utf32(gf_hb_buffer_t *b, const unsigned int *text, int count);
int gf_hb_shape(const GF_Font *font, const gf_hb_font_t *hbfont, gf_hb_buffer_t *buffer);

#endif
