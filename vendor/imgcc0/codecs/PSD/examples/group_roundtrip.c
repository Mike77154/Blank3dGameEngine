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
                                const unsigned char *r,
                                const unsigned char *g,
                                const unsigned char *b,
                                const unsigned char *a)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = top;
    layer->left = left;
    layer->bottom = bottom;
    layer->right = right;
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = opacity;
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = PSD89_COMP_RLE;
    layer->channels[0].plane = r;
    layer->channels[0].stride = (psd89_u32)(right - left);
    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = PSD89_COMP_RLE;
    layer->channels[1].plane = g;
    layer->channels[1].stride = (psd89_u32)(right - left);
    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = PSD89_COMP_RLE;
    layer->channels[2].plane = b;
    layer->channels[2].stride = (psd89_u32)(right - left);
    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = PSD89_COMP_RLE;
    layer->channels[3].plane = a;
    layer->channels[3].stride = (psd89_u32)(right - left);
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

int main(void)
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
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];
    unsigned char scratch_r[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_g[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_b[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_a[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char *dst_color[3];
    unsigned char *scratch_color[3];
    psd89_compose_options opt;
    psd89_doc doc;
    int rc;

    psd89_doc_init(&doc);
    doc.width = 1U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 1U;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 4U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 1, "bg", norm, 255U,
                        bg_r, bg_g, bg_b, bg_a);
    init_group_marker(&doc.layers[1], "</folder>", 3U, 0, 255U);
    init_rgb_layer_rect(&doc.layers[2], 0, 0, 1, 1, "red", norm, 255U,
                        red_r, red_g, red_b, red_a);
    init_group_marker(&doc.layers[3], "folder", 1U, "mul ", 128U);
    doc.layers[3].transparency_shapes_layer_present = 1U;
    doc.layers[3].transparency_shapes_layer = 1U;

    dst_color[0] = out_r;
    dst_color[1] = out_g;
    dst_color[2] = out_b;
    scratch_color[0] = scratch_r;
    scratch_color[1] = scratch_g;
    scratch_color[2] = scratch_b;
    psd89_compose_options_init(&opt);
    rc = psd89_compose_layers_u8(&doc,
                                 0,
                                 dst_color,
                                 out_a,
                                 1U,
                                 scratch_color,
                                 scratch_a,
                                 PSD89_MAX_COMPOSE_ROW_BYTES,
                                 &opt);
    if (rc != PSD89_OK) {
        fprintf(stderr, "compose failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;

    rc = psd89_write_path("examples/example_group_roundtrip.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    printf("group_roundtrip: wrote examples/example_group_roundtrip.psd\n");
    return 0;
}
