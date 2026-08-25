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

static void build_group_doc(psd89_doc *doc,
                            const char group_mode[4],
                            psd89_u8 group_opacity,
                            psd89_u8 group_tsly,
                            int nested)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char bg_r[1] = { 128U };
    static const unsigned char bg_g[1] = { 128U };
    static const unsigned char bg_b[1] = { 128U };
    static const unsigned char bg_a[1] = { 255U };
    static const unsigned char red_r[1] = { 255U };
    static const unsigned char red_g[1] = { 0U };
    static const unsigned char red_b[1] = { 0U };
    static const unsigned char red_a[1] = { 255U };

    psd89_doc_init(doc);
    doc->width = 1U;
    doc->height = 1U;
    doc->channels = 4U;
    doc->depth = 8U;
    doc->color_mode = PSD89_MODE_RGB;
    doc->layer_count = (psd89_u16)(nested ? 6U : 4U);

    init_rgb_layer_rect(&doc->layers[0], 0, 0, 1, 1, "bg", norm, 255U, 0U,
                        bg_r, bg_g, bg_b, bg_a, PSD89_COMP_RAW, 1U);
    init_group_marker(&doc->layers[1], "</outer>", 3U, 0, 255U);
    if (nested) {
        init_group_marker(&doc->layers[2], "</inner>", 3U, 0, 255U);
        init_rgb_layer_rect(&doc->layers[3], 0, 0, 1, 1, "red", norm, 255U, 0U,
                            red_r, red_g, red_b, red_a, PSD89_COMP_RAW, 1U);
        init_group_marker(&doc->layers[4], "inner", 1U, group_mode, group_opacity);
        doc->layers[4].transparency_shapes_layer_present = 1U;
        doc->layers[4].transparency_shapes_layer = group_tsly;
        init_group_marker(&doc->layers[5], "outer", 1U, "pass", 255U);
    } else {
        init_rgb_layer_rect(&doc->layers[2], 0, 0, 1, 1, "red", norm, 255U, 0U,
                            red_r, red_g, red_b, red_a, PSD89_COMP_RAW, 1U);
        init_group_marker(&doc->layers[3], "group", 1U, group_mode, group_opacity);
        doc->layers[3].transparency_shapes_layer_present = 1U;
        doc->layers[3].transparency_shapes_layer = group_tsly;
    }
}

static int test_group_pass_through(void)
{
    psd89_doc doc;
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];

    build_group_doc(&doc, "pass", 255U, 1U, 0);
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 0U, 0U, 255U, "group pass-through");
}

static int test_group_multiply_opacity(void)
{
    psd89_doc doc;
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];

    build_group_doc(&doc, "mul ", 128U, 1U, 0);
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 128U, 64U, 64U, 255U, "group multiply opacity");
}

static int test_nested_groups(void)
{
    psd89_doc doc;
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];

    build_group_doc(&doc, "mul ", 128U, 1U, 1);
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 128U, 64U, 64U, 255U, "nested groups");
}

static int test_lsct_tsly_roundtrip(void)
{
    psd89_doc doc;
    psd89_doc parsed;
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];
    unsigned char buf[8192];
    psd89_memio m;
    psd89_io io;
    int rc;

    build_group_doc(&doc, "mul ", 128U, 0U, 0);
    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;
    doc.composite_stride = 1U;
    doc.composite_write_compression = PSD89_COMP_RLE;

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
    if (!parsed.layers[1].section_divider_present || parsed.layers[1].section_divider_type != 3U) {
        fprintf(stderr, "missing bounding section divider after roundtrip\n");
        return 0;
    }
    if (!parsed.layers[3].section_divider_present || parsed.layers[3].section_divider_type != 1U) {
        fprintf(stderr, "missing closing folder layer after roundtrip\n");
        return 0;
    }
    if (memcmp(parsed.layers[3].section_divider_blend_mode, "mul ", 4U) != 0) {
        fprintf(stderr, "section divider blend mode lost after roundtrip\n");
        return 0;
    }
    if (!parsed.layers[3].transparency_shapes_layer_present || parsed.layers[3].transparency_shapes_layer != 0U) {
        fprintf(stderr, "tsly lost after roundtrip\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_group_pass_through()) {
        return 1;
    }
    if (!test_group_multiply_opacity()) {
        return 1;
    }
    if (!test_nested_groups()) {
        return 1;
    }
    if (!test_lsct_tsly_roundtrip()) {
        return 1;
    }
    printf("test_v18: ok\n");
    return 0;
}
