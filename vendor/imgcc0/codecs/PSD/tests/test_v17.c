#include "psd89/psd89.h"

#include <stdio.h>
#include <string.h>

static void init_rgb_layer_rect(psd89_layer *layer,
                                psd89_s32 top,
                                psd89_s32 left,
                                psd89_s32 bottom,
                                psd89_s32 right,
                                const char *name,
                                const char blend_mode[4],
                                psd89_u8 opacity,
                                psd89_u8 clipping,
                                const unsigned char *r,
                                const unsigned char *g,
                                const unsigned char *b,
                                const unsigned char *a,
                                psd89_u16 compression,
                                psd89_u32 stride)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = top;
    layer->left = left;
    layer->bottom = bottom;
    layer->right = right;
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = opacity;
    layer->clipping = clipping;
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = compression;
    layer->channels[0].plane = r;
    layer->channels[0].stride = stride;
    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = compression;
    layer->channels[1].plane = g;
    layer->channels[1].stride = stride;
    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = compression;
    layer->channels[2].plane = b;
    layer->channels[2].stride = stride;
    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = compression;
    layer->channels[3].plane = a;
    layer->channels[3].stride = stride;
}

static void add_mask_channel(psd89_layer *layer,
                             psd89_s16 channel_id,
                             const unsigned char *plane,
                             psd89_u16 compression,
                             psd89_u32 stride)
{
    unsigned int idx;
    idx = layer->channel_count;
    layer->channels[idx].id = channel_id;
    layer->channels[idx].compression = compression;
    layer->channels[idx].plane = plane;
    layer->channels[idx].stride = stride;
    layer->channel_count = (psd89_u16)(idx + 1U);
}

static psd89_fx24 fx24_frac(psd89_s32 num, psd89_s32 den)
{
    if (den == 0) {
        return 0;
    }
    return (psd89_fx24)(((psd89_s32)num << 24) / den);
}

static void set_vector_point_knot(psd89_vector_knot *knot, psd89_fx24 x, psd89_fx24 y)
{
    memset(knot, 0, sizeof(*knot));
    knot->linked = 1U;
    knot->preceding_h = x;
    knot->anchor_h = x;
    knot->leaving_h = x;
    knot->preceding_v = y;
    knot->anchor_v = y;
    knot->leaving_v = y;
}

static void set_vector_curve_knot(psd89_vector_knot *knot,
                                  psd89_fx24 prev_x,
                                  psd89_fx24 prev_y,
                                  psd89_fx24 anchor_x,
                                  psd89_fx24 anchor_y,
                                  psd89_fx24 next_x,
                                  psd89_fx24 next_y)
{
    memset(knot, 0, sizeof(*knot));
    knot->linked = 0U;
    knot->preceding_h = prev_x;
    knot->preceding_v = prev_y;
    knot->anchor_h = anchor_x;
    knot->anchor_v = anchor_y;
    knot->leaving_h = next_x;
    knot->leaving_v = next_y;
}

