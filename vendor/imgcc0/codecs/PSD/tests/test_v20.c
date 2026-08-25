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

static void add_solid_fill_lrfx(psd89_layer *layer,
                                psd89_u8 opacity,
                                psd89_u16 r,
                                psd89_u16 g,
                                psd89_u16 b)
{
    memset(&layer->lrfx, 0, sizeof(layer->lrfx));
    layer->lrfx.present = 1U;
    layer->lrfx.version = 0U;
    layer->lrfx.common_state_present = 1U;
    layer->lrfx.common_visible = 1U;
    layer->lrfx.solid_fill.present = 1U;
    layer->lrfx.solid_fill.enabled = 1U;
    layer->lrfx.solid_fill.opacity = opacity;
    memcpy(layer->lrfx.solid_fill.blend_mode, "norm", 4U);
    layer->lrfx.solid_fill.color[0] = 0U;
    layer->lrfx.solid_fill.color[1] = r;
    layer->lrfx.solid_fill.color[2] = g;
    layer->lrfx.solid_fill.color[3] = b;
    layer->lrfx.solid_fill.color[4] = 0U;
    layer->lrfx.solid_fill.native_color[0] = 0U;
    layer->lrfx.solid_fill.native_color[1] = r;
    layer->lrfx.solid_fill.native_color[2] = g;
    layer->lrfx.solid_fill.native_color[3] = b;
    layer->lrfx.solid_fill.native_color[4] = 0U;
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

static int test_lrfx_solid_fill_compose(void)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char red_r[1] = { 255U };
    static const unsigned char red_g[1] = { 0U };
    static const unsigned char red_b[1] = { 0U };
    static const unsigned char red_a[1] = { 255U };
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];
    psd89_doc doc;

    psd89_doc_init(&doc);
    doc.width = 1U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.layer_count = 1U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 1, "fx", norm, 255U, 0U,
                        red_r, red_g, red_b, red_a, PSD89_COMP_RAW, 1U);
    add_solid_fill_lrfx(&doc.layers[0], 255U, 0U, 0U, 65535U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 0U, 0U, 255U, 255U, "lrfx solid fill");
}

static int test_clipping_through_pass_wrapper(void)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char base_r[2] = { 0U, 0U };
    static const unsigned char base_g[2] = { 0U, 0U };
    static const unsigned char base_b[2] = { 0U, 0U };
    static const unsigned char base_a[2] = { 255U, 0U };
    static const unsigned char red_r[2] = { 255U, 255U };
    static const unsigned char red_g[2] = { 0U, 0U };
    static const unsigned char red_b[2] = { 0U, 0U };
    static const unsigned char red_a[2] = { 255U, 255U };
    unsigned char out_r[2];
    unsigned char out_g[2];
    unsigned char out_b[2];
    unsigned char out_a[2];
    psd89_doc doc;

    psd89_doc_init(&doc);
    doc.width = 2U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.layer_count = 4U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 2, "base", norm, 255U, 0U,
                        base_r, base_g, base_b, base_a, PSD89_COMP_RAW, 2U);
    init_group_marker(&doc.layers[1], "</grp>", 3U, 0, 255U);
    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 2, "clipped", norm, 255U, 1U,
                        red_r, red_g, red_b, red_a, PSD89_COMP_RAW, 2U);
    init_group_marker(&doc.layers[3], "grp", 1U, "pass", 255U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "wrapper clip px0")) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 0U, 0U, 0U, 0U, "wrapper clip px1");
}

