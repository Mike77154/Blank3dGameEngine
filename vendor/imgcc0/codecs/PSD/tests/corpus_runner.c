#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

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

static void init_group_marker(psd89_layer *layer,
                              const char *name,
                              psd89_u8 divider_type,
                              const char section_blend_mode[4],
                              psd89_u8 opacity)
{
    memset(layer, 0, sizeof(*layer));
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';
    layer->opacity = opacity;
    layer->section_divider_present = 1U;
    layer->section_divider_type = divider_type;
    if (section_blend_mode != 0) {
        memcpy(layer->section_divider_blend_mode, section_blend_mode, 4U);
    }
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
    return den == 0 ? 0 : (psd89_fx24)(((psd89_s32)num << 24) / den);
}

static void set_vector_knot(psd89_vector_knot *knot, psd89_fx24 x, psd89_fx24 y)
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

static void set_vector_rect(psd89_layer *layer,
                            psd89_u32 doc_width,
                            psd89_u32 doc_height,
                            psd89_s32 left,
                            psd89_s32 top,
                            psd89_s32 right,
                            psd89_s32 bottom,
                            psd89_u8 use_vsms)
{
    psd89_vector_mask *vm;
    vm = &layer->vector_mask;
    memset(vm, 0, sizeof(*vm));
    vm->present = 1U;
    vm->use_vsms = use_vsms;
    vm->version = 3U;
    vm->path_fill_rule_present = 1U;
    vm->initial_fill_rule_present = 1U;
    vm->initial_fill_rule = 0U;
    vm->subpath_count = 1U;
    vm->knot_count = 4U;
    vm->subpaths[0].closed = 1U;
    vm->subpaths[0].first_knot = 0U;
    vm->subpaths[0].knot_count = 4U;

    set_vector_knot(&vm->knots[0], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[1], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[2], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[3], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
}

static int compose_and_write(const char *path,
                             psd89_doc *doc,
                             psd89_u32 stride)
{
    unsigned char out_r[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_g[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_b[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_a[PSD89_MAX_COMPOSE_ROW_BYTES];
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
        fprintf(stderr, "compose failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }
    doc->composite_planes[0] = out_r;
    doc->composite_planes[1] = out_g;
    doc->composite_planes[2] = out_b;
    doc->composite_planes[3] = out_a;
    rc = psd89_write_path(path, doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }
    return 1;
}

static int write_basic_user_mask_fixture(const char *path, psd89_u16 comp)
{
    unsigned char blue_r[4] = { 0, 0, 0, 0 };
    unsigned char blue_g[4] = { 0, 0, 0, 0 };
    unsigned char blue_b[4] = { 255, 255, 255, 255 };
    unsigned char blue_a[4] = { 255, 255, 255, 255 };
    unsigned char red_r[4]  = { 255, 255, 255, 255 };
    unsigned char red_g[4]  = { 0, 0, 0, 0 };
    unsigned char red_b[4]  = { 0, 0, 0, 0 };
    unsigned char red_a[4]  = { 255, 255, 255, 255 };
    unsigned char red_m[4]  = { 255, 0, 128, 255 };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2;
    doc.height = 2;
    doc.channels = 4;
    doc.depth = 8;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 2U;
    doc.composite_write_compression = comp;
    doc.layer_count = 2U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 2, 2, "blue-base", norm, 255U, 0U,
                        blue_r, blue_g, blue_b, blue_a, comp, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 2, 2, "red-masked", norm, 255U, 0U,
                        red_r, red_g, red_b, red_a, comp, 2U);
    add_mask_channel(&doc.layers[1], PSD89_CH_LAYER_MASK, red_m, comp, 2U);
    doc.layers[1].user_mask.present = 1U;
    doc.layers[1].user_mask.top = 0;
    doc.layers[1].user_mask.left = 0;
    doc.layers[1].user_mask.bottom = 2;
    doc.layers[1].user_mask.right = 2;
    doc.layers[1].user_mask.default_color = 255U;
    doc.layers[1].user_mask.flags = 0U;

    return compose_and_write(path, &doc, 2U);
}

static int write_shifted_density_fixture(const char *path)
{
    unsigned char blue_r[16];
    unsigned char blue_g[16];
    unsigned char blue_b[16];
    unsigned char blue_a[16];
    unsigned char red_r[9];
    unsigned char red_g[9];
    unsigned char red_b[9];
    unsigned char red_a[9];
    unsigned char mask_2x2[4] = { 255U, 255U, 255U, 255U };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    psd89_u32 i;

    for (i = 0U; i < 16U; ++i) {
        blue_r[i] = 0U;
        blue_g[i] = 0U;
        blue_b[i] = 255U;
        blue_a[i] = 255U;
    }
    for (i = 0U; i < 9U; ++i) {
        red_r[i] = 255U;
        red_g[i] = 0U;
        red_b[i] = 0U;
        red_a[i] = 255U;
    }

    psd89_doc_init(&doc);
    doc.width = 4;
    doc.height = 4;
    doc.channels = 4;
    doc.depth = 8;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 4U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 2U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 4, 4, "blue-base", norm, 255U, 0U,
                        blue_r, blue_g, blue_b, blue_a, PSD89_COMP_RLE, 4U);
    init_rgb_layer_rect(&doc.layers[1], 1, 1, 4, 4, "red-shifted", norm, 255U, 0U,
                        red_r, red_g, red_b, red_a, PSD89_COMP_RLE, 3U);
    add_mask_channel(&doc.layers[1], PSD89_CH_LAYER_MASK, mask_2x2, PSD89_COMP_RLE, 2U);
    doc.layers[1].user_mask.present = 1U;
    doc.layers[1].user_mask.top = 0;
    doc.layers[1].user_mask.left = 0;
    doc.layers[1].user_mask.bottom = 2;
    doc.layers[1].user_mask.right = 2;
    doc.layers[1].user_mask.default_color = 0U;
    doc.layers[1].user_mask.flags = 0x01U;
    doc.layers[1].user_mask.user_density_present = 1U;
    doc.layers[1].user_mask.user_density = 128U;
    doc.layers[1].user_mask.user_feather_present = 1U;
    doc.layers[1].user_mask.user_feather = psd89_fx16_from_int(2);

    return compose_and_write(path, &doc, 4U);
}

static int write_real_mask_fixture(const char *path)
{
    unsigned char blue_r[4] = { 0, 0, 0, 0 };
    unsigned char blue_g[4] = { 0, 0, 0, 0 };
    unsigned char blue_b[4] = { 255, 255, 255, 255 };
    unsigned char blue_a[4] = { 255, 255, 255, 255 };
    unsigned char green_r[4] = { 0, 0, 0, 0 };
    unsigned char green_g[4] = { 255, 255, 255, 255 };
    unsigned char green_b[4] = { 0, 0, 0, 0 };
    unsigned char green_a[4] = { 255, 255, 255, 255 };
    unsigned char mask_rendered[4] = { 255, 255, 255, 255 };
    unsigned char mask_real[4] = { 0, 0, 0, 0 };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2;
    doc.height = 2;
    doc.channels = 4;
    doc.depth = 8;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 2U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 2U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 2, 2, "blue-base", norm, 255U, 0U,
                        blue_r, blue_g, blue_b, blue_a, PSD89_COMP_RLE, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 2, 2, "green-effective", norm, 255U, 0U,
                        green_r, green_g, green_b, green_a, PSD89_COMP_RLE, 2U);
    add_mask_channel(&doc.layers[1], PSD89_CH_LAYER_MASK, mask_rendered, PSD89_COMP_RLE, 2U);
    add_mask_channel(&doc.layers[1], PSD89_CH_REAL_LAYER_MASK, mask_real, PSD89_COMP_RLE, 2U);
    doc.layers[1].user_mask.present = 1U;
    doc.layers[1].user_mask.top = 0;
    doc.layers[1].user_mask.left = 0;
    doc.layers[1].user_mask.bottom = 2;
    doc.layers[1].user_mask.right = 2;
    doc.layers[1].user_mask.default_color = 0U;
    doc.layers[1].user_mask.flags = 0x08U;
    doc.layers[1].user_mask.real_present = 1U;
    doc.layers[1].user_mask.real_top = 0;
    doc.layers[1].user_mask.real_left = 0;
    doc.layers[1].user_mask.real_bottom = 2;
    doc.layers[1].user_mask.real_right = 2;
    doc.layers[1].user_mask.real_background = 0U;
    doc.layers[1].user_mask.real_flags = 0U;

    return compose_and_write(path, &doc, 2U);
}


static int write_vector_zip_fixture(const char *path, psd89_u16 comp, psd89_u8 use_vsms)
{
    unsigned char back_r[8] = { 5U, 15U, 25U, 35U, 45U, 55U, 65U, 75U };
    unsigned char back_g[8] = { 10U, 20U, 30U, 40U, 50U, 60U, 70U, 80U };
    unsigned char back_b[8] = { 180U, 181U, 182U, 183U, 184U, 185U, 186U, 187U };
    unsigned char back_a[8] = { 255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U };
    unsigned char fore_r[8] = { 200U, 150U, 100U, 50U, 210U, 160U, 110U, 60U };
    unsigned char fore_g[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    unsigned char fore_b[8] = { 90U, 80U, 70U, 60U, 50U, 40U, 30U, 20U };
    unsigned char fore_a[8] = { 255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 4U;
    doc.height = 2U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 4U;
    doc.composite_write_compression = comp;
    doc.layer_count = 2U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 2, 4, "back", norm, 255U, 0U,
                        back_r, back_g, back_b, back_a, comp, 4U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 2, 4, "fore-vector", norm, 255U, 0U,
                        fore_r, fore_g, fore_b, fore_a, comp, 4U);
    set_vector_rect(&doc.layers[1], 4U, 2U, 0, 0, 2, 2, use_vsms);

    return compose_and_write(path, &doc, 4U);
}

static int write_clbl_fixture(const char *path, psd89_u8 blend_clipped)
{
    unsigned char gray[1] = { 128U };
    unsigned char white[1] = { 255U };
    unsigned char zero[1] = { 0U };
    unsigned char alpha[1] = { 255U };
    unsigned char red[1] = { 255U };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    char mul[4] = { 'm', 'u', 'l', ' ' };

    psd89_doc_init(&doc);
    doc.width = 1;
    doc.height = 1;
    doc.channels = 4;
    doc.depth = 8;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 1U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 3U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 1, "gray", norm, 255U, 0U,
                        gray, gray, gray, alpha, PSD89_COMP_RLE, 1U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 1, 1, "base-white", mul, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RLE, 1U);
    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 1, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RLE, 1U);
    doc.layers[1].blend_clipped_present = 1U;
    doc.layers[1].blend_clipped = blend_clipped;

    return compose_and_write(path, &doc, 1U);
}

static int write_lmgm_clipping_fixture(const char *path)
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
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 2U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 3U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 2, "back", norm, 255U, 0U,
                        gray_r, gray_g, gray_b, gray_a, PSD89_COMP_RLE, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 1, 2, "base", norm, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RLE, 2U);
    add_mask_channel(&doc.layers[1], PSD89_CH_LAYER_MASK, mask, PSD89_COMP_RLE, 2U);
    doc.layers[1].user_mask.present = 1U;
    doc.layers[1].user_mask.top = 0;
    doc.layers[1].user_mask.left = 0;
    doc.layers[1].user_mask.bottom = 1;
    doc.layers[1].user_mask.right = 2;
    doc.layers[1].user_mask.default_color = 255U;
    doc.layers[1].user_mask.flags = 0x01U;
    doc.layers[1].layer_mask_global_present = 1U;
    doc.layers[1].layer_mask_global = 1U;
    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 2, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RLE, 2U);

    return compose_and_write(path, &doc, 2U);
}