static void set_vector_rect(psd89_layer *layer,
                            psd89_u32 doc_width,
                            psd89_u32 doc_height,
                            psd89_s32 left,
                            psd89_s32 top,
                            psd89_s32 right,
                            psd89_s32 bottom)
{
    psd89_vector_mask *vm;

    vm = &layer->vector_mask;
    memset(vm, 0, sizeof(*vm));
    vm->present = 1U;
    vm->version = 3U;
    vm->path_fill_rule_present = 1U;
    vm->initial_fill_rule_present = 1U;
    vm->initial_fill_rule = 0U;
    vm->subpath_count = 1U;
    vm->knot_count = 4U;
    vm->subpaths[0].closed = 1U;
    vm->subpaths[0].first_knot = 0U;
    vm->subpaths[0].knot_count = 4U;

    set_vector_point_knot(&vm->knots[0], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_point_knot(&vm->knots[1], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_point_knot(&vm->knots[2], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
    set_vector_point_knot(&vm->knots[3], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
}

static void set_vector_ellipse(psd89_layer *layer, psd89_u32 doc_width, psd89_u32 doc_height)
{
    psd89_vector_mask *vm;

    vm = &layer->vector_mask;
    memset(vm, 0, sizeof(*vm));
    vm->present = 1U;
    vm->version = 3U;
    vm->path_fill_rule_present = 1U;
    vm->initial_fill_rule_present = 1U;
    vm->initial_fill_rule = 0U;
    vm->subpath_count = 1U;
    vm->knot_count = 2U;
    vm->subpaths[0].closed = 1U;
    vm->subpaths[0].first_knot = 0U;
    vm->subpaths[0].knot_count = 2U;

    set_vector_curve_knot(&vm->knots[0],
                          fx24_frac(2, (psd89_s32)doc_width), fx24_frac(7, (psd89_s32)doc_height),
                          fx24_frac(2, (psd89_s32)doc_width), fx24_frac(4, (psd89_s32)doc_height),
                          fx24_frac(2, (psd89_s32)doc_width), fx24_frac(1, (psd89_s32)doc_height));
    set_vector_curve_knot(&vm->knots[1],
                          fx24_frac(6, (psd89_s32)doc_width), fx24_frac(1, (psd89_s32)doc_height),
                          fx24_frac(6, (psd89_s32)doc_width), fx24_frac(4, (psd89_s32)doc_height),
                          fx24_frac(6, (psd89_s32)doc_width), fx24_frac(7, (psd89_s32)doc_height));
}

static int compose_doc(const psd89_doc *doc,
                       unsigned char *out_r,
                       unsigned char *out_g,
                       unsigned char *out_b,
                       unsigned char *out_a,
                       psd89_u32 stride)
{
    unsigned char scratch_r[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_g[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_b[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_a[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char *dst_color[3];
    unsigned char *scratch_color[3];
    psd89_compose_options opt;
    int rc;

    dst_color[0] = out_r;
    dst_color[1] = out_g;
    dst_color[2] = out_b;
    scratch_color[0] = scratch_r;
    scratch_color[1] = scratch_g;
    scratch_color[2] = scratch_b;

    psd89_compose_options_init(&opt);
    rc = psd89_compose_layers_u8(doc,
                                 0,
                                 dst_color,
                                 out_a,
                                 stride,
                                 scratch_color,
                                 scratch_a,
                                 PSD89_MAX_COMPOSE_ROW_BYTES,
                                 &opt);
    if (rc != PSD89_OK) {
        fprintf(stderr, "compose returned %s\n", psd89_error_string(rc));
        return 0;
    }
    return 1;
}

static int expect_pixel_rgba(const unsigned char *r,
                             const unsigned char *g,
                             const unsigned char *b,
                             const unsigned char *a,
                             psd89_u32 idx,
                             unsigned char er,
                             unsigned char eg,
                             unsigned char eb,
                             unsigned char ea,
                             const char *label)
{
    if (r[idx] != er || g[idx] != eg || b[idx] != eb || a[idx] != ea) {
        fprintf(stderr,
                "%s mismatch at %u: got=(%u,%u,%u,%u) want=(%u,%u,%u,%u)\n",
                label,
                idx,
                (unsigned int)r[idx],
                (unsigned int)g[idx],
                (unsigned int)b[idx],
                (unsigned int)a[idx],
                (unsigned int)er,
                (unsigned int)eg,
                (unsigned int)eb,
                (unsigned int)ea);
        return 0;
    }
    return 1;
}

static int test_lmgm_clipping(void)
{
    unsigned char gray_r[2] = { 64U, 64U };
    unsigned char gray_g[2] = { 64U, 64U };
    unsigned char gray_b[2] = { 64U, 64U };
    unsigned char gray_a[2] = { 255U, 255U };
    unsigned char white[2] = { 255U, 255U };
    unsigned char zero[2] = { 0U, 0U };
    unsigned char alpha[2] = { 255U, 255U };
    unsigned char red[2] = { 255U, 255U };
    unsigned char mask[2] = { 255U, 0U };
    unsigned char out_r[2];
    unsigned char out_g[2];
    unsigned char out_b[2];
    unsigned char out_a[2];
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.layer_count = 3U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 2, "back", norm, 255U, 0U,
                        gray_r, gray_g, gray_b, gray_a, PSD89_COMP_RAW, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 1, 2, "base", norm, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RAW, 2U);
    add_mask_channel(&doc.layers[1], PSD89_CH_LAYER_MASK, mask, PSD89_COMP_RAW, 2U);
    doc.layers[1].user_mask.present = 1U;
    doc.layers[1].user_mask.top = 0;
    doc.layers[1].user_mask.left = 0;
    doc.layers[1].user_mask.bottom = 1;
    doc.layers[1].user_mask.right = 2;
    doc.layers[1].user_mask.default_color = 255U;
    doc.layers[1].user_mask.flags = 0x01U;
    doc.layers[1].layer_mask_global_present = 1U;
    doc.layers[1].layer_mask_global = 0U;

    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 2, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RAW, 2U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "lmgm false left") ||
        !expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 64U, 64U, 64U, 255U, "lmgm false right")) {
        return 0;
    }

    doc.layers[1].layer_mask_global = 1U;
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "lmgm true left") ||
        !expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 255U, 0U, 0U, 255U, "lmgm true right")) {
        return 0;
    }
    return 1;
}

static int test_vmgm_clipping(void)
{
    unsigned char gray_r[2] = { 64U, 64U };
    unsigned char gray_g[2] = { 64U, 64U };
    unsigned char gray_b[2] = { 64U, 64U };
    unsigned char gray_a[2] = { 255U, 255U };
    unsigned char white[2] = { 255U, 255U };
    unsigned char zero[2] = { 0U, 0U };
    unsigned char alpha[2] = { 255U, 255U };
    unsigned char red[2] = { 255U, 255U };
    unsigned char out_r[2];
    unsigned char out_g[2];
    unsigned char out_b[2];
    unsigned char out_a[2];
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.layer_count = 3U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 2, "back", norm, 255U, 0U,
                        gray_r, gray_g, gray_b, gray_a, PSD89_COMP_RAW, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 1, 2, "base", norm, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RAW, 2U);
    set_vector_rect(&doc.layers[1], 2U, 1U, 0, 0, 1, 1);
    doc.layers[1].vector_mask_global_present = 1U;
    doc.layers[1].vector_mask_global = 0U;

    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 2, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RAW, 2U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "vmgm false left") ||
        !expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 64U, 64U, 64U, 255U, "vmgm false right")) {
        return 0;
    }

    doc.layers[1].vector_mask_global = 1U;
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "vmgm true left") ||
        !expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 255U, 0U, 0U, 255U, "vmgm true right")) {
        return 0;
    }
    return 1;
}

