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

static void init_one_pixel_doc(psd89_doc *doc, psd89_layer **layer_out)
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    static const unsigned char red_r[1] = { 255U };
    static const unsigned char red_g[1] = { 0U };
    static const unsigned char red_b[1] = { 0U };
    static const unsigned char red_a[1] = { 255U };

    psd89_doc_init(doc);
    doc->width = 1U;
    doc->height = 1U;
    doc->channels = 4U;
    doc->depth = 8U;
    doc->color_mode = PSD89_MODE_RGB;
    doc->merged_alpha_in_first_channel = 1;
    doc->layer_count = 1U;
    doc->composite_stride = 1U;
    doc->composite_write_compression = PSD89_COMP_RLE;
    doc->composite_planes[0] = red_r;
    doc->composite_planes[1] = red_g;
    doc->composite_planes[2] = red_b;
    doc->composite_planes[3] = red_a;

    init_rgb_layer_rect(&doc->layers[0], 0, 0, 1, 1, "px", norm, 255U, 0U,
                        red_r, red_g, red_b, red_a, PSD89_COMP_RLE, 1U);
    if (layer_out != 0) {
        *layer_out = &doc->layers[0];
    }
}

typedef struct bw_t {
    unsigned char *buf;
    psd89_u32 cap;
    psd89_u32 pos;
    int ok;
} bw_t;

static void bw_init(bw_t *bw, unsigned char *buf, psd89_u32 cap)
{
    bw->buf = buf;
    bw->cap = cap;
    bw->pos = 0U;
    bw->ok = 1;
}

static void bw_put_u8(bw_t *bw, unsigned char v)
{
    if (!bw->ok || bw->pos >= bw->cap) {
        bw->ok = 0;
        return;
    }
    bw->buf[bw->pos++] = v;
}

static void bw_put_be16(bw_t *bw, unsigned short v)
{
    bw_put_u8(bw, (unsigned char)((v >> 8) & 0xFFU));
    bw_put_u8(bw, (unsigned char)(v & 0xFFU));
}

static void bw_put_be32(bw_t *bw, psd89_u32 v)
{
    bw_put_u8(bw, (unsigned char)((v >> 24) & 0xFFU));
    bw_put_u8(bw, (unsigned char)((v >> 16) & 0xFFU));
    bw_put_u8(bw, (unsigned char)((v >> 8) & 0xFFU));
    bw_put_u8(bw, (unsigned char)(v & 0xFFU));
}

static void bw_put_bytes(bw_t *bw, const void *src, psd89_u32 n)
{
    const unsigned char *p;
    psd89_u32 i;
    p = (const unsigned char *)src;
    for (i = 0U; i < n; ++i) {
        bw_put_u8(bw, p[i]);
    }
}

static void bw_put_unicode_ascii(bw_t *bw, const char *s)
{
    psd89_u32 i;
    psd89_u32 n;
    if (s == 0) {
        s = "";
    }
    n = 0U;
    while (s[n] != '\0') {
        ++n;
    }
    bw_put_be32(bw, (psd89_u32)n);
    for (i = 0U; i < n; ++i) {
        bw_put_be16(bw, (unsigned short)(unsigned char)s[i]);
    }
    bw_put_be16(bw, 0U);
}

static void bw_put_desc_id_fourcc(bw_t *bw, const char id4[4])
{
    bw_put_be32(bw, 0U);
    bw_put_bytes(bw, id4, 4U);
}

