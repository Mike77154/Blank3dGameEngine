#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <string.h>

static void init_rgb_layer_rect(psd89_layer *layer,
                                const unsigned char *r,
                                const unsigned char *g,
                                const unsigned char *b,
                                const unsigned char *a)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    memset(layer, 0, sizeof(*layer));
    layer->top = 0;
    layer->left = 0;
    layer->bottom = 1;
    layer->right = 1;
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, norm, 4U);
    layer->opacity = 255U;
    strncpy(layer->name, "advanced", PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = PSD89_COMP_RLE;
    layer->channels[0].plane = r;
    layer->channels[0].stride = 1U;
    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = PSD89_COMP_RLE;
    layer->channels[1].plane = g;
    layer->channels[1].stride = 1U;
    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = PSD89_COMP_RLE;
    layer->channels[2].plane = b;
    layer->channels[2].stride = 1U;
    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = PSD89_COMP_RLE;
    layer->channels[3].plane = a;
    layer->channels[3].stride = 1U;
}

static void add_solid_fill_lrfx(psd89_layer *layer)
{
    memset(&layer->lrfx, 0, sizeof(layer->lrfx));
    layer->lrfx.present = 1U;
    layer->lrfx.common_state_present = 1U;
    layer->lrfx.common_visible = 1U;
    layer->lrfx.solid_fill.present = 1U;
    layer->lrfx.solid_fill.enabled = 1U;
    layer->lrfx.solid_fill.opacity = 255U;
    memcpy(layer->lrfx.solid_fill.blend_mode, "norm", 4U);
    layer->lrfx.solid_fill.color[1] = 0U;
    layer->lrfx.solid_fill.color[2] = 65535U;
    layer->lrfx.solid_fill.color[3] = 0U;
    layer->lrfx.solid_fill.native_color[1] = 0U;
    layer->lrfx.solid_fill.native_color[2] = 65535U;
    layer->lrfx.solid_fill.native_color[3] = 0U;
}

int main(void)
{
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
    psd89_doc parsed;
    int rc;

    psd89_doc_init(&doc);
    doc.width = 1U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.layer_count = 1U;
    doc.composite_stride = 1U;
    doc.composite_write_compression = PSD89_COMP_RLE;

    init_rgb_layer_rect(&doc.layers[0], red_r, red_g, red_b, red_a);
    add_solid_fill_lrfx(&doc.layers[0]);
    doc.layers[0].blend_interior_present = 1U;
    doc.layers[0].blend_interior = 0U;
    doc.layers[0].knockout_present = 1U;
    doc.layers[0].knockout = 1U;
    doc.layers[0].transparency_shapes_layer_present = 1U;
    doc.layers[0].transparency_shapes_layer = 0U;

    doc.layers[0].type_tool.present = 1U;
    doc.layers[0].type_tool.version = 1U;
    doc.layers[0].type_tool.transform[0] = PSD89_FX16_ONE;
    doc.layers[0].type_tool.transform[3] = PSD89_FX16_ONE;
    doc.layers[0].type_tool.text_version = 50U;
    doc.layers[0].type_tool.text_descriptor_version = 16U;
    doc.layers[0].type_tool.warp_version = 1U;
    doc.layers[0].type_tool.warp_descriptor_version = 16U;
    doc.layers[0].type_tool.bounds[2] = PSD89_FX16_ONE;
    doc.layers[0].type_tool.bounds[3] = PSD89_FX16_ONE;

    doc.layers[0].object_effects.present = 1U;
    doc.layers[0].object_effects.object_version = 0U;
    doc.layers[0].object_effects.descriptor_version = 16U;

    doc.layers[0].smart_object.present = 1U;
    memcpy(doc.layers[0].smart_object.tag_key, "SoLd", 4U);
    memcpy(doc.layers[0].smart_object.type, "soLD", 4U);
    doc.layers[0].smart_object.version = 4U;
    doc.layers[0].smart_object.descriptor_version = 16U;

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

    rc = psd89_write_path("examples/example_advanced_roundtrip.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    psd89_doc_init(&parsed);
    rc = psd89_read_path(&parsed, "examples/example_advanced_roundtrip.psd");
    if (rc != PSD89_OK) {
        fprintf(stderr, "read failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    printf("advanced_roundtrip: wrote examples/example_advanced_roundtrip.psd ");
    printf("(lrFX=%u TySh=%u lfx2=%u So=%u)\n",
           (unsigned int)parsed.layers[0].lrfx.present,
           (unsigned int)parsed.layers[0].type_tool.present,
           (unsigned int)parsed.layers[0].object_effects.present,
           (unsigned int)parsed.layers[0].smart_object.present);
    return 0;
}
