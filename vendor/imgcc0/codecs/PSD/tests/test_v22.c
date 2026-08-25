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
                                const unsigned char *r,
                                const unsigned char *g,
                                const unsigned char *b,
                                const unsigned char *a,
                                psd89_u32 stride)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = top;
    layer->left = left;
    layer->bottom = bottom;
    layer->right = right;
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = 255U;
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = PSD89_COMP_RAW;
    layer->channels[0].plane = r;
    layer->channels[0].stride = stride;

    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = PSD89_COMP_RAW;
    layer->channels[1].plane = g;
    layer->channels[1].stride = stride;

    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = PSD89_COMP_RAW;
    layer->channels[2].plane = b;
    layer->channels[2].stride = stride;

    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = PSD89_COMP_RAW;
    layer->channels[3].plane = a;
    layer->channels[3].stride = stride;
}

static void init_doc(psd89_doc *doc, psd89_u32 width, psd89_u32 height)
{
    psd89_doc_init(doc);
    doc->width = width;
    doc->height = height;
    doc->channels = 4U;
    doc->depth = 8U;
    doc->color_mode = PSD89_MODE_RGB;
    doc->layer_count = 1U;
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

static void clear_lrfx(psd89_layer *layer)
{
    memset(&layer->lrfx, 0, sizeof(layer->lrfx));
    layer->lrfx.present = 1U;
    layer->lrfx.common_state_present = 1U;
    layer->lrfx.common_visible = 1U;
}

static int test_drop_shadow_live(void)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char r[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char g[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char b[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char a[5] = { 0U, 0U, 255U, 0U, 0U };
    unsigned char out_r[5];
    unsigned char out_g[5];
    unsigned char out_b[5];
    unsigned char out_a[5];
    psd89_doc doc;

    init_doc(&doc, 5U, 1U);
    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 5, "drop", norm, r, g, b, a, 5U);
    clear_lrfx(&doc.layers[0]);
    doc.layers[0].lrfx.drop_shadow.present = 1U;
    doc.layers[0].lrfx.drop_shadow.enabled = 1U;
    doc.layers[0].lrfx.drop_shadow.opacity = 255U;
    doc.layers[0].lrfx.drop_shadow.intensity = 100;
    doc.layers[0].lrfx.drop_shadow.blur = 1;
    doc.layers[0].lrfx.drop_shadow.distance = 1;
    doc.layers[0].lrfx.drop_shadow.angle = 0;
    memcpy(doc.layers[0].lrfx.drop_shadow.blend_mode, "norm", 4U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 5U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 2U, 255U, 255U, 255U, 255U, "drop center")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 3U, 0U, 0U, 0U, 255U, "drop main shadow")) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 4U, 0U, 0U, 0U, 128U, "drop tail");
}

static int test_outer_glow_live(void)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char r[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char g[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char b[5] = { 0U, 0U, 255U, 0U, 0U };
    static const unsigned char a[5] = { 0U, 0U, 255U, 0U, 0U };
    unsigned char out_r[5];
    unsigned char out_g[5];
    unsigned char out_b[5];
    unsigned char out_a[5];
    psd89_doc doc;

    init_doc(&doc, 5U, 1U);
    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 5, "glow", norm, r, g, b, a, 5U);
    clear_lrfx(&doc.layers[0]);
    doc.layers[0].lrfx.outer_glow.present = 1U;
    doc.layers[0].lrfx.outer_glow.enabled = 1U;
    doc.layers[0].lrfx.outer_glow.opacity = 255U;
    doc.layers[0].lrfx.outer_glow.intensity = 100;
    doc.layers[0].lrfx.outer_glow.blur = 1;
    memcpy(doc.layers[0].lrfx.outer_glow.blend_mode, "norm", 4U);
    doc.layers[0].lrfx.outer_glow.color[1] = 65535U;
    doc.layers[0].lrfx.outer_glow.color[2] = 65535U;
    doc.layers[0].lrfx.outer_glow.color[3] = 0U;

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 5U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 255U, 255U, 0U, 128U, "outer glow left")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 2U, 255U, 255U, 255U, 255U, "outer glow center")) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 3U, 255U, 255U, 0U, 128U, "outer glow right");
}