static psd89_u32 build_deep_descriptor(unsigned char *dst, psd89_u32 cap)
{
    static const char k_bool[4] = { 'f', 'l', 'a', 'g' };
    static const char k_list[4] = { 'l', 'i', 's', 't' };
    static const char k_obj1[4] = { 'o', 'b', 'j', '1' };
    static const char k_ref1[4] = { 'r', 'e', 'f', '1' };
    static const char k_unit[4] = { 'u', 'n', 'i', 't' };
    static const char k_data[4] = { 'd', 'a', 't', 'a' };
    static const char k_enum[4] = { 'e', 'n', 'u', 'm' };
    static const char k_comp[4] = { 'c', 'o', 'm', 'p' };
    static const char type_bool[4] = { 'b', 'o', 'o', 'l' };
    static const char type_vlls[4] = { 'V', 'l', 'L', 's' };
    static const char type_long[4] = { 'l', 'o', 'n', 'g' };
    static const char type_text[4] = { 'T', 'E', 'X', 'T' };
    static const char type_objc[4] = { 'O', 'b', 'j', 'c' };
    static const char type_objref[4] = { 'o', 'b', 'j', ' ' };
    static const char type_untf[4] = { 'U', 'n', 't', 'F' };
    static const char type_tdta[4] = { 't', 'd', 't', 'a' };
    static const char type_enum[4] = { 'e', 'n', 'u', 'm' };
    static const char type_comp[4] = { 'c', 'o', 'm', 'p' };
    static const char class_null[4] = { 'n', 'u', 'l', 'l' };
    static const char nested_class[4] = { 'C', 'l', 'a', 's' };
    static const char nested_key[4] = { 't', 'x', 't', '1' };
    static const char ref_type[4] = { 'i', 'n', 'd', 'x' };
    static const char ref_class[4] = { 'p', 'r', 'o', 'p' };
    static const char ordn[4] = { 'O', 'r', 'd', 'n' };
    static const char trgt[4] = { 'T', 'r', 'g', 't' };
    static const char units[4] = { 'P', 'x', 'l', ' ' };
    unsigned char raw3[3];
    bw_t bw;

    raw3[0] = 1U;
    raw3[1] = 2U;
    raw3[2] = 3U;

    bw_init(&bw, dst, cap);
    bw_put_unicode_ascii(&bw, "root");
    bw_put_desc_id_fourcc(&bw, class_null);
    bw_put_be32(&bw, 8U);

    bw_put_desc_id_fourcc(&bw, k_bool);
    bw_put_bytes(&bw, type_bool, 4U);
    bw_put_u8(&bw, 1U);

    bw_put_desc_id_fourcc(&bw, k_list);
    bw_put_bytes(&bw, type_vlls, 4U);
    bw_put_be32(&bw, 2U);
    bw_put_bytes(&bw, type_long, 4U);
    bw_put_be32(&bw, 5U);
    bw_put_bytes(&bw, type_text, 4U);
    bw_put_unicode_ascii(&bw, "hi");

    bw_put_desc_id_fourcc(&bw, k_obj1);
    bw_put_bytes(&bw, type_objc, 4U);
    bw_put_unicode_ascii(&bw, "nest");
    bw_put_desc_id_fourcc(&bw, nested_class);
    bw_put_be32(&bw, 1U);
    bw_put_desc_id_fourcc(&bw, nested_key);
    bw_put_bytes(&bw, type_text, 4U);
    bw_put_unicode_ascii(&bw, "deep");

    bw_put_desc_id_fourcc(&bw, k_ref1);
    bw_put_bytes(&bw, type_objref, 4U);
    bw_put_be32(&bw, 1U);
    bw_put_bytes(&bw, ref_type, 4U);
    bw_put_unicode_ascii(&bw, "");
    bw_put_desc_id_fourcc(&bw, ref_class);
    bw_put_be32(&bw, 7U);

    bw_put_desc_id_fourcc(&bw, k_unit);
    bw_put_bytes(&bw, type_untf, 4U);
    bw_put_bytes(&bw, units, 4U);
    bw_put_be32(&bw, 0x40290000U);
    bw_put_be32(&bw, 0x00000000U);

    bw_put_desc_id_fourcc(&bw, k_data);
    bw_put_bytes(&bw, type_tdta, 4U);
    bw_put_be32(&bw, 3U);
    bw_put_bytes(&bw, raw3, 3U);

    bw_put_desc_id_fourcc(&bw, k_enum);
    bw_put_bytes(&bw, type_enum, 4U);
    bw_put_desc_id_fourcc(&bw, ordn);
    bw_put_desc_id_fourcc(&bw, trgt);

    bw_put_desc_id_fourcc(&bw, k_comp);
    bw_put_bytes(&bw, type_comp, 4U);
    bw_put_be32(&bw, 0U);
    bw_put_be32(&bw, 9U);

    return bw.ok ? bw.pos : 0U;
}

