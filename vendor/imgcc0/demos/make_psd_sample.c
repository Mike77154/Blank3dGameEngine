#include "psd89/psd89.h"
#include "psd89/psd89_stdio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_rgb_layer(psd89_layer *layer,
                           const char *name,
                           const char blend_mode[4],
                           psd89_u8 opacity,
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
    layer->channel_count = 4U;
    memcpy(layer->blend_mode, blend_mode, 4U);
    layer->opacity = opacity;
    layer->flags = 0U;
    layer->clipping = 0U;
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

    layer->channels[3].id = PSD89_CH_ALPHA;
    layer->channels[3].compression = PSD89_COMP_RLE;
    layer->channels[3].plane = a;
    layer->channels[3].stride = stride;
}

int main(int argc, char **argv)
{
    const psd89_u32 w = 96U;
    const psd89_u32 h = 96U;
    const psd89_u32 stride = 96U;
    const char norm[4] = { 'n', 'o', 'r', 'm' };
    psd89_doc doc;
    unsigned char *r;
    unsigned char *g;
    unsigned char *b;
    unsigned char *a;
    unsigned long n;
    unsigned long y;
    const char *path;
    int rc;

    path = (argc > 1) ? argv[1] : "sample.psd";
    n = (unsigned long)w * (unsigned long)h;
    r = (unsigned char*)malloc(n);
    g = (unsigned char*)malloc(n);
    b = (unsigned char*)malloc(n);
    a = (unsigned char*)malloc(n);
    if (r == 0 || g == 0 || b == 0 || a == 0) {
        fprintf(stderr, "alloc failed\n");
        free(r); free(g); free(b); free(a);
        return 1;
    }

    for (y = 0UL; y < (unsigned long)h; ++y) {
        unsigned long x;
        for (x = 0UL; x < (unsigned long)w; ++x) {
            unsigned long i = y * (unsigned long)w + x;
            unsigned char checker = (((x / 12UL) + (y / 12UL)) & 1UL) ? 30U : 220U;
            unsigned char inside = (x > 18UL && x < 78UL && y > 18UL && y < 78UL) ? 1U : 0U;
            r[i] = (unsigned char)((x * 255UL) / (w - 1U));
            g[i] = (unsigned char)((y * 255UL) / (h - 1U));
            b[i] = (unsigned char)(checker);
            a[i] = inside ? 255U : 180U;
            if ((x > 28UL && x < 68UL) && (y > 28UL && y < 68UL)) {
                r[i] = 255U;
                g[i] = (unsigned char)(180U - ((y - 28UL) * 4UL));
                b[i] = (unsigned char)(40U + ((x - 28UL) * 3UL));
                a[i] = 255U;
            }
        }
    }

    psd89_doc_init(&doc);
    doc.width = w;
    doc.height = h;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1U;
    doc.composite_stride = stride;
    doc.composite_write_compression = PSD89_COMP_RLE;
    doc.layer_count = 1U;
    doc.composite_planes[0] = r;
    doc.composite_planes[1] = g;
    doc.composite_planes[2] = b;
    doc.composite_planes[3] = a;
    init_rgb_layer(&doc.layers[0], "sample-layer", norm, 255U, r, g, b, a, stride, (psd89_s32)w, (psd89_s32)h);

    rc = psd89_write_path(path, &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "psd write failed: %s\n", psd89_error_string(rc));
        free(r); free(g); free(b); free(a);
        return 1;
    }

    printf("wrote %s\n", path);
    free(r); free(g); free(b); free(a);
    return 0;
}
