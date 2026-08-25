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

static int expect_equal(const unsigned char *got, const unsigned char *want, psd89_u32 n, const char *label)
{
    psd89_u32 i;
    for (i = 0U; i < n; ++i) {
        if (got[i] != want[i]) {
            fprintf(stderr, "%s mismatch at %u: got=%u want=%u\n",
                    label, i, (unsigned int)got[i], (unsigned int)want[i]);
            return 0;
        }
    }
    return 1;
}

static int expect_close_u8(unsigned char got, unsigned char want, unsigned int tol, const char *label)
{
    unsigned int diff;
    if (got > want) {
        diff = (unsigned int)got - (unsigned int)want;
    } else {
        diff = (unsigned int)want - (unsigned int)got;
    }
    if (diff > tol) {
        fprintf(stderr, "%s mismatch: got=%u want=%u tol=%u\n",
                label, (unsigned int)got, (unsigned int)want, tol);
        return 0;
    }
    return 1;
}

static int compose_doc(psd89_doc *doc,
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

static int test_shifted_mask_density_feather(void)
{
    unsigned char blue_r[16];
    unsigned char blue_g[16];
    unsigned char blue_b[16];
    unsigned char blue_a[16];
    unsigned char red_r[9];
    unsigned char red_g[9];
    unsigned char red_b[9];
    unsigned char red_a[9];
    unsigned char mask_2x2[4];
    unsigned char out_r[16];
    unsigned char out_g[16];
    unsigned char out_b[16];
    unsigned char out_a[16];
    unsigned char dec_m[4];
    psd89_doc doc;
    psd89_doc parsed;
    FILE *fp;
    psd89_io io;
    int rc;
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
    mask_2x2[0] = 255U;
    mask_2x2[1] = 255U;
    mask_2x2[2] = 255U;
    mask_2x2[3] = 255U;

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
                        blue_r, blue_g, blue_b, blue_a, PSD89_COMP_RAW, 4U);
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

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 4U)) {
        return 0;
    }

    for (i = 0U; i < 16U; ++i) {
        unsigned int x;
        unsigned int y;
        x = (unsigned int)(i % 4U);
        y = (unsigned int)(i / 4U);
        if (x >= 1U && x <= 2U && y >= 1U && y <= 2U) {
            if (!expect_close_u8(out_r[i], 64U, 2U, "shifted mask R") ||
                !expect_close_u8(out_g[i], 0U, 1U, "shifted mask G") ||
                !expect_close_u8(out_b[i], 191U, 2U, "shifted mask B") ||
                !expect_equal(out_a + i, blue_a, 1U, "shifted mask A")) {
                return 0;
            }
        } else {
            if (out_r[i] != 0U || out_g[i] != 0U || out_b[i] != 255U || out_a[i] != 255U) {
                fprintf(stderr, "shifted mask outside region changed at %u\n", i);
                return 0;
            }
        }
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;
    rc = psd89_write_path("tests/test_v14_shifted_mask.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write_path shifted mask returned %s\n", psd89_error_string(rc));
        return 0;
    }
    rc = psd89_read_path(&parsed, "tests/test_v14_shifted_mask.psd");
    if (rc != PSD89_OK) {
        fprintf(stderr, "read_path shifted mask returned %s\n", psd89_error_string(rc));
        return 0;
    }
    if (!parsed.layers[1].user_mask.present ||
        (parsed.layers[1].user_mask.flags & 0x01U) == 0U ||
        (parsed.layers[1].user_mask.flags & 0x10U) == 0U ||
        !parsed.layers[1].user_mask.user_density_present ||
        parsed.layers[1].user_mask.user_density != 128U ||
        !parsed.layers[1].user_mask.user_feather_present ||
        parsed.layers[1].user_mask.user_feather != psd89_fx16_from_int(2)) {
        fprintf(stderr, "parsed shifted mask metadata mismatch\n");
        return 0;
    }

    fp = fopen("tests/test_v14_shifted_mask.psd", "rb");
    if (fp == 0) {
        fprintf(stderr, "failed to reopen shifted mask fixture\n");
        return 0;
    }
    psd89_stdio_make_io(fp, &io);
    rc = psd89_decode_layer_channel_u8(&parsed, &io, 1U, PSD89_CH_LAYER_MASK, dec_m, 2U);
    fclose(fp);
    if (rc != PSD89_OK) {
        fprintf(stderr, "decode shifted layer mask returned %s\n", psd89_error_string(rc));
        return 0;
    }
    if (!expect_equal(dec_m, mask_2x2, 4U, "shifted mask decode")) {
        return 0;
    }

    return 1;
}

