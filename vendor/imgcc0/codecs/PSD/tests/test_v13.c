#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <string.h>

static void init_rgb_layer(psd89_layer *layer,
                           const char *name,
                           const char blend_mode[4],
                           psd89_u8 opacity,
                           const unsigned char *r,
                           const unsigned char *g,
                           const unsigned char *b,
                           const unsigned char *a,
                           const unsigned char *m,
                           psd89_u16 compression,
                           psd89_u32 stride)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = 0;
    layer->left = 0;
    layer->bottom = 2;
    layer->right = 2;
    layer->channel_count = (psd89_u16)(m != 0 ? 5U : 4U);
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = opacity;
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

    if (m != 0) {
        layer->channels[4].id = PSD89_CH_LAYER_MASK;
        layer->channels[4].compression = compression;
        layer->channels[4].plane = m;
        layer->channels[4].stride = stride;
        layer->user_mask.present = 1U;
        layer->user_mask.top = 0;
        layer->user_mask.left = 0;
        layer->user_mask.bottom = 2;
        layer->user_mask.right = 2;
        layer->user_mask.default_color = 255U;
        layer->user_mask.flags = 0U;
    }
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

int main(void)
{
    unsigned char blue_r[4] = { 0, 0, 0, 0 };
    unsigned char blue_g[4] = { 0, 0, 0, 0 };
    unsigned char blue_b[4] = { 255, 255, 255, 255 };
    unsigned char blue_a[4] = { 255, 255, 255, 255 };

    unsigned char red_r[4] = { 255, 255, 255, 255 };
    unsigned char red_g[4] = { 0, 0, 0, 0 };
    unsigned char red_b[4] = { 0, 0, 0, 0 };
    unsigned char red_a[4] = { 255, 255, 255, 255 };
    unsigned char red_m[4] = { 255, 0, 128, 255 };

    unsigned char want_r[4] = { 255, 0, 128, 255 };
    unsigned char want_g[4] = { 0, 0, 0, 0 };
    unsigned char want_b[4] = { 0, 255, 127, 0 };
    unsigned char want_a[4] = { 255, 255, 255, 255 };

    unsigned char out_r[4];
    unsigned char out_g[4];
    unsigned char out_b[4];
    unsigned char out_a[4];
    unsigned char dec_r[4];
    unsigned char dec_g[4];
    unsigned char dec_b[4];
    unsigned char dec_a[4];
    unsigned char dec_m[4];
    unsigned char scratch_r[4];
    unsigned char scratch_g[4];
    unsigned char scratch_b[4];
    unsigned char scratch_a[4];
    unsigned char *dst_color[3];
    unsigned char *scratch_color[3];
    unsigned char *decode_planes[4];
    psd89_doc doc;
    psd89_doc parsed;
    psd89_compose_options opt;
    psd89_io io;
    FILE *fp;
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
    doc.layer_count = 2;

    init_rgb_layer(&doc.layers[0], "blue-base", norm, 255U,
                   blue_r, blue_g, blue_b, blue_a, 0, PSD89_COMP_RAW, 2U);
    init_rgb_layer(&doc.layers[1], "red-masked", norm, 255U,
                   red_r, red_g, red_b, red_a, red_m, PSD89_COMP_RLE, 2U);

    dst_color[0] = out_r;
    dst_color[1] = out_g;
    dst_color[2] = out_b;
    scratch_color[0] = scratch_r;
    scratch_color[1] = scratch_g;
    scratch_color[2] = scratch_b;

    psd89_compose_options_init(&opt);
    rc = psd89_compose_layers_u8(&doc, 0, dst_color, out_a, 2U,
                                 scratch_color, scratch_a, 2U, &opt);
    if (rc != PSD89_OK) {
        fprintf(stderr, "compose returned %s\n", psd89_error_string(rc));
        return 1;
    }

    if (!expect_equal(out_r, want_r, 4U, "R") ||
        !expect_equal(out_g, want_g, 4U, "G") ||
        !expect_equal(out_b, want_b, 4U, "B") ||
        !expect_equal(out_a, want_a, 4U, "A")) {
        return 1;
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;

    rc = psd89_write_path("tests/test_v13_roundtrip.psd", &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write_path returned %s\n", psd89_error_string(rc));
        return 1;
    }

    rc = psd89_read_path(&parsed, "tests/test_v13_roundtrip.psd");
    if (rc != PSD89_OK) {
        fprintf(stderr, "read_path returned %s\n", psd89_error_string(rc));
        return 1;
    }
    if (!parsed.layers[1].user_mask.present ||
        parsed.layers[1].user_mask.left != 0 || parsed.layers[1].user_mask.top != 0 ||
        parsed.layers[1].user_mask.right != 2 || parsed.layers[1].user_mask.bottom != 2 ||
        parsed.layers[1].user_mask.default_color != 255U) {
        fprintf(stderr, "parsed user mask metadata mismatch\n");
        return 1;
    }

    fp = fopen("tests/test_v13_roundtrip.psd", "rb");
    if (fp == 0) {
        fprintf(stderr, "failed to reopen test_v13_roundtrip.psd\n");
        return 1;
    }
    psd89_stdio_make_io(fp, &io);
    decode_planes[0] = dec_r;
    decode_planes[1] = dec_g;
    decode_planes[2] = dec_b;
    decode_planes[3] = dec_a;
    rc = psd89_decode_composite_u8(&parsed, &io, decode_planes, 2U);
    if (rc != PSD89_OK) {
        fclose(fp);
        fprintf(stderr, "decode composite returned %s\n", psd89_error_string(rc));
        return 1;
    }
    rc = psd89_decode_layer_channel_u8(&parsed, &io, 1U, PSD89_CH_LAYER_MASK, dec_m, 2U);
    fclose(fp);
    if (rc != PSD89_OK) {
        fprintf(stderr, "decode layer mask returned %s\n", psd89_error_string(rc));
        return 1;
    }

    if (!expect_equal(dec_r, out_r, 4U, "roundtrip R") ||
        !expect_equal(dec_g, out_g, 4U, "roundtrip G") ||
        !expect_equal(dec_b, out_b, 4U, "roundtrip B") ||
        !expect_equal(dec_a, out_a, 4U, "roundtrip A") ||
        !expect_equal(dec_m, red_m, 4U, "roundtrip M")) {
        return 1;
    }

    printf("test_v13: ok\n");
    return 0;
}