static int test_inner_shadow_glow_and_bevel_live(void)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char r[5] = { 255U, 255U, 255U, 0U, 0U };
    static const unsigned char g[5] = { 0U, 0U, 0U, 0U, 0U };
    static const unsigned char b[5] = { 0U, 0U, 0U, 0U, 0U };
    static const unsigned char a[5] = { 255U, 255U, 255U, 0U, 0U };
    unsigned char out_r[5];
    unsigned char out_g[5];
    unsigned char out_b[5];
    unsigned char out_a[5];
    psd89_doc doc;

    init_doc(&doc, 5U, 1U);
    init_rgb_layer_rect(&doc.layers[0], 0, 0, 1, 5, "inner", norm, r, g, b, a, 5U);

    clear_lrfx(&doc.layers[0]);
    doc.layers[0].lrfx.inner_shadow.present = 1U;
    doc.layers[0].lrfx.inner_shadow.enabled = 1U;
    doc.layers[0].lrfx.inner_shadow.opacity = 255U;
    doc.layers[0].lrfx.inner_shadow.intensity = 100;
    doc.layers[0].lrfx.inner_shadow.blur = 1;
    doc.layers[0].lrfx.inner_shadow.distance = 1;
    doc.layers[0].lrfx.inner_shadow.angle = 0;
    memcpy(doc.layers[0].lrfx.inner_shadow.blend_mode, "norm", 4U);

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 5U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 128U, 0U, 0U, 255U, "inner shadow edge")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 255U, 0U, 0U, 255U, "inner shadow mid")) {
        return 0;
    }

    clear_lrfx(&doc.layers[0]);
    doc.layers[0].lrfx.inner_glow.present = 1U;
    doc.layers[0].lrfx.inner_glow.enabled = 1U;
    doc.layers[0].lrfx.inner_glow.opacity = 255U;
    doc.layers[0].lrfx.inner_glow.intensity = 100;
    doc.layers[0].lrfx.inner_glow.blur = 1;
    memcpy(doc.layers[0].lrfx.inner_glow.blend_mode, "norm", 4U);
    doc.layers[0].lrfx.inner_glow.color[1] = 0U;
    doc.layers[0].lrfx.inner_glow.color[2] = 65535U;
    doc.layers[0].lrfx.inner_glow.color[3] = 0U;

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 5U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 0U, 255U, 0U, 255U, "inner glow left")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 128U, 128U, 0U, 255U, "inner glow center")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 2U, 0U, 255U, 0U, 255U, "inner glow right")) {
        return 0;
    }

    clear_lrfx(&doc.layers[0]);
    doc.layers[0].lrfx.bevel.present = 1U;
    doc.layers[0].lrfx.bevel.enabled = 1U;
    doc.layers[0].lrfx.bevel.highlight_opacity = 255U;
    doc.layers[0].lrfx.bevel.shadow_opacity = 255U;
    doc.layers[0].lrfx.bevel.angle = 0;
    memcpy(doc.layers[0].lrfx.bevel.highlight_blend_mode, "norm", 4U);
    memcpy(doc.layers[0].lrfx.bevel.shadow_blend_mode, "norm", 4U);
    doc.layers[0].lrfx.bevel.highlight_color[1] = 65535U;
    doc.layers[0].lrfx.bevel.highlight_color[2] = 65535U;
    doc.layers[0].lrfx.bevel.highlight_color[3] = 65535U;
    doc.layers[0].lrfx.bevel.shadow_color[1] = 0U;
    doc.layers[0].lrfx.bevel.shadow_color[2] = 0U;
    doc.layers[0].lrfx.bevel.shadow_color[3] = 0U;

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 5U)) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 0U, 255U, 255U, 255U, 255U, "bevel highlight")) {
        return 0;
    }
    if (!expect_pixel_rgba(out_r, out_g, out_b, out_a, 1U, 255U, 0U, 0U, 255U, "bevel mid")) {
        return 0;
    }
    return expect_pixel_rgba(out_r, out_g, out_b, out_a, 2U, 0U, 0U, 0U, 255U, "bevel shadow");
}

int main(void)
{
    if (!test_drop_shadow_live()) {
        return 1;
    }
    if (!test_outer_glow_live()) {
        return 1;
    }
    if (!test_inner_shadow_glow_and_bevel_live()) {
        return 1;
    }
    printf("test_v22: ok\n");
    return 0;
}