static int roundtrip_doc(const psd89_doc *doc, psd89_doc *parsed, unsigned char *buf, psd89_u32 cap)
{
    psd89_memio m;
    psd89_io io;
    int rc;

    psd89_memio_init_write(&m, buf, cap);
    psd89_memio_make_io(&m, &io);
    rc = psd89_write(&io, doc);
    if (rc != PSD89_OK) {
        fprintf(stderr, "write returned %s\n", psd89_error_string(rc));
        return 0;
    }
    psd89_doc_init(parsed);
    psd89_memio_init_read(&m, buf, m.size);
    psd89_memio_make_io(&m, &io);
    rc = psd89_read(parsed, &io);
    if (rc != PSD89_OK) {
        fprintf(stderr, "read returned %s\n", psd89_error_string(rc));
        return 0;
    }
    return 1;
}

static const psd89_descriptor_item *find_item(const psd89_descriptor_summary *summary, const char key[4])
{
    psd89_u16 i;
    if (summary == 0) {
        return 0;
    }
    for (i = 0U; i < summary->parsed_item_count; ++i) {
        if (summary->items[i].key.is_fourcc && memcmp(summary->items[i].key.fourcc, key, 4U) == 0) {
            return &summary->items[i];
        }
    }
    return 0;
}

static int test_lrfx_classic_roundtrip(void)
{
    unsigned char buf[65536];
    psd89_doc doc;
    psd89_doc parsed;
    psd89_layer *layer;
    const psd89_layer *pl;

    init_one_pixel_doc(&doc, &layer);
    memset(&layer->lrfx, 0, sizeof(layer->lrfx));
    layer->lrfx.present = 1U;
    layer->lrfx.version = 0U;
    layer->lrfx.common_state_present = 1U;
    layer->lrfx.common_visible = 1U;

    layer->lrfx.drop_shadow.present = 1U;
    layer->lrfx.drop_shadow.enabled = 1U;
    layer->lrfx.drop_shadow.use_global_angle = 1U;
    layer->lrfx.drop_shadow.native_color_present = 1U;
    layer->lrfx.drop_shadow.version = 2U;
    layer->lrfx.drop_shadow.blur = 5;
    layer->lrfx.drop_shadow.intensity = 60;
    layer->lrfx.drop_shadow.angle = 120;
    layer->lrfx.drop_shadow.distance = 4;
    memcpy(layer->lrfx.drop_shadow.blend_mode, "mul ", 4U);
    layer->lrfx.drop_shadow.opacity = 75U;
    layer->lrfx.drop_shadow.color[1] = 0U;
    layer->lrfx.drop_shadow.color[2] = 0U;
    layer->lrfx.drop_shadow.color[3] = 0U;
    layer->lrfx.drop_shadow.native_color[1] = 0U;
    layer->lrfx.drop_shadow.native_color[2] = 0U;
    layer->lrfx.drop_shadow.native_color[3] = 0U;

    layer->lrfx.inner_shadow.present = 1U;
    layer->lrfx.inner_shadow.enabled = 1U;
    layer->lrfx.inner_shadow.version = 2U;
    layer->lrfx.inner_shadow.blur = 2;
    layer->lrfx.inner_shadow.intensity = 30;
    layer->lrfx.inner_shadow.angle = -45;
    layer->lrfx.inner_shadow.distance = 1;
    memcpy(layer->lrfx.inner_shadow.blend_mode, "mul ", 4U);
    layer->lrfx.inner_shadow.opacity = 64U;
    layer->lrfx.inner_shadow.native_color_present = 1U;

    layer->lrfx.outer_glow.present = 1U;
    layer->lrfx.outer_glow.enabled = 1U;
    layer->lrfx.outer_glow.version = 2U;
    layer->lrfx.outer_glow.blur = 7;
    layer->lrfx.outer_glow.intensity = 55;
    memcpy(layer->lrfx.outer_glow.blend_mode, "scrn", 4U);
    layer->lrfx.outer_glow.opacity = 80U;
    layer->lrfx.outer_glow.color[1] = 65535U;
    layer->lrfx.outer_glow.color[2] = 65535U;
    layer->lrfx.outer_glow.color[3] = 0U;
    layer->lrfx.outer_glow.native_color_present = 1U;
    layer->lrfx.outer_glow.native_color[1] = 65535U;
    layer->lrfx.outer_glow.native_color[2] = 65535U;
    layer->lrfx.outer_glow.native_color[3] = 0U;

    layer->lrfx.inner_glow.present = 1U;
    layer->lrfx.inner_glow.enabled = 1U;
    layer->lrfx.inner_glow.version = 2U;
    layer->lrfx.inner_glow.blur = 4;
    layer->lrfx.inner_glow.intensity = 50;
    memcpy(layer->lrfx.inner_glow.blend_mode, "scrn", 4U);
    layer->lrfx.inner_glow.opacity = 81U;
    layer->lrfx.inner_glow.invert = 1U;
    layer->lrfx.inner_glow.color[1] = 0U;
    layer->lrfx.inner_glow.color[2] = 65535U;
    layer->lrfx.inner_glow.color[3] = 65535U;
    layer->lrfx.inner_glow.native_color_present = 1U;

    layer->lrfx.bevel.present = 1U;
    layer->lrfx.bevel.enabled = 1U;
    layer->lrfx.bevel.version = 2U;
    layer->lrfx.bevel.angle = 90;
    layer->lrfx.bevel.strength = 6;
    layer->lrfx.bevel.blur = 3;
    memcpy(layer->lrfx.bevel.highlight_blend_mode, "scrn", 4U);
    memcpy(layer->lrfx.bevel.shadow_blend_mode, "mul ", 4U);
    layer->lrfx.bevel.highlight_color[1] = 65535U;
    layer->lrfx.bevel.highlight_color[2] = 65535U;
    layer->lrfx.bevel.highlight_color[3] = 65535U;
    layer->lrfx.bevel.shadow_color[1] = 0U;
    layer->lrfx.bevel.shadow_color[2] = 0U;
    layer->lrfx.bevel.shadow_color[3] = 0U;
    layer->lrfx.bevel.bevel_style = 2U;
    layer->lrfx.bevel.highlight_opacity = 70U;
    layer->lrfx.bevel.shadow_opacity = 65U;
    layer->lrfx.bevel.use_global_angle = 1U;
    layer->lrfx.bevel.up = 1U;
    layer->lrfx.bevel.real_colors_present = 1U;

    layer->lrfx.solid_fill.present = 1U;
    layer->lrfx.solid_fill.enabled = 1U;
    layer->lrfx.solid_fill.opacity = 255U;
    layer->lrfx.solid_fill.color[3] = 65535U;
    layer->lrfx.solid_fill.native_color[3] = 65535U;

    if (!roundtrip_doc(&doc, &parsed, buf, sizeof(buf))) {
        return 0;
    }
    pl = &parsed.layers[0];
    if (!pl->lrfx.present || !pl->lrfx.drop_shadow.present || !pl->lrfx.inner_shadow.present ||
        !pl->lrfx.outer_glow.present || !pl->lrfx.inner_glow.present || !pl->lrfx.bevel.present ||
        !pl->lrfx.solid_fill.present) {
        fprintf(stderr, "classic lrFX families not preserved\n");
        return 0;
    }
    if (pl->lrfx.drop_shadow.blur != 5 || pl->lrfx.drop_shadow.distance != 4 || pl->lrfx.drop_shadow.opacity != 75U) {
        fprintf(stderr, "drop shadow fields lost\n");
        return 0;
    }
    if (pl->lrfx.inner_glow.invert != 1U || pl->lrfx.outer_glow.blur != 7 || pl->lrfx.bevel.bevel_style != 2U) {
        fprintf(stderr, "glow/bevel fields lost\n");
        return 0;
    }
    return 1;
}

