#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <string.h>

static void init_rgb_layer(psd89_layer *layer,
                           const char *name,
                           const char blend_mode[4],
                           psd89_u8 opacity,
                           psd89_u8 flags,
                           psd89_u8 clipping,
                           const unsigned char *r,
                           const unsigned char *g,
                           const unsigned char *b,
                           const unsigned char *a,
                           psd89_u32 stride,
                           psd89_s32 width,
                           psd89_s32 height)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = 0;
    layer->left = 0;
    layer->bottom = height;
    layer->right = width;
    layer->channel_count = (psd89_u16)(a != 0 ? 4U : 3U);
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = opacity;
    layer->flags = flags;
    layer->clipping = clipping;
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
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

    if (a != 0) {
        layer->channels[3].id = PSD89_CH_ALPHA;
        layer->channels[3].compression = PSD89_COMP_RLE;
        layer->channels[3].plane = a;
        layer->channels[3].stride = stride;
    }
}

int main(int argc, char **argv)
{
    unsigned char base_r[4] = { 220, 220, 220, 220 };
    unsigned char base_g[4] = {   0,   0,   0,   0 };
    unsigned char base_b[4] = {   0,   0,   0,   0 };
    unsigned char base_a[4] = { 255,   0, 128, 255 };

    unsigned char clip_r[4] = {   0,   0,   0,   0 };
    unsigned char clip_g[4] = { 255, 255, 255, 255 };
    unsigned char clip_b[4] = {   0,   0,   0,   0 };
    unsigned char clip_a[4] = { 255, 255, 255, 255 };

    unsigned char comp_r[4];
    unsigned char comp_g[4];
    unsigned char comp_b[4];
    unsigned char comp_a[4];
    unsigned char dec_r[4];
    unsigned char dec_g[4];
    unsigned char dec_b[4];
    unsigned char dec_a[4];
    unsigned char scratch_r[4];
    unsigned char scratch_g[4];
    unsigned char scratch_b[4];
    unsigned char scratch_a[4];
    unsigned char *compose_color[3];
    unsigned char *scratch_color[3];
    unsigned char *decode_planes[4];
    psd89_doc doc;
    psd89_doc parsed;
    psd89_compose_options opt;
    psd89_io io_r;
    const char *path;
    int rc;
    FILE *fp;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    path = argc > 1 ? argv[1] : "example_file_roundtrip.psd";

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

    init_rgb_layer(&doc.layers[0], "base-shape", norm, 255U, 0U, 0U,
                   base_r, base_g, base_b, base_a, 2U, 2, 2);
    init_rgb_layer(&doc.layers[1], "green-clip", norm, 255U, 0U, 1U,
                   clip_r, clip_g, clip_b, clip_a, 2U, 2, 2);
    doc.layers[0].blend_clipped_present = 1U;
    doc.layers[0].blend_clipped = 1U;

    compose_color[0] = comp_r;
    compose_color[1] = comp_g;
    compose_color[2] = comp_b;
    scratch_color[0] = scratch_r;
    scratch_color[1] = scratch_g;
    scratch_color[2] = scratch_b;

    psd89_compose_options_init(&opt);
    rc = psd89_compose_layers_u8(&doc,
                                 0,
                                 compose_color,
                                 comp_a,
                                 2U,
                                 scratch_color,
                                 scratch_a,
                                 2U,
                                 &opt);
    if (rc != PSD89_OK) {
        fprintf(stderr, "compose failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    doc.composite_planes[0] = comp_r;
    doc.composite_planes[1] = comp_g;
    doc.composite_planes[2] = comp_b;
    doc.composite_planes[3] = comp_a;

    rc = psd89_write_path(path, &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write_path failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    rc = psd89_read_path(&parsed, path);
    if (rc != PSD89_OK) {
        fprintf(stderr, "read_path failed: %s\n", psd89_error_string(rc));
        return 1;
    }
    if (!parsed.merged_alpha_in_first_channel || parsed.layer_count != 2U ||
        parsed.layers[1].clipping != 1U ||
        !parsed.layers[0].blend_clipped_present || !parsed.layers[0].blend_clipped) {
        fprintf(stderr, "parsed metadata mismatch\n");
        return 1;
    }

    fp = fopen(path, "rb");
    if (fp == 0) {
        fprintf(stderr, "failed to reopen %s\n", path);
        return 1;
    }
    psd89_stdio_make_io(fp, &io_r);
    decode_planes[0] = dec_r;
    decode_planes[1] = dec_g;
    decode_planes[2] = dec_b;
    decode_planes[3] = dec_a;
    rc = psd89_decode_composite_u8(&parsed, &io_r, decode_planes, 2U);
    fclose(fp);
    if (rc != PSD89_OK) {
        fprintf(stderr, "decode composite failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    if (memcmp(dec_r, comp_r, sizeof(comp_r)) != 0 ||
        memcmp(dec_g, comp_g, sizeof(comp_g)) != 0 ||
        memcmp(dec_b, comp_b, sizeof(comp_b)) != 0 ||
        memcmp(dec_a, comp_a, sizeof(comp_a)) != 0) {
        fprintf(stderr, "file roundtrip mismatch\n");
        return 1;
    }

    printf("wrote and verified %s\n", path);
    printf("pixels: ");
    printf("[%u,%u,%u,%u] [%u,%u,%u,%u] [%u,%u,%u,%u] [%u,%u,%u,%u]\n",
           (unsigned int)comp_r[0], (unsigned int)comp_g[0], (unsigned int)comp_b[0], (unsigned int)comp_a[0],
           (unsigned int)comp_r[1], (unsigned int)comp_g[1], (unsigned int)comp_b[1], (unsigned int)comp_a[1],
           (unsigned int)comp_r[2], (unsigned int)comp_g[2], (unsigned int)comp_b[2], (unsigned int)comp_a[2],
           (unsigned int)comp_r[3], (unsigned int)comp_g[3], (unsigned int)comp_b[3], (unsigned int)comp_a[3]);
    return 0;
}