static int test_real_mask_selection(void)
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
    unsigned char out_r[4];
    unsigned char out_g[4];
    unsigned char out_b[4];
    unsigned char out_a[4];
    unsigned char dec_rendered[4];
    unsigned char dec_real[4];
    psd89_doc doc;
    psd89_doc parsed;
    FILE *fp;
    psd89_io io;
    int rc;
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
                        blue_r, blue_g, blue_b, blue_a, PSD89_COMP_RAW, 2U);
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

    if (!compose_doc(&doc, out_r, out_g, out_b, out_a, 2U)) {
        return 0;
    }
    if (out_r[0] != 0U || out_g[0] != 255U || out_b[0] != 0U) {
        fprintf(stderr, "real mask semantics picked -3 instead of effective -2\n");
        return 0;
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;
    rc = psd89_write_path("tests/test_v14_real_mask.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write_path real mask returned %s\n", psd89_error_string(rc));
        return 0;
    }
    rc = psd89_read_path(&parsed, "tests/test_v14_real_mask.psd");
    if (rc != PSD89_OK) {
        fprintf(stderr, "read_path real mask returned %s\n", psd89_error_string(rc));
        return 0;
    }

    fp = fopen("tests/test_v14_real_mask.psd", "rb");
    if (fp == 0) {
        fprintf(stderr, "failed to reopen real mask fixture\n");
        return 0;
    }
    psd89_stdio_make_io(fp, &io);
    rc = psd89_decode_layer_channel_u8(&parsed, &io, 1U, PSD89_CH_LAYER_MASK, dec_rendered, 2U);
    if (rc == PSD89_OK) {
        rc = psd89_decode_layer_channel_u8(&parsed, &io, 1U, PSD89_CH_REAL_LAYER_MASK, dec_real, 2U);
    }
    fclose(fp);
    if (rc != PSD89_OK) {
        fprintf(stderr, "decode real/effective masks returned %s\n", psd89_error_string(rc));
        return 0;
    }
    if (!expect_equal(dec_rendered, mask_rendered, 4U, "effective mask decode") ||
        !expect_equal(dec_real, mask_real, 4U, "real mask decode")) {
        return 0;
    }
    return 1;
}

static int test_clbl_group_semantics(void)
{
    unsigned char gray[1] = { 128U };
    unsigned char white[1] = { 255U };
    unsigned char zero[1] = { 0U };
    unsigned char alpha[1] = { 255U };
    unsigned char red[1] = { 255U };
    unsigned char out_r[1];
    unsigned char out_g[1];
    unsigned char out_b[1];
    unsigned char out_a[1];
    psd89_doc doc_group;
    psd89_doc doc_individual;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    char mul[4] = { 'm', 'u', 'l', ' ' };

    psd89_doc_init(&doc_group);
    doc_group.width = 1;
    doc_group.height = 1;
    doc_group.channels = 4;
    doc_group.depth = 8;
    doc_group.color_mode = PSD89_MODE_RGB;
    doc_group.layer_count = 3U;

    init_rgb_layer_rect(&doc_group.layers[0], 0, 0, 1, 1, "gray", norm, 255U, 0U,
                        gray, gray, gray, alpha, PSD89_COMP_RAW, 1U);
    init_rgb_layer_rect(&doc_group.layers[1], 0, 0, 1, 1, "base-white", mul, 255U, 0U,
                        white, white, white, alpha, PSD89_COMP_RAW, 1U);
    init_rgb_layer_rect(&doc_group.layers[2], 0, 0, 1, 1, "clip-red", norm, 255U, 1U,
                        red, zero, zero, alpha, PSD89_COMP_RAW, 1U);
    doc_group.layers[1].blend_clipped_present = 1U;
    doc_group.layers[1].blend_clipped = 1U;

    if (!compose_doc(&doc_group, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    if (!expect_close_u8(out_r[0], 128U, 1U, "clbl group R") ||
        !expect_close_u8(out_g[0], 0U, 1U, "clbl group G") ||
        !expect_close_u8(out_b[0], 0U, 1U, "clbl group B")) {
        return 0;
    }

    doc_individual = doc_group;
    doc_individual.layers[1].blend_clipped = 0U;
    if (!compose_doc(&doc_individual, out_r, out_g, out_b, out_a, 1U)) {
        return 0;
    }
    if (out_r[0] != 255U || out_g[0] != 0U || out_b[0] != 0U) {
        fprintf(stderr, "clbl=false path did not preserve clipped layer appearance\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_shifted_mask_density_feather()) {
        return 1;
    }
    if (!test_real_mask_selection()) {
        return 1;
    }
    if (!test_clbl_group_semantics()) {
        return 1;
    }
    printf("test_v14: ok\n");
    return 0;
}