static void build_advanced_doc(psd89_doc *doc,
                               unsigned char *comp_r,
                               unsigned char *comp_g,
                               unsigned char *comp_b,
                               unsigned char *comp_a)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char red_r[1] = { 255U };
    static const unsigned char red_g[1] = { 0U };
    static const unsigned char red_b[1] = { 0U };
    static const unsigned char red_a[1] = { 255U };
    psd89_layer *layer;

    psd89_doc_init(doc);
    doc->width = 1U;
    doc->height = 1U;
    doc->channels = 4U;
    doc->depth = 8U;
    doc->color_mode = PSD89_MODE_RGB;
    doc->merged_alpha_in_first_channel = 1;
    doc->layer_count = 1U;
    doc->composite_stride = 1U;
    doc->composite_write_compression = PSD89_COMP_RLE;

    layer = &doc->layers[0];
    init_rgb_layer_rect(layer, 0, 0, 1, 1, "advanced", norm, 255U, 0U,
                        red_r, red_g, red_b, red_a, PSD89_COMP_RLE, 1U);
    layer->blend_interior_present = 1U;
    layer->blend_interior = 0U;
    layer->knockout_present = 1U;
    layer->knockout = 1U;
    layer->transparency_shapes_layer_present = 1U;
    layer->transparency_shapes_layer = 0U;

    add_solid_fill_lrfx(layer, 255U, 0U, 65535U, 0U);

    layer->type_tool.present = 1U;
    layer->type_tool.version = 1U;
    layer->type_tool.transform[0] = PSD89_FX16_ONE;
    layer->type_tool.transform[1] = 0;
    layer->type_tool.transform[2] = 0;
    layer->type_tool.transform[3] = PSD89_FX16_ONE;
    layer->type_tool.transform[4] = 0;
    layer->type_tool.transform[5] = 0;
    layer->type_tool.text_version = 50U;
    layer->type_tool.text_descriptor_version = 16U;
    layer->type_tool.warp_version = 1U;
    layer->type_tool.warp_descriptor_version = 16U;
    layer->type_tool.bounds[0] = 0;
    layer->type_tool.bounds[1] = 0;
    layer->type_tool.bounds[2] = PSD89_FX16_ONE;
    layer->type_tool.bounds[3] = PSD89_FX16_ONE;

    layer->object_effects.present = 1U;
    layer->object_effects.object_version = 0U;
    layer->object_effects.descriptor_version = 16U;

    layer->smart_object.present = 1U;
    memcpy(layer->smart_object.tag_key, "SoLd", 4U);
    memcpy(layer->smart_object.type, "soLD", 4U);
    layer->smart_object.version = 4U;
    layer->smart_object.descriptor_version = 16U;

    if (compose_doc(doc, comp_r, comp_g, comp_b, comp_a, 1U)) {
        doc->composite_planes[0] = comp_r;
        doc->composite_planes[1] = comp_g;
        doc->composite_planes[2] = comp_b;
        doc->composite_planes[3] = comp_a;
    }
}

static int test_advanced_roundtrip(void)
{
    psd89_doc doc;
    psd89_doc parsed;
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];
    unsigned char buf[65536];
    psd89_memio m;
    psd89_io io;
    int rc;
    psd89_layer *layer;

    build_advanced_doc(&doc, out_r, out_g, out_b, out_a);
    if (doc.composite_planes[0] == 0) {
        fprintf(stderr, "compose failed while building advanced doc\n");
        return 0;
    }

    psd89_memio_init_write(&m, buf, sizeof(buf));
    psd89_memio_make_io(&m, &io);
    rc = psd89_write(&io, &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write returned %s\n", psd89_error_string(rc));
        return 0;
    }

    psd89_doc_init(&parsed);
    psd89_memio_init_read(&m, buf, m.size);
    psd89_memio_make_io(&m, &io);
    rc = psd89_read(&parsed, &io);
    if (rc != PSD89_OK) {
        fprintf(stderr, "read returned %s\n", psd89_error_string(rc));
        return 0;
    }

    layer = &parsed.layers[0];
    if (!layer->blend_interior_present || layer->blend_interior != 0U) {
        fprintf(stderr, "infx lost after roundtrip\n");
        return 0;
    }
    if (!layer->knockout_present || layer->knockout != 1U) {
        fprintf(stderr, "knko lost after roundtrip\n");
        return 0;
    }
    if (!layer->transparency_shapes_layer_present || layer->transparency_shapes_layer != 0U) {
        fprintf(stderr, "tsly lost after roundtrip\n");
        return 0;
    }
    if (!layer->lrfx.present || !layer->lrfx.solid_fill.present || !layer->lrfx.common_state_present) {
        fprintf(stderr, "lrFX lost after roundtrip\n");
        return 0;
    }
    if (!layer->type_tool.present || !layer->type_tool.text.summary.parsed || !layer->type_tool.warp.summary.parsed) {
        fprintf(stderr, "TySh lost after roundtrip\n");
        return 0;
    }
    if (!layer->object_effects.present || !layer->object_effects.descriptor.summary.parsed) {
        fprintf(stderr, "lfx2 lost after roundtrip\n");
        return 0;
    }
    if (!layer->smart_object.present || memcmp(layer->smart_object.tag_key, "SoLd", 4U) != 0 || !layer->smart_object.descriptor.summary.parsed) {
        fprintf(stderr, "smart object tag lost after roundtrip\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_lrfx_solid_fill_compose()) {
        return 1;
    }
    if (!test_clipping_through_pass_wrapper()) {
        return 1;
    }
    if (!test_advanced_roundtrip()) {
        return 1;
    }
    printf("test_v20: ok\n");
    return 0;
}