static int test_bezier_vector_compose(void)
{
    unsigned char white_r[64];
    unsigned char white_g[64];
    unsigned char white_b[64];
    unsigned char white_a[64];
    unsigned char out_r[64];
    unsigned char out_g[64];
    unsigned char out_b[64];
    unsigned char out_a[64];
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    psd89_u32 i;
    psd89_u32 center;

    for (i = 0U; i < 64U; ++i) {
        white_r[i] = 255U;
        white_g[i] = 255U;
        white_b[i] = 255U;
        white_a[i] = 255U;
    }

    psd89_doc_init(&doc);
    doc.width = 8U;
    doc.height = 8U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.layer_count = 1U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 8, 8, "ellipse", norm, 255U, 0U,
                        white_r, white_g, white_b, white_a, PSD89_COMP_RAW, 8U);
    set_vector_ellipse(&doc.layers[0], 8U, 8U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 8U)) {
        return 0;
    }

    center = 3U + 3U * 8U;
    if (out_a[center] < 200U || out_r[center] < 200U) {
        fprintf(stderr, "bezier compose center too low: a=%u r=%u\n",
                (unsigned int)out_a[center],
                (unsigned int)out_r[center]);
        return 0;
    }
    if (out_a[0] != 0U || out_r[0] != 0U || out_g[0] != 0U || out_b[0] != 0U) {
        fprintf(stderr, "bezier compose corner not transparent\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_lmgm_clipping()) {
        return 1;
    }
    if (!test_vmgm_clipping()) {
        return 1;
    }
    if (!test_bezier_vector_compose()) {
        return 1;
    }
    printf("test_v17: ok\n");
    return 0;
}
