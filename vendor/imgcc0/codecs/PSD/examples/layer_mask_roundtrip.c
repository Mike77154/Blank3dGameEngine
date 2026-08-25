#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <string.h>

static void init_layer(psd89_layer *layer,
                       const char *name,
                       const char blend_mode[4],
                       const unsigned char *r,
                       const unsigned char *g,
                       const unsigned char *b,
                       const unsigned char *a,
                       const unsigned char *m,
                       psd89_u16 compression)
{
    memset(layer, 0, sizeof(*layer));
    layer->top = 0;
    layer->left = 0;
    layer->bottom = 2;
    layer->right = 2;
    layer->channel_count = (psd89_u16)(m != 0 ? 5U : 4U);
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = 255U;
    strncpy(layer->name, name, PSD89_MAX_NAME_CHARS);
    layer->name[PSD89_MAX_NAME_CHARS] = '\0';

    layer->channels[0].id = PSD89_CH_RED;
    layer->channels[0].compression = compression;
    layer->channels[0].plane = r;
    layer->channels[0].stride = 2U;
    layer->channels[1].id = PSD89_CH_GREEN;
    layer->channels[1].compression = compression;
    layer->channels[1].plane = g;
    layer->channels[1].stride = 2U;
    layer->channels[2].id = PSD89_CH_BLUE;
    layer->channels[2].compression = compression;
    layer->channels[2].plane = b;
    layer->channels[2].stride = 2U;
    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = compression;
    layer->channels[3].plane = a;
    layer->channels[3].stride = 2U;

    if (m != 0) {
        layer->channels[4].id = PSD89_CH_LAYER_MASK;
        layer->channels[4].compression = compression;
        layer->channels[4].plane = m;
        layer->channels[4].stride = 2U;
        layer->user_mask.present = 1U;
        layer->user_mask.top = 0;
        layer->user_mask.left = 0;
        layer->user_mask.bottom = 2;
        layer->user_mask.right = 2;
        layer->user_mask.default_color = 255U;
        layer->user_mask.flags = 0U;
    }
}

int main(int argc, char **argv)
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
    unsigned char out_r[4];
    unsigned char out_g[4];
    unsigned char out_b[4];
    unsigned char out_a[4];
    unsigned char scratch_r[4];
    unsigned char scratch_g[4];
    unsigned char scratch_b[4];
    unsigned char scratch_a[4];
    unsigned char *dst_color[3];
    unsigned char *scratch_color[3];
    psd89_doc doc;
    psd89_compose_options opt;
    const char *path;
    int rc;
    char norm[4] = { 'n', 'o', 'r', 'm' };

    path = argc > 1 ? argv[1] : "example_layer_mask_roundtrip.psd";

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

    init_layer(&doc.layers[0], "blue-base", norm,
               blue_r, blue_g, blue_b, blue_a, 0, PSD89_COMP_RAW);
    init_layer(&doc.layers[1], "red-masked", norm,
               red_r, red_g, red_b, red_a, red_m, PSD89_COMP_RLE);

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
        fprintf(stderr, "compose failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;

    rc = psd89_write_path(path, &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write_path failed: %s\n", psd89_error_string(rc));
        return 1;
    }

    printf("wrote %s\n", path);
    printf("pixels: [%u,%u,%u,%u] [%u,%u,%u,%u] [%u,%u,%u,%u] [%u,%u,%u,%u]\n",
           (unsigned int)out_r[0], (unsigned int)out_g[0], (unsigned int)out_b[0], (unsigned int)out_a[0],
           (unsigned int)out_r[1], (unsigned int)out_g[1], (unsigned int)out_b[1], (unsigned int)out_a[1],
           (unsigned int)out_r[2], (unsigned int)out_g[2], (unsigned int)out_b[2], (unsigned int)out_a[2],
           (unsigned int)out_r[3], (unsigned int)out_g[3], (unsigned int)out_b[3], (unsigned int)out_a[3]);
    return 0;
}