static int test_descriptor_depth_and_txt2_roundtrip(void)
{
    unsigned char buf[131072];
    unsigned char desc_buf[4096];
    unsigned char txt2_buf[6];
    psd89_u32 desc_len;
    psd89_doc doc;
    psd89_doc parsed;
    psd89_layer *layer;
    const psd89_descriptor_item *item;
    static const char k_list[4] = { 'l', 'i', 's', 't' };
    static const char k_obj1[4] = { 'o', 'b', 'j', '1' };
    static const char k_ref1[4] = { 'r', 'e', 'f', '1' };
    static const char k_unit[4] = { 'u', 'n', 'i', 't' };
    static const char k_data[4] = { 'd', 'a', 't', 'a' };
    static const char k_enum[4] = { 'e', 'n', 'u', 'm' };
    static const char k_comp[4] = { 'c', 'o', 'm', 'p' };

    txt2_buf[0] = 'E'; txt2_buf[1] = 'n'; txt2_buf[2] = 'g'; txt2_buf[3] = 'i'; txt2_buf[4] = 'n'; txt2_buf[5] = 'e';
    desc_len = build_deep_descriptor(desc_buf, sizeof(desc_buf));
    if (desc_len == 0U) {
        fprintf(stderr, "descriptor builder failed\n");
        return 0;
    }

    init_one_pixel_doc(&doc, &layer);
    layer->type_tool.present = 1U;
    layer->type_tool.version = 1U;
    layer->type_tool.transform[0] = PSD89_FX16_ONE;
    layer->type_tool.transform[3] = PSD89_FX16_ONE;
    layer->type_tool.text_version = 50U;
    layer->type_tool.text_descriptor_version = 16U;
    layer->type_tool.text.authored_raw = desc_buf;
    layer->type_tool.text.authored_raw_size = (psd89_u32)desc_len;
    layer->type_tool.warp_version = 1U;
    layer->type_tool.warp_descriptor_version = 16U;
    layer->type_tool.warp.authored_raw = desc_buf;
    layer->type_tool.warp.authored_raw_size = (psd89_u32)desc_len;
    layer->type_tool.bounds[2] = PSD89_FX16_ONE;
    layer->type_tool.bounds[3] = PSD89_FX16_ONE;

    layer->text_engine.present = 1U;
    layer->text_engine.authored_raw = txt2_buf;
    layer->text_engine.authored_raw_size = 6U;

    layer->object_effects.present = 1U;
    layer->object_effects.object_version = 0U;
    layer->object_effects.descriptor_version = 16U;
    layer->object_effects.descriptor.authored_raw = desc_buf;
    layer->object_effects.descriptor.authored_raw_size = (psd89_u32)desc_len;

    layer->smart_object.present = 1U;
    memcpy(layer->smart_object.tag_key, "SoLd", 4U);
    memcpy(layer->smart_object.type, "soLD", 4U);
    layer->smart_object.version = 4U;
    layer->smart_object.descriptor_version = 16U;
    layer->smart_object.descriptor.authored_raw = desc_buf;
    layer->smart_object.descriptor.authored_raw_size = (psd89_u32)desc_len;

    if (!roundtrip_doc(&doc, &parsed, buf, sizeof(buf))) {
        return 0;
    }
    layer = &parsed.layers[0];
    if (!layer->type_tool.present || !layer->type_tool.text.summary.parsed || !layer->object_effects.descriptor.summary.parsed) {
        fprintf(stderr, "deep descriptor parse missing\n");
        return 0;
    }
    if (!layer->text_engine.present || layer->text_engine.raw_size != 6U) {
        fprintf(stderr, "Txt2 missing after roundtrip\n");
        return 0;
    }
    if (layer->type_tool.text.summary.parsed_list_count == 0U ||
        layer->type_tool.text.summary.parsed_nested_count == 0U ||
        layer->type_tool.text.summary.parsed_ref_count == 0U) {
        fprintf(stderr, "descriptor depth counters missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_list);
    if (item == 0 || !item->list_count_valid || item->list_count != 2U) {
        fprintf(stderr, "list item parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_obj1);
    if (item == 0 || !item->nested_valid || item->nested_item_count != 1U) {
        fprintf(stderr, "nested object parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_ref1);
    if (item == 0 || !item->ref_count_valid || item->ref_count != 1U) {
        fprintf(stderr, "reference parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_unit);
    if (item == 0 || !item->unit_valid || memcmp(item->unit, "Pxl ", 4U) != 0 || !item->fx_valid) {
        fprintf(stderr, "unit scalar parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_data);
    if (item == 0 || !item->length_valid || item->data_length != 3U) {
        fprintf(stderr, "raw data length parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_enum);
    if (item == 0 || !item->text_valid || !item->aux_text_valid) {
        fprintf(stderr, "enum parse missing\n");
        return 0;
    }
    item = find_item(&layer->type_tool.text.summary, k_comp);
    if (item == 0 || !item->comp_valid || item->comp_lo != 9U) {
        fprintf(stderr, "comp parse missing\n");
        return 0;
    }
    if (!layer->smart_object.present || memcmp(layer->smart_object.tag_key, "SoLd", 4U) != 0 || !layer->smart_object.descriptor.summary.parsed) {
        fprintf(stderr, "smart object deep descriptor missing\n");
        return 0;
    }
    return 1;
}

static int test_plld_roundtrip(void)
{
    unsigned char buf[131072];
    unsigned char desc_buf[4096];
    psd89_u32 desc_len;
    psd89_doc doc;
    psd89_doc parsed;
    psd89_layer *layer;

    desc_len = build_deep_descriptor(desc_buf, sizeof(desc_buf));
    if (desc_len == 0U) {
        fprintf(stderr, "descriptor builder failed for plLd\n");
        return 0;
    }

    init_one_pixel_doc(&doc, &layer);
    layer->smart_object.present = 1U;
    memcpy(layer->smart_object.tag_key, "plLd", 4U);
    memcpy(layer->smart_object.type, "plcL", 4U);
    layer->smart_object.version = 3U;
    layer->smart_object.placed_info_present = 1U;
    strcpy(layer->smart_object.unique_id, "placed-id");
    layer->smart_object.page_number = 2U;
    layer->smart_object.total_pages = 5U;
    layer->smart_object.anti_alias_policy = 1U;
    layer->smart_object.placed_layer_type = 2U;
    layer->smart_object.transform[0] = 0;
    layer->smart_object.transform[1] = 0;
    layer->smart_object.transform[2] = PSD89_FX16_ONE;
    layer->smart_object.transform[3] = 0;
    layer->smart_object.transform[4] = PSD89_FX16_ONE;
    layer->smart_object.transform[5] = PSD89_FX16_ONE;
    layer->smart_object.transform[6] = 0;
    layer->smart_object.transform[7] = PSD89_FX16_ONE;
    layer->smart_object.warp_version = 0U;
    layer->smart_object.warp_descriptor_version = 16U;
    layer->smart_object.warp_descriptor.authored_raw = desc_buf;
    layer->smart_object.warp_descriptor.authored_raw_size = (psd89_u32)desc_len;

    if (!roundtrip_doc(&doc, &parsed, buf, sizeof(buf))) {
        return 0;
    }
    layer = &parsed.layers[0];
    if (!layer->smart_object.present || memcmp(layer->smart_object.tag_key, "plLd", 4U) != 0 || !layer->smart_object.placed_info_present) {
        fprintf(stderr, "plLd missing after roundtrip\n");
        return 0;
    }
    if (strcmp(layer->smart_object.unique_id, "placed-id") != 0 ||
        layer->smart_object.page_number != 2U ||
        layer->smart_object.total_pages != 5U ||
        layer->smart_object.placed_layer_type != 2U) {
        fprintf(stderr, "plLd fields lost\n");
        return 0;
    }
    if (!layer->smart_object.warp_descriptor.summary.parsed || layer->smart_object.warp_descriptor.summary.parsed_nested_count == 0U) {
        fprintf(stderr, "plLd warp descriptor not parsed\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_lrfx_classic_roundtrip()) {
        return 1;
    }
    if (!test_descriptor_depth_and_txt2_roundtrip()) {
        return 1;
    }
    if (!test_plld_roundtrip()) {
        return 1;
    }
    printf("test_v21: ok\n");
    return 0;
}