static int write_vmgm_clipping_fixture(const char *path)
{
    unsigned char gray_r[2] = { 64U, 64U };
    unsigned char gray_g[2] = { 64U, 64U };
    unsigned char gray_b[2] = { 64U, 64U };
    unsigned char gray_a[2] = { 255U, 255U };
    unsigned char white[2] = { 255U, 255U };
    unsigned char zero[2] = { 0U, 0U };
    unsigned char alpha[2] = { 255U, 255U };
    unsigned char red[2] = { 255U, 255U };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 2U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 2U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 3U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 2, "back", norm, 255U, 0U,
                        gray_r, gray_g, gray_b, gray_a, PSD89_COMP_RLE, 2U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 1, 2, "base", norm, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RLE, 2U);
    set_vector_rect(&doc.layers[1], 2U, 1U, 0, 0, 1, 1, 0U);
    doc.layers[1].vector_mask_global_present = 1U;
    doc.layers[1].vector_mask_global = 1U;
    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 2, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RLE, 2U);

    return compose_and_write(path, &doc, 2U);
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

    memset(&vm->knots[0], 0, sizeof(vm->knots[0]));
    vm->knots[0].linked = 0U;
    vm->knots[0].preceding_h = fx24_frac(2, (psd89_s32)doc_width);
    vm->knots[0].preceding_v = fx24_frac(7, (psd89_s32)doc_height);
    vm->knots[0].anchor_h = fx24_frac(2, (psd89_s32)doc_width);
    vm->knots[0].anchor_v = fx24_frac(4, (psd89_s32)doc_height);
    vm->knots[0].leaving_h = fx24_frac(2, (psd89_s32)doc_width);
    vm->knots[0].leaving_v = fx24_frac(1, (psd89_s32)doc_height);

    memset(&vm->knots[1], 0, sizeof(vm->knots[1]));
    vm->knots[1].linked = 0U;
    vm->knots[1].preceding_h = fx24_frac(6, (psd89_s32)doc_width);
    vm->knots[1].preceding_v = fx24_frac(1, (psd89_s32)doc_height);
    vm->knots[1].anchor_h = fx24_frac(6, (psd89_s32)doc_width);
    vm->knots[1].anchor_v = fx24_frac(4, (psd89_s32)doc_height);
    vm->knots[1].leaving_h = fx24_frac(6, (psd89_s32)doc_width);
    vm->knots[1].leaving_v = fx24_frac(7, (psd89_s32)doc_height);
}

