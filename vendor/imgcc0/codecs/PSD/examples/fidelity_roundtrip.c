#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <string.h>

static void init_rgb_layer_rect(psd89_layer *layer,
                                const unsigned char *r,
                                const unsigned char *g,
                                const unsigned char *b,
                                const unsigned char *a,
                                psd89_u32 stride)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    memset(layer, 0, sizeof(*layer));
    layer->top = 0;
    layer->left = 0;
    layer->bottom = 1;
    layer->right = (psd89_s32)stride;
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, norm, 4U);
    layer->opacity = 255U;
    strncpy(layer->name, "fidelity", PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = PSD89_COMP_RLE;
    layer->channels[0].plane = r;
    layer->channels[0].stride = stride;
    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = PSD89_COMP_RLE;
    layer->channels[1].plane = g;
    layer->channels[1].stride = stride;
    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = PSD89_COMP_RLE;
    layer->channels[2].plane = b;
    layer->channels[2].stride = stride;
    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = PSD89_COMP_RLE;
    layer->channels[3].plane = a;
    layer->channels[3].stride = stride;
}

int main(void)
{
    static const unsigned char src_r[5] = { 255U, 255U, 255U, 0U, 0U };
    static const unsigned char src_g[5] = { 0U, 0U, 0U, 0U, 0U };
    static const unsigned char src_b[5] = { 0U, 0U, 0U, 0U, 0U };
    static const unsigned char src_a[5] = { 255U, 255U, 255U, 0U, 0U };
    unsigned char out_r[5];
    unsigned char out_g[5];
    unsigned char out_b[5];
    unsigned char out_a[5];
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
    doc.width = 5U;
    doc.height = 1U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.layer_count = 1U;
    doc.composite_stride = 5U;
    doc.composite_write_compression = PSD89_COMP_RLE;

    init_rgb_layer_rect(&doc.layers[0], src_r, src_g, src_b, src_a, 5U);

    memset(&doc.layers[0].lrfx, 0, sizeof(doc.layers[0].lrfx));
    doc.layers[0].lrfx.present = 1U;
    doc.layers[0].lrfx.common_state_present = 1U;
    doc.layers[0].lrfx.common_visible = 1U;

    doc.layers[0].lrfx.drop_shadow.present = 1U;
    doc.layers[0].lrfx.drop_shadow.enabled = 1U;
    doc.layers[0].lrfx.drop_shadow.opacity = 200U;
    doc.layers[0].lrfx.drop_shadow.intensity = 100;
    doc.layers[0].lrfx.drop_shadow.blur = 1;
    doc.layers[0].lrfx.drop_shadow.distance = 1;
    doc.layers[0].lrfx.drop_shadow.angle = 0;
    memcpy(doc.layers[0].lrfx.drop_shadow.blend_mode, "norm", 4U);

    doc.layers[0].lrfx.outer_glow.present = 1U;
    doc.layers[0].lrfx.outer_glow.enabled = 1U;
    doc.layers[0].lrfx.outer_glow.opacity = 160U;
    doc.layers[0].lrfx.outer_glow.intensity = 100;
    doc.layers[0].lrfx.outer_glow.blur = 1;
    memcpy(doc.layers[0].lrfx.outer_glow.blend_mode, "norm", 4U);
    doc.layers[0].lrfx.outer_glow.color[1] = 65535U;
    doc.layers[0].lrfx.outer_glow.color[2] = 65535U;
    doc.layers[0].lrfx.outer_glow.color[3] = 0U;

    doc.layers[0].lrfx.inner_glow.present = 1U;
    doc.layers[0].lrfx.inner_glow.enabled = 1U;
    doc.layers[0].lrfx.inner_glow.opacity = 160U;
    doc.layers[0].lrfx.inner_glow.intensity = 100;
    doc.layers[0].lrfx.inner_glow.blur = 1;
    memcpy(doc.layers[0].lrfx.inner_glow.blend_mode, "norm", 4U);
    doc.layers[0].lrfx.inner_glow.color[1] = 0U;
    doc.layers[0].lrfx.inner_glow.color[2] = 65535U;
    doc.layers[0].lrfx.inner_glow.color[3] = 0U;

    doc.layers[0].lrfx.bevel.present = 1U;
    doc.layers[0].lrfx.bevel.enabled = 1U;
    doc.layers[0].lrfx.bevel.highlight_opacity = 180U;
    doc.layers[0].lrfx.bevel.shadow_opacity = 180U;
    doc.layers[0].lrfx.bevel.angle = 0;
    memcpy(doc.layers[0].lrfx.bevel.highlight_blend_mode, "norm", 4U);
    memcpy(doc.layers[0].lrfx.bevel.shadow_blend_mode, "norm", 4U);
    doc.layers[0].lrfx.bevel.highlight_color[1] = 65535U;
    doc.layers[0].lrfx.bevel.highlight_color[2] = 65535U;
    doc.layers[0].lrfx.bevel.highlight_color[3] = 65535U;
    doc.layers[0].lrfx.bevel.shadow_color[1] = 0U;
    doc.layers[0].lrfx.bevel.shadow_color[2] = 0U;
    doc.layers[0].lrfx.bevel.shadow_color[3] = 0U;

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
                                 5U,
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

    rc = psd89_write_path("examples/example_fidelity_roundtrip.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    psd89_doc_init(&parsed);
    rc = psd89_read_path(&parsed, "examples/example_fidelity_roundtrip.psd");
    if (rc != PSD89_OK) {
        fprintf(stderr, "read failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    printf("fidelity_roundtrip: wrote examples/example_fidelity_roundtrip.psd ");
    printf("(ds=%u og=%u ig=%u bevel=%u)\n",
           (unsigned int)parsed.layers[0].lrfx.drop_shadow.present,
           (unsigned int)parsed.layers[0].lrfx.outer_glow.present,
           (unsigned int)parsed.layers[0].lrfx.inner_glow.present,
           (unsigned int)parsed.layers[0].lrfx.bevel.present);
    return 0;
}
