#include "gf_hb_compat.h"
void gf_hb_buffer_clear(gf_hb_buffer_t *b) { b->text_count = 0; b->item_count = 0; }
int gf_hb_buffer_add_utf32(gf_hb_buffer_t *b, const unsigned int *text, int count) { int i; if (count > GF_MAX_RUNES) count = GF_MAX_RUNES; for (i=0;i<count;i++) b->text[i]=text[i]; b->text_count=count; return count; }
int gf_hb_shape(const GF_Font *font, const gf_hb_font_t *hbfont, gf_hb_buffer_t *buffer) { buffer->item_count = gf_shape_text(font, hbfont, buffer->text, buffer->text_count, buffer->items, GF_MAX_SHAPE_ITEMS); return buffer->item_count; }