static int write_bezier_vector_fixture(const char *path)
{
    unsigned char white_r[64];
    unsigned char white_g[64];
    unsigned char white_b[64];
    unsigned char white_a[64];
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    psd89_u32 i;

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
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 8U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 1U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 8, 8, "ellipse", norm, 255U, 0U,
                        white_r, white_g, white_b, white_a, PSD89_COMP_RLE, 8U);
    set_vector_ellipse(&doc.layers[0], 8U, 8U);

    return compose_and_write(path, &doc, 8U);
}

static int write_group_fixture(const char *path, const char group_mode[4], psd89_u8 group_opacity, int nested)
{
    unsigned char bg_r[1] = { 128U };
    unsigned char bg_g[1] = { 128U };
    unsigned char bg_b[1] = { 128U };
    unsigned char bg_a[1] = { 255U };
    unsigned char red_r[1] = { 255U };
    unsigned char red_g[1] = { 0U };
    unsigned char red_b[1] = { 0U };
    unsigned char red_a[1] = { 255U };
    psd89_doc doc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    psd89_doc_init(&doc);
    doc.width = 1U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 1U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = (psd89_u16)(nested ? 6U : 4U);

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 1, "bg", norm, 255U, 0U,
                        bg_r, bg_g, bg_b, bg_a, PSD89_COMP_RLE, 1U);
    init_group_marker(&doc.layers[1], "</outer>", 3U, 0, 255U);
    if (nested) {
        init_group_marker(&doc.layers[2], "</inner>", 3U, 0, 255U);
        init_rgb_layer_rect(&doc.layers[3], 0, 0, 1, 1, "red", norm, 255U, 0U,
                            red_r, red_g, red_b, red_a, PSD89_COMP_RLE, 1U);
        init_group_marker(&doc.layers[4], "inner", 1U, group_mode, group_opacity);
        init_group_marker(&doc.layers[5], "outer", 1U, "pass", 255U);
    } else {
        init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 1, "red", norm, 255U, 0U,
                            red_r, red_g, red_b, red_a, PSD89_COMP_RLE, 1U);
        init_group_marker(&doc.layers[3], "group", 1U, group_mode, group_opacity);
    }
    return compose_and_write(path, &doc, 1U);
}

