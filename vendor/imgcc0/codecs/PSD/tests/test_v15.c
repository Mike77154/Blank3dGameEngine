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

static psd89_fx24 fx24_frac(psd89_s32 num, psd89_s32 den)
{
    return den == 0 ? 0 : (psd89_fx24)(((psd89_s32)num << 24) / den);
}

static void set_vector_knot(psd89_vector_knot *knot, psd89_fx24 x, psd89_fx24 y)
{
    memset(knot, 0, sizeof(*knot));
    knot->linked = 1U;
    knot->preceding_h = x;
    knot->anchor_h = x;
    knot->leaving_h = x;
    knot->preceding_v = y;
    knot->anchor_v = y;
    knot->leaving_v = y;
}

static void set_vector_rect(psd89_layer *layer,
                            psd89_u32 doc_width,
                            psd89_u32 doc_height,
                            psd89_s32 left,
                            psd89_s32 top,
                            psd89_s32 right,
                            psd89_s32 bottom,
                            psd89_u8 use_vsms)
{
    psd89_vector_mask *vm;
    vm = &layer->vector_mask;
    memset(vm, 0, sizeof(*vm));
    vm->present = 1U;
    vm->use_vsms = use_vsms;
    vm->version = 3U;
    vm->path_fill_rule_present = 1U;
    vm->initial_fill_rule_present = 1U;
    vm->initial_fill_rule = 0U;
    vm->subpath_count = 1U;
    vm->knot_count = 4U;
    vm->subpaths[0].closed = 1U;
    vm->subpaths[0].first_knot = 0U;
    vm->subpaths[0].knot_count = 4U;

    set_vector_knot(&vm->knots[0], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[1], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(top, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[2], fx24_frac(right, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
    set_vector_knot(&vm->knots[3], fx24_frac(left, (psd89_s32)doc_width), fx24_frac(bottom, (psd89_s32)doc_height));
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

static int run_case(const char *path, psd89_u16 comp, psd89_u8 use_vsms)
{
    unsigned char back_r[8] = { 5U, 15U, 25U, 35U, 45U, 55U, 65U, 75U };
    unsigned char back_g[8] = { 10U, 20U, 30U, 40U, 50U, 60U, 70U, 80U };
    unsigned char back_b[8] = { 180U, 181U, 182U, 183U, 184U, 185U, 186U, 187U };
    unsigned char back_a[8] = { 255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U };

    unsigned char fore_r[8] = { 200U, 150U, 100U,  50U, 210U, 160U, 110U,  60U };
    unsigned char fore_g[8] = {   1U,   2U,   3U,   4U,   5U,   6U,   7U,   8U };
    unsigned char fore_b[8] = {  90U,  80U,  70U,  60U,  50U,  40U,  30U,  20U };
    unsigned char fore_a[8] = { 255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U };

    unsigned char expect_r[8];
    unsigned char expect_g[8];
    unsigned char expect_b[8];
    unsigned char expect_a[8];
    unsigned char out_r[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_g[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_b[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char out_a[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_r[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_g[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_b[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char scratch_a[PSD89_MAX_COMPOSE_ROW_BYTES];
    unsigned char dec_r[8];
    unsigned char dec_g[8];
    unsigned char dec_b[8];
    unsigned char dec_a[8];
    unsigned char layer_r[8];
    unsigned char *dst_color[3];
    unsigned char *scratch_color[3];
    unsigned char *decode_planes[4];
    psd89_compose_options opt;
    psd89_doc doc;
    psd89_doc parsed;
    psd89_io io;
    FILE *fp;
    char norm[4] = { 'n', 'o', 'r', 'm' };
    psd89_u32 i;
    int rc;

    for (i = 0U; i < 8U; ++i) {
        if ((i % 4U) < 2U) {
            expect_r[i] = fore_r[i];
            expect_g[i] = fore_g[i];
            expect_b[i] = fore_b[i];
            expect_a[i] = 255U;
        } else {
            expect_r[i] = back_r[i];
            expect_g[i] = back_g[i];
            expect_b[i] = back_b[i];
            expect_a[i] = 255U;
        }
    }

    psd89_doc_init(&doc);
    doc.width = 4U;
    doc.height = 2U;
    doc.channels = 4U;
    doc.depth = 8U;
    doc.color_mode = PSD89_MODE_RGB;
    doc.merged_alpha_in_first_channel = 1;
    doc.composite_stride = 4U;
    doc.composite_write_compression = comp;
    doc.layer_count = 2U;

    init_rgb_layer_rect(&doc.layers[0], 0, 0, 2, 4, "back", norm, 255U, 0U,
                        back_r, back_g, back_b, back_a, comp, 4U);
    init_rgb_layer_rect(&doc.layers[1], 0, 0, 2, 4, "fore-vector", norm, 255U, 0U,
                        fore_r, fore_g, fore_b, fore_a, comp, 4U);
    set_vector_rect(&doc.layers[1], 4U, 2U, 0, 0, 2, 2, use_vsms);

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
                                 4U,
                                 scratch_color,
                                 scratch_a,
                                 PSD89_MAX_COMPOSE_ROW_BYTES,
                                 &opt);
    if (rc != PSD89_OK) {
        fprintf(stderr, "compose failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }
    if (!expect_equal(out_r, expect_r, 8U, "compose R") ||
        !expect_equal(out_g, expect_g, 8U, "compose G") ||
        !expect_equal(out_b, expect_b, 8U, "compose B") ||
        !expect_equal(out_a, expect_a, 8U, "compose A")) {
        return 0;
    }

    doc.composite_planes[0] = out_r;
    doc.composite_planes[1] = out_g;
    doc.composite_planes[2] = out_b;
    doc.composite_planes[3] = out_a;

    rc = psd89_write_path(path, &doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }

    psd89_doc_init(&parsed);
    rc = psd89_read_path(&parsed, path);
    if (rc != PSD89_OK) {
        fprintf(stderr, "read failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }
    if (parsed.layer_count != 2U || parsed.composite_compression != comp ||
        !parsed.layers[1].vector_mask.present ||
        parsed.layers[1].vector_mask.subpath_count != 1U ||
        parsed.layers[1].vector_mask.knot_count != 4U ||
        parsed.layers[1].vector_mask.use_vsms != use_vsms) {
        fprintf(stderr, "parsed vector metadata mismatch for %s\n", path);
        return 0;
    }

    fp = fopen(path, "rb");
    if (fp == 0) {
        fprintf(stderr, "reopen failed for %s\n", path);
        return 0;
    }
    psd89_stdio_make_io(fp, &io);
    decode_planes[0] = dec_r;
    decode_planes[1] = dec_g;
    decode_planes[2] = dec_b;
    decode_planes[3] = dec_a;
    rc = psd89_decode_composite_u8(&parsed, &io, decode_planes, 4U);
    if (rc != PSD89_OK) {
        fclose(fp);
        fprintf(stderr, "decode composite failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }
    rc = psd89_decode_layer_channel_u8(&parsed, &io, 1U, PSD89_CH_RED, layer_r, 4U);
    fclose(fp);
    if (rc != PSD89_OK) {
        fprintf(stderr, "decode layer red failed for %s: %s\n", path, psd89_error_string(rc));
        return 0;
    }

    if (!expect_equal(dec_r, expect_r, 8U, "decode R") ||
        !expect_equal(dec_g, expect_g, 8U, "decode G") ||
        !expect_equal(dec_b, expect_b, 8U, "decode B") ||
        !expect_equal(dec_a, expect_a, 8U, "decode A") ||
        !expect_equal(layer_r, fore_r, 8U, "layer red")) {
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!run_case("tests/test_v15_zip.psd", PSD89_COMP_ZIP, 0U)) {
        return 1;
    }
    if (!run_case("tests/test_v15_zip_pred.psd", PSD89_COMP_ZIP_PRED, 1U)) {
        return 1;
    }
    printf("test_v15: ok\n");
    return 0;
}