int main(void)
{
    if (!write_basic_user_mask_fixture("tests/corpus/rgb_user_mask_raw.psd", PSD89_COMP_RAW) ||
        !write_basic_user_mask_fixture("tests/corpus/rgb_user_mask_rle.psd", PSD89_COMP_RLE) ||
        !write_shifted_density_fixture("tests/corpus/rgb_mask_shift_density_rle.psd") ||
        !write_real_mask_fixture("tests/corpus/rgb_real_user_mask_rle.psd") ||
        !write_clbl_fixture("tests/corpus/rgb_clbl_group_rle.psd", 1U) ||
        !write_clbl_fixture("tests/corpus/rgb_clbl_individual_rle.psd", 0U) ||
        !write_vector_zip_fixture("tests/corpus/rgb_vector_mask_zip.psd", PSD89_COMP_ZIP, 0U) ||
        !write_vector_zip_fixture("tests/corpus/rgb_vector_mask_zip_pred.psd", PSD89_COMP_ZIP_PRED, 1U) ||
        !write_lmgm_clipping_fixture("tests/corpus/rgb_lmgm_clip_rle.psd") ||
        !write_vmgm_clipping_fixture("tests/corpus/rgb_vmgm_clip_rle.psd") ||
        !write_bezier_vector_fixture("tests/corpus/rgb_vector_bezier_rle.psd") ||
        !write_group_fixture("tests/corpus/rgb_group_pass_rle.psd", "pass", 255U, 0) ||
        !write_group_fixture("tests/corpus/rgb_group_mul_rle.psd", "mul ", 128U, 0) ||
        !write_group_fixture("tests/corpus/rgb_nested_group_mul_rle.psd", "mul ", 128U, 1)) {
        return 1;
    }
    printf("corpus_runner: wrote mask, clipping, vector, zip, v17, and v18 fixtures\n");
    return 0;
}
