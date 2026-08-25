#include "psd89_internal.h"

#include <string.h>

static void psd89_desc_id_init(psd89_descriptor_id *id)
{
    if (id == 0) {
        return;
    }
    memset(id, 0, sizeof(*id));
}

void psd89_descriptor_summary_init(psd89_descriptor_summary *summary)
{
    if (summary == 0) {
        return;
    }
    memset(summary, 0, sizeof(*summary));
}

static int psd89_desc_key_eq(const char key[4], const char *lit4)
{
    return key[0] == lit4[0] && key[1] == lit4[1] && key[2] == lit4[2] && key[3] == lit4[3];
}

static int psd89_rd_be8_raw(psd89_io *io, psd89_u8 out[8])
{
    return psd89_io_read(io, out, 8U);
}

static int psd89_skip_unicode_string(psd89_io *io)
{
    psd89_u32 count;
    psd89_u32 total_bytes;
    if (!psd89_rd_be32(io, &count)) {
        return 0;
    }
    total_bytes = (psd89_u32)count * 2U + 2U;
    return psd89_skip(io, total_bytes);
}

static int psd89_read_unicode_ascii(psd89_io *io, char *dst, psd89_u32 dst_size)
{
    psd89_u32 count;
    psd89_u32 i;
    psd89_u16 ch;
    psd89_u32 copy_n;

    if (!psd89_rd_be32(io, &count)) {
        return 0;
    }
    copy_n = (psd89_u32)count;
    if (dst != 0 && dst_size != 0U) {
        if (copy_n >= dst_size) {
            copy_n = dst_size - 1U;
        }
    } else {
        copy_n = 0U;
    }
    for (i = 0U; i < (psd89_u32)count; ++i) {
        if (!psd89_rd_be16(io, &ch)) {
            return 0;
        }
        if (dst != 0 && i < copy_n) {
            dst[i] = (char)((ch <= 0x7FU) ? ch : '?');
        }
    }
    if (!psd89_rd_be16(io, &ch)) {
        return 0;
    }
    if (dst != 0 && dst_size != 0U) {
        dst[copy_n] = '\0';
    }
    return 1;
}

static int psd89_read_desc_id(psd89_io *io, psd89_descriptor_id *id)
{
    psd89_u32 len;
    psd89_u32 copy_n;
    psd89_u8 buf[256];
    if (!psd89_rd_be32(io, &len)) {
        return 0;
    }
    psd89_desc_id_init(id);
    if (len == 0U) {
        if (!psd89_io_read(io, id->fourcc, 4U)) {
            return 0;
        }
        id->is_fourcc = 1U;
        return 1;
    }
    copy_n = (psd89_u32)len;
    if (copy_n > PSD89_MAX_DESCRIPTOR_NAME_CHARS) {
        copy_n = PSD89_MAX_DESCRIPTOR_NAME_CHARS;
    }
    if ((psd89_u32)len > sizeof(buf)) {
        if (!psd89_io_read(io, buf, copy_n)) {
            return 0;
        }
        id->text[copy_n] = '\0';
        memcpy(id->text, buf, copy_n);
        return psd89_skip(io, (psd89_u32)len - copy_n);
    }
    if (!psd89_io_read(io, buf, (psd89_u32)len)) {
        return 0;
    }
    memcpy(id->text, buf, copy_n);
    id->text[copy_n] = '\0';
    return 1;
}

static int psd89_skip_desc_id(psd89_io *io)
{
    psd89_u32 len;
    if (!psd89_rd_be32(io, &len)) {
        return 0;
    }
    if (len == 0U) {
        return psd89_skip(io, 4U);
    }
    return psd89_skip(io, (psd89_u32)len);
}

static int psd89_skip_reference_item(psd89_io *io, const char ref_type[4])
{
    if (psd89_desc_key_eq(ref_type, "prop")) {
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io) && psd89_skip_desc_id(io);
    }
    if (psd89_desc_key_eq(ref_type, "Clss")) {
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io);
    }
    if (psd89_desc_key_eq(ref_type, "Enmr")) {
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io) && psd89_skip_desc_id(io) && psd89_skip_desc_id(io);
    }
    if (psd89_desc_key_eq(ref_type, "rele") || psd89_desc_key_eq(ref_type, "Idnt") || psd89_desc_key_eq(ref_type, "indx")) {
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io) && psd89_skip(io, 4U);
    }
    if (psd89_desc_key_eq(ref_type, "name")) {
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io) && psd89_skip_unicode_string(io);
    }
    return 0;
}

static void psd89_desc_id_copy_ascii(const psd89_descriptor_id *id,
                                      char *dst,
                                      psd89_u32 dst_size)
{
    psd89_u32 i;
    if (dst == 0 || dst_size == 0U) {
        return;
    }
    dst[0] = '\0';
    if (id == 0) {
        return;
    }
    if (id->is_fourcc) {
        if (dst_size > 5U) {
            memcpy(dst, id->fourcc, 4U);
            dst[4] = '\0';
        } else {
            for (i = 0U; i + 1U < dst_size && i < 4U; ++i) {
                dst[i] = id->fourcc[i];
            }
            dst[i] = '\0';
        }
        return;
    }
    strncpy(dst, id->text, dst_size - 1U);
    dst[dst_size - 1U] = '\0';
}

static int psd89_skip_descriptor_value(psd89_io *io,
                                       const char type[4],
                                       psd89_descriptor_item *item,
                                       int depth);

static int psd89_parse_descriptor_internal(psd89_io *io,
                                           psd89_descriptor_summary *summary,
                                           int depth)
{
    psd89_u32 count;
    psd89_u32 i;
    psd89_descriptor_item *item;
    psd89_descriptor_id tmp_id;
    char type[4];

    if (depth > 16) {
        return 0;
    }
    if (summary != 0) {
        psd89_descriptor_summary_init(summary);
        summary->max_depth_seen = (psd89_u8)depth;
    }
    if (summary != 0) {
        if (!psd89_read_unicode_ascii(io, summary->name, sizeof(summary->name))) {
            return 0;
        }
        if (!psd89_read_desc_id(io, &summary->class_id)) {
            return 0;
        }
    } else {
        if (!psd89_skip_unicode_string(io) || !psd89_skip_desc_id(io)) {
            return 0;
        }
    }
    if (!psd89_rd_be32(io, &count)) {
        return 0;
    }
    if (summary != 0) {
        summary->item_count = count;
    }
    for (i = 0U; i < count; ++i) {
        item = 0;
        if (summary != 0 && summary->parsed_item_count < PSD89_MAX_DESCRIPTOR_ITEMS) {
            item = &summary->items[summary->parsed_item_count];
            memset(item, 0, sizeof(*item));
        }
        if (item != 0) {
            if (!psd89_read_desc_id(io, &item->key)) {
                return 0;
            }
        } else {
            if (!psd89_read_desc_id(io, &tmp_id)) {
                return 0;
            }
        }
        if (!psd89_io_read(io, type, 4U)) {
            return 0;
        }
        if (item != 0) {
            memcpy(item->type, type, 4U);
        }
        if (!psd89_skip_descriptor_value(io, type, item, depth + 1)) {
            return 0;
        }
        if (item != 0) {
            if ((psd89_u8)(depth + 1) > summary->max_depth_seen) {
                summary->max_depth_seen = (psd89_u8)(depth + 1);
            }
            if (item->list_count_valid) {
                ++summary->parsed_list_count;
            }
            if (item->ref_count_valid) {
                ++summary->parsed_ref_count;
            }
            if (item->nested_valid) {
                ++summary->parsed_nested_count;
            }
            ++summary->parsed_item_count;
        }
    }
    if (summary != 0) {
        summary->parsed = 1U;
    }
    return 1;
}

static int psd89_skip_list(psd89_io *io, psd89_descriptor_item *item, int depth)
{
    psd89_u32 count;
    psd89_u32 i;
    char type[4];
    if (!psd89_rd_be32(io, &count)) {
        return 0;
    }
    if (item != 0) {
        item->list_count_valid = 1U;
        item->list_count = count;
    }
    for (i = 0U; i < count; ++i) {
        if (!psd89_io_read(io, type, 4U)) {
            return 0;
        }
        if (!psd89_skip_descriptor_value(io, type, 0, depth + 1)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_skip_reference(psd89_io *io, psd89_descriptor_item *item)
{
    psd89_u32 count;
    psd89_u32 i;
    char type[4];
    if (!psd89_rd_be32(io, &count)) {
        return 0;
    }
    if (item != 0) {
        item->ref_count_valid = 1U;
        item->ref_count = count;
    }
    for (i = 0U; i < count; ++i) {
        if (!psd89_io_read(io, type, 4U)) {
            return 0;
        }
        if (!psd89_skip_reference_item(io, type)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_skip_descriptor_value(psd89_io *io,
                                       const char type[4],
                                       psd89_descriptor_item *item,
                                       int depth)
{
    psd89_s32 s32v;
    psd89_fx16 fxv;
    psd89_u8 u8v;
    psd89_descriptor_id id1;
    psd89_descriptor_id id2;
    char units[4];
    psd89_u8 raw8[8];
    psd89_u32 len;
    psd89_descriptor_summary nested;
    char tmp_text[PSD89_MAX_DESCRIPTOR_NAME_CHARS + 1];

    if (psd89_desc_key_eq(type, "bool")) {
        if (!psd89_rd_u8(io, &u8v)) {
            return 0;
        }
        if (item != 0) {
            item->bool_valid = 1U;
            item->bool_value = (psd89_u8)(u8v != 0U);
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "long")) {
        if (!psd89_rd_be32s(io, &s32v)) {
            return 0;
        }
        if (item != 0) {
            item->int_valid = 1U;
            item->int_value = s32v;
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "comp")) {
        if (!psd89_rd_be8_raw(io, raw8)) {
            return 0;
        }
        if (item != 0) {
            item->comp_valid = 1U;
            item->comp_hi = ((psd89_u32)raw8[0] << 24) | ((psd89_u32)raw8[1] << 16) |
                            ((psd89_u32)raw8[2] << 8) | (psd89_u32)raw8[3];
            item->comp_lo = ((psd89_u32)raw8[4] << 24) | ((psd89_u32)raw8[5] << 16) |
                            ((psd89_u32)raw8[6] << 8) | (psd89_u32)raw8[7];
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "doub")) {
        if (!psd89_rd_ieee8_q16(io, &fxv)) {
            return 0;
        }
        if (item != 0) {
            item->fx_valid = 1U;
            item->fx_value = fxv;
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "UntF")) {
        if (!psd89_io_read(io, units, 4U) || !psd89_rd_ieee8_q16(io, &fxv)) {
            return 0;
        }
        if (item != 0) {
            item->fx_valid = 1U;
            item->fx_value = fxv;
            item->unit_valid = 1U;
            memcpy(item->unit, units, 4U);
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "TEXT")) {
        if (item != 0) {
            if (!psd89_read_unicode_ascii(io, item->text, sizeof(item->text))) {
                return 0;
            }
            item->text_valid = 1U;
            return 1;
        }
        return psd89_skip_unicode_string(io);
    }
    if (psd89_desc_key_eq(type, "enum")) {
        if (!psd89_read_desc_id(io, &id1) || !psd89_read_desc_id(io, &id2)) {
            return 0;
        }
        if (item != 0) {
            psd89_desc_id_copy_ascii(&id2, item->text, sizeof(item->text));
            item->text_valid = 1U;
            psd89_desc_id_copy_ascii(&id1, item->aux_text, sizeof(item->aux_text));
            item->aux_text_valid = 1U;
        }
        return 1;
    }
    if (psd89_desc_key_eq(type, "Objc") || psd89_desc_key_eq(type, "GlbO")) {
        if (item != 0) {
            if (!psd89_parse_descriptor_internal(io, &nested, depth + 1)) {
                return 0;
            }
            item->nested_valid = 1U;
            item->nested_item_count = nested.item_count;
            if (nested.name[0] != '\0') {
                strncpy(item->aux_text, nested.name, PSD89_MAX_DESCRIPTOR_NAME_CHARS);
                item->aux_text[PSD89_MAX_DESCRIPTOR_NAME_CHARS] = '\0';
                item->aux_text_valid = 1U;
            } else {
                psd89_desc_id_copy_ascii(&nested.class_id, item->aux_text, sizeof(item->aux_text));
                item->aux_text_valid = 1U;
            }
            return 1;
        }
        return psd89_parse_descriptor_internal(io, 0, depth + 1);
    }
    if (psd89_desc_key_eq(type, "VlLs")) {
        return psd89_skip_list(io, item, depth + 1);
    }
    if (psd89_desc_key_eq(type, "type") || psd89_desc_key_eq(type, "GlbC")) {
        if (item != 0) {
            if (!psd89_read_unicode_ascii(io, tmp_text, sizeof(tmp_text)) || !psd89_read_desc_id(io, &id1)) {
                return 0;
            }
            if (tmp_text[0] != '\0') {
                strncpy(item->text, tmp_text, PSD89_MAX_DESCRIPTOR_NAME_CHARS);
                item->text[PSD89_MAX_DESCRIPTOR_NAME_CHARS] = '\0';
                item->text_valid = 1U;
            }
            psd89_desc_id_copy_ascii(&id1, item->aux_text, sizeof(item->aux_text));
            item->aux_text_valid = 1U;
            return 1;
        }
        return psd89_skip_unicode_string(io) && psd89_skip_desc_id(io);
    }
    if (psd89_desc_key_eq(type, "alis") || psd89_desc_key_eq(type, "tdta")) {
        if (!psd89_rd_be32(io, &len)) {
            return 0;
        }
        if (item != 0) {
            item->length_valid = 1U;
            item->data_length = len;
        }
        return psd89_skip(io, (psd89_u32)len);
    }
    if (psd89_desc_key_eq(type, "obj ")) {
        return psd89_skip_reference(io, item);
    }
    return 0;
}

int psd89_descriptor_parse_io(psd89_io *io, psd89_descriptor_summary *summary)
{
    return psd89_parse_descriptor_internal(io, summary, 0);
}

static int psd89_wr_unicode_ascii(psd89_io *io, const char *s)
{
    psd89_u32 len;
    psd89_u32 i;
    psd89_u16 ch;
    if (s == 0) {
        s = "";
    }
    len = 0U;
    while (s[len] != '\0') {
        ++len;
    }
    if (!psd89_wr_be32(io, (psd89_u32)len)) {
        return 0;
    }
    for (i = 0U; i < len; ++i) {
        ch = (psd89_u16)(unsigned char)s[i];
        if (!psd89_wr_be16(io, ch)) {
            return 0;
        }
    }
    return psd89_wr_be16(io, 0U);
}

static int psd89_write_empty_descriptor(psd89_io *io)
{
    return psd89_wr_unicode_ascii(io, "") &&
           psd89_wr_be32(io, 0U) &&
           psd89_io_write(io, "null", 4U) &&
           psd89_wr_be32(io, 0U);
}


static int psd89_read_color5(psd89_io *io, psd89_u16 out[PSD89_MAX_LRFX_COLOR_COMPONENTS])
{
    unsigned int i;
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_rd_be16(io, &out[i])) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_color5(psd89_io *io, const psd89_u16 src[PSD89_MAX_LRFX_COLOR_COMPONENTS])
{
    unsigned int i;
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_wr_be16(io, src[i])) {
            return 0;
        }
    }
    return 1;
}

static int psd89_read_blend_mode_sig_key(psd89_io *io, char key[4])
{
    psd89_u32 sig;
    if (!psd89_rd_be32(io, &sig) || !psd89_io_read(io, key, 4U)) {
        return 0;
    }
    return sig == PSD89_SIG_8BIM;
}

static int psd89_write_blend_mode_sig_key(psd89_io *io, const char key[4])
{
    static const char norm[4] = { 'n', 'o', 'r', 'm' };
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM)) {
        return 0;
    }
    if (key[0] != '\0' || key[1] != '\0' || key[2] != '\0' || key[3] != '\0') {
        return psd89_io_write(io, key, 4U);
    }
    return psd89_io_write(io, norm, 4U);
}

static int psd89_parse_lrfx_common(psd89_layer *layer, psd89_io *io, psd89_u32 size)
{
    psd89_u32 version;
    psd89_u8 visible;
    psd89_u16 unused;
    if (size < 7U) {
        return 0;
    }
    if (!psd89_rd_be32(io, &version) || !psd89_rd_u8(io, &visible) || !psd89_rd_be16(io, &unused)) {
        return 0;
    }
    if (size > 7U && !psd89_skip(io, (psd89_u32)(size - 7U))) {
        return 0;
    }
    layer->lrfx.common_state_present = 1U;
    layer->lrfx.common_visible = (psd89_u8)(visible != 0U);
    return 1;
}

static int psd89_parse_lrfx_shadow(psd89_lrfx_shadow *fx, psd89_io *io, psd89_u32 size)
{
    if (fx == 0 || size < 41U) {
        return 0;
    }
    memset(fx, 0, sizeof(*fx));
    fx->present = 1U;
    fx->size = size;
    if (!psd89_rd_be32(io, &fx->version) ||
        !psd89_rd_be32s(io, &fx->blur) ||
        !psd89_rd_be32s(io, &fx->intensity) ||
        !psd89_rd_be32s(io, &fx->angle) ||
        !psd89_rd_be32s(io, &fx->distance) ||
        !psd89_read_color5(io, fx->color) ||
        !psd89_read_blend_mode_sig_key(io, fx->blend_mode) ||
        !psd89_rd_u8(io, &fx->enabled) ||
        !psd89_rd_u8(io, &fx->use_global_angle) ||
        !psd89_rd_u8(io, &fx->opacity)) {
        return 0;
    }
    if (fx->version >= 2U) {
        if (size < 51U || !psd89_read_color5(io, fx->native_color)) {
            return 0;
        }
        fx->native_color_present = 1U;
        if (size > 51U && !psd89_skip(io, (psd89_u32)(size - 51U))) {
            return 0;
        }
    } else if (size > 41U && !psd89_skip(io, (psd89_u32)(size - 41U))) {
        return 0;
    }
    return 1;
}

static int psd89_parse_lrfx_glow(psd89_lrfx_glow *fx, psd89_io *io, psd89_u32 size, int inner)
{
    if (fx == 0 || size < (inner ? 33U : 32U)) {
        return 0;
    }
    memset(fx, 0, sizeof(*fx));
    fx->present = 1U;
    fx->size = size;
    if (!psd89_rd_be32(io, &fx->version) ||
        !psd89_rd_be32s(io, &fx->blur) ||
        !psd89_rd_be32s(io, &fx->intensity) ||
        !psd89_read_color5(io, fx->color) ||
        !psd89_read_blend_mode_sig_key(io, fx->blend_mode) ||
        !psd89_rd_u8(io, &fx->enabled) ||
        !psd89_rd_u8(io, &fx->opacity)) {
        return 0;
    }
    if (inner && fx->version >= 2U) {
        if (size < 43U || !psd89_rd_u8(io, &fx->invert) || !psd89_read_color5(io, fx->native_color)) {
            return 0;
        }
        fx->native_color_present = 1U;
        if (size > 43U && !psd89_skip(io, (psd89_u32)(size - 43U))) {
            return 0;
        }
    } else if (!inner && fx->version >= 2U) {
        if (size < 42U || !psd89_read_color5(io, fx->native_color)) {
            return 0;
        }
        fx->native_color_present = 1U;
        if (size > 42U && !psd89_skip(io, (psd89_u32)(size - 42U))) {
            return 0;
        }
    } else {
        psd89_u32 base = inner ? 33U : 32U;
        if (size > base && !psd89_skip(io, (psd89_u32)(size - base))) {
            return 0;
        }
    }
    return 1;
}

static int psd89_parse_lrfx_bevel(psd89_lrfx_bevel *fx, psd89_io *io, psd89_u32 size)
{
    if (fx == 0 || size < 58U) {
        return 0;
    }
    memset(fx, 0, sizeof(*fx));
    fx->present = 1U;
    fx->size = size;
    if (!psd89_rd_be32(io, &fx->version) ||
        !psd89_rd_be32s(io, &fx->angle) ||
        !psd89_rd_be32s(io, &fx->strength) ||
        !psd89_rd_be32s(io, &fx->blur) ||
        !psd89_read_blend_mode_sig_key(io, fx->highlight_blend_mode) ||
        !psd89_read_blend_mode_sig_key(io, fx->shadow_blend_mode) ||
        !psd89_read_color5(io, fx->highlight_color) ||
        !psd89_read_color5(io, fx->shadow_color) ||
        !psd89_rd_u8(io, &fx->bevel_style) ||
        !psd89_rd_u8(io, &fx->highlight_opacity) ||
        !psd89_rd_u8(io, &fx->shadow_opacity) ||
        !psd89_rd_u8(io, &fx->enabled) ||
        !psd89_rd_u8(io, &fx->use_global_angle) ||
        !psd89_rd_u8(io, &fx->up)) {
        return 0;
    }
    if (fx->version >= 2U) {
        if (size < 78U || !psd89_read_color5(io, fx->real_highlight_color) || !psd89_read_color5(io, fx->real_shadow_color)) {
            return 0;
        }
        fx->real_colors_present = 1U;
        if (size > 78U && !psd89_skip(io, (psd89_u32)(size - 78U))) {
            return 0;
        }
    } else if (size > 58U && !psd89_skip(io, (psd89_u32)(size - 58U))) {
        return 0;
    }
    return 1;
}

static int psd89_parse_lrfx_solid_fill(psd89_layer *layer, psd89_io *io, psd89_u32 size)
{
    unsigned int i;
    psd89_lrfx_solid_fill *sf;
    psd89_u32 version;

    if (size < 34U) {
        return 0;
    }
    sf = &layer->lrfx.solid_fill;
    memset(sf, 0, sizeof(*sf));
    sf->present = 1U;
    sf->size = size;
    if (!psd89_rd_be32(io, &version)) {
        return 0;
    }
    sf->version = version;
    if (!psd89_io_read(io, sf->blend_mode, 4U)) {
        return 0;
    }
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_rd_be16(io, &sf->color[i])) {
            return 0;
        }
    }
    if (!psd89_rd_u8(io, &sf->opacity) || !psd89_rd_u8(io, &sf->enabled)) {
        return 0;
    }
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_rd_be16(io, &sf->native_color[i])) {
            return 0;
        }
    }
    if (size > 34U && !psd89_skip(io, (psd89_u32)(size - 34U))) {
        return 0;
    }
    return 1;
}

int psd89_parse_lrfx_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len)
{
    psd89_u16 version;
    psd89_u16 count;
    psd89_u16 i;
    psd89_u32 sig;
    char effect_key[4];
    psd89_u32 size;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    memset(&layer->lrfx, 0, sizeof(layer->lrfx));
    layer->lrfx.present = 1U;
    if (!psd89_rd_be16(io, &version) || !psd89_rd_be16(io, &count)) {
        return PSD89_E_TRUNCATED;
    }
    layer->lrfx.version = version;
    layer->lrfx.effect_count = count;
    for (i = 0U; i < count; ++i) {
        if (!psd89_rd_be32(io, &sig) || !psd89_io_read(io, effect_key, 4U) || !psd89_rd_be32(io, &size)) {
            return PSD89_E_TRUNCATED;
        }
        if (sig != PSD89_SIG_8BIM) {
            return PSD89_E_BAD_LAYER;
        }
        if (psd89_desc_key_eq(effect_key, "cmnS")) {
            if (!psd89_parse_lrfx_common(layer, io, size)) {
                return PSD89_E_TRUNCATED;
            }
        } else if (psd89_desc_key_eq(effect_key, "dsdw")) {
            if (!psd89_parse_lrfx_shadow(&layer->lrfx.drop_shadow, io, size)) {
                return PSD89_E_TRUNCATED;
            }
            layer->lrfx.drop_shadow_present = 1U;
        } else if (psd89_desc_key_eq(effect_key, "isdw")) {
            if (!psd89_parse_lrfx_shadow(&layer->lrfx.inner_shadow, io, size)) {
                return PSD89_E_TRUNCATED;
            }
            layer->lrfx.inner_shadow_present = 1U;
        } else if (psd89_desc_key_eq(effect_key, "oglw")) {
            if (!psd89_parse_lrfx_glow(&layer->lrfx.outer_glow, io, size, 0)) {
                return PSD89_E_TRUNCATED;
            }
            layer->lrfx.outer_glow_present = 1U;
        } else if (psd89_desc_key_eq(effect_key, "iglw")) {
            if (!psd89_parse_lrfx_glow(&layer->lrfx.inner_glow, io, size, 1)) {
                return PSD89_E_TRUNCATED;
            }
            layer->lrfx.inner_glow_present = 1U;
        } else if (psd89_desc_key_eq(effect_key, "bevl")) {
            if (!psd89_parse_lrfx_bevel(&layer->lrfx.bevel, io, size)) {
                return PSD89_E_TRUNCATED;
            }
            layer->lrfx.bevel_present = 1U;
        } else if (psd89_desc_key_eq(effect_key, "sofi")) {
            if (!psd89_parse_lrfx_solid_fill(layer, io, size)) {
                return PSD89_E_TRUNCATED;
            }
        } else {
            if (!psd89_skip(io, (psd89_u32)size)) {
                return PSD89_E_TRUNCATED;
            }
        }
    }
    return PSD89_OK;
}

static int psd89_write_lrfx_common_block(psd89_io *io, const psd89_lrfx *lrfx)
{
    static const char cmnS[4] = { 'c', 'm', 'n', 'S' };
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, cmnS, 4U) || !psd89_wr_be32(io, 7U) ||
        !psd89_wr_be32(io, 0U) || !psd89_wr_u8(io, lrfx->common_visible ? 1U : 0U) || !psd89_wr_be16(io, 0U)) {
        return 0;
    }
    return 1;
}

static int psd89_write_lrfx_shadow_block(psd89_io *io, const char key[4], const psd89_lrfx_shadow *fx)
{
    psd89_u32 size;
    psd89_u32 version;
    version = fx->version != 0U ? fx->version : (fx->native_color_present ? 2U : 0U);
    size = version >= 2U ? 51U : 41U;
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, key, 4U) || !psd89_wr_be32(io, size) ||
        !psd89_wr_be32(io, version) ||
        !psd89_wr_be32(io, (psd89_u32)fx->blur) ||
        !psd89_wr_be32(io, (psd89_u32)fx->intensity) ||
        !psd89_wr_be32(io, (psd89_u32)fx->angle) ||
        !psd89_wr_be32(io, (psd89_u32)fx->distance) ||
        !psd89_write_color5(io, fx->color) ||
        !psd89_write_blend_mode_sig_key(io, fx->blend_mode) ||
        !psd89_wr_u8(io, fx->enabled ? 1U : 0U) ||
        !psd89_wr_u8(io, fx->use_global_angle ? 1U : 0U) ||
        !psd89_wr_u8(io, fx->opacity)) {
        return 0;
    }
    if (version >= 2U && !psd89_write_color5(io, fx->native_color_present ? fx->native_color : fx->color)) {
        return 0;
    }
    return 1;
}

static int psd89_write_lrfx_glow_block(psd89_io *io, const char key[4], const psd89_lrfx_glow *fx, int inner)
{
    psd89_u32 size;
    psd89_u32 version;
    version = fx->version != 0U ? fx->version : (fx->native_color_present || fx->invert ? 2U : 0U);
    size = inner ? (version >= 2U ? 43U : 33U) : (version >= 2U ? 42U : 32U);
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, key, 4U) || !psd89_wr_be32(io, size) ||
        !psd89_wr_be32(io, version) ||
        !psd89_wr_be32(io, (psd89_u32)fx->blur) ||
        !psd89_wr_be32(io, (psd89_u32)fx->intensity) ||
        !psd89_write_color5(io, fx->color) ||
        !psd89_write_blend_mode_sig_key(io, fx->blend_mode) ||
        !psd89_wr_u8(io, fx->enabled ? 1U : 0U) ||
        !psd89_wr_u8(io, fx->opacity)) {
        return 0;
    }
    if (inner && version >= 2U) {
        if (!psd89_wr_u8(io, fx->invert ? 1U : 0U) ||
            !psd89_write_color5(io, fx->native_color_present ? fx->native_color : fx->color)) {
            return 0;
        }
    } else if (!inner && version >= 2U) {
        if (!psd89_write_color5(io, fx->native_color_present ? fx->native_color : fx->color)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_lrfx_bevel_block(psd89_io *io, const psd89_lrfx_bevel *fx)
{
    static const char key[4] = { 'b', 'e', 'v', 'l' };
    psd89_u32 version;
    psd89_u32 size;
    version = fx->version != 0U ? fx->version : (fx->real_colors_present ? 2U : 0U);
    size = version >= 2U ? 78U : 58U;
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, key, 4U) || !psd89_wr_be32(io, size) ||
        !psd89_wr_be32(io, version) ||
        !psd89_wr_be32(io, (psd89_u32)fx->angle) ||
        !psd89_wr_be32(io, (psd89_u32)fx->strength) ||
        !psd89_wr_be32(io, (psd89_u32)fx->blur) ||
        !psd89_write_blend_mode_sig_key(io, fx->highlight_blend_mode) ||
        !psd89_write_blend_mode_sig_key(io, fx->shadow_blend_mode) ||
        !psd89_write_color5(io, fx->highlight_color) ||
        !psd89_write_color5(io, fx->shadow_color) ||
        !psd89_wr_u8(io, fx->bevel_style) ||
        !psd89_wr_u8(io, fx->highlight_opacity) ||
        !psd89_wr_u8(io, fx->shadow_opacity) ||
        !psd89_wr_u8(io, fx->enabled ? 1U : 0U) ||
        !psd89_wr_u8(io, fx->use_global_angle ? 1U : 0U) ||
        !psd89_wr_u8(io, fx->up ? 1U : 0U)) {
        return 0;
    }
    if (version >= 2U) {
        if (!psd89_write_color5(io, fx->real_colors_present ? fx->real_highlight_color : fx->highlight_color) ||
            !psd89_write_color5(io, fx->real_colors_present ? fx->real_shadow_color : fx->shadow_color)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_lrfx_sofi_block(psd89_io *io, const psd89_lrfx_solid_fill *sf)
{
    static const char sofi[4] = { 's', 'o', 'f', 'i' };
    unsigned int i;
    const char norm[4] = { 'n', 'o', 'r', 'm' };
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, sofi, 4U) || !psd89_wr_be32(io, 34U) ||
        !psd89_wr_be32(io, sf->version != 0U ? sf->version : 2U)) {
        return 0;
    }
    if (sf->blend_mode[0] != '\0' || sf->blend_mode[1] != '\0' || sf->blend_mode[2] != '\0' || sf->blend_mode[3] != '\0') {
        if (!psd89_io_write(io, sf->blend_mode, 4U)) {
            return 0;
        }
    } else if (!psd89_io_write(io, norm, 4U)) {
        return 0;
    }
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_wr_be16(io, sf->color[i])) {
            return 0;
        }
    }
    if (!psd89_wr_u8(io, sf->opacity) || !psd89_wr_u8(io, sf->enabled ? 1U : 0U)) {
        return 0;
    }
    for (i = 0U; i < PSD89_MAX_LRFX_COLOR_COMPONENTS; ++i) {
        if (!psd89_wr_be16(io, sf->native_color[i])) {
            return 0;
        }
    }
    return 1;
}

int psd89_write_lrfx_tag(psd89_io *io, const psd89_layer *layer)
{
    static const char lrfx_key[4] = { 'l', 'r', 'F', 'X' };
    static const char dsdw[4] = { 'd', 's', 'd', 'w' };
    static const char isdw[4] = { 'i', 's', 'd', 'w' };
    static const char oglw[4] = { 'o', 'g', 'l', 'w' };
    static const char iglw[4] = { 'i', 'g', 'l', 'w' };
    psd89_u32 len_pos;
    psd89_u32 start;
    psd89_u16 count;
    const psd89_lrfx *lrfx;

    if (layer == 0 || !layer->lrfx.present) {
        return 1;
    }
    lrfx = &layer->lrfx;
    count = 0U;
    if (lrfx->common_state_present) {
        ++count;
    }
    if (lrfx->drop_shadow.present) {
        ++count;
    }
    if (lrfx->inner_shadow.present) {
        ++count;
    }
    if (lrfx->outer_glow.present) {
        ++count;
    }
    if (lrfx->inner_glow.present) {
        ++count;
    }
    if (lrfx->bevel.present) {
        ++count;
    }
    if (lrfx->solid_fill.present) {
        ++count;
    }
    if (count == 0U) {
        return 1;
    }
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, lrfx_key, 4U)) {
        return 0;
    }
    len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (!psd89_wr_be16(io, lrfx->version) || !psd89_wr_be16(io, count)) {
        return 0;
    }
    if (lrfx->common_state_present && !psd89_write_lrfx_common_block(io, lrfx)) {
        return 0;
    }
    if (lrfx->drop_shadow.present && !psd89_write_lrfx_shadow_block(io, dsdw, &lrfx->drop_shadow)) {
        return 0;
    }
    if (lrfx->inner_shadow.present && !psd89_write_lrfx_shadow_block(io, isdw, &lrfx->inner_shadow)) {
        return 0;
    }
    if (lrfx->outer_glow.present && !psd89_write_lrfx_glow_block(io, oglw, &lrfx->outer_glow, 0)) {
        return 0;
    }
    if (lrfx->inner_glow.present && !psd89_write_lrfx_glow_block(io, iglw, &lrfx->inner_glow, 1)) {
        return 0;
    }
    if (lrfx->bevel.present && !psd89_write_lrfx_bevel_block(io, &lrfx->bevel)) {
        return 0;
    }
    if (lrfx->solid_fill.present && !psd89_write_lrfx_sofi_block(io, &lrfx->solid_fill)) {
        return 0;
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
}

static int psd89_read_q16_doubles(psd89_io *io, psd89_fx16 *dst, unsigned int count)
{
    unsigned int i;
    for (i = 0U; i < count; ++i) {
        if (!psd89_rd_ieee8_q16(io, &dst[i])) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_q16_doubles(psd89_io *io, const psd89_fx16 *src, unsigned int count)
{
    unsigned int i;
    for (i = 0U; i < count; ++i) {
        if (!psd89_wr_ieee8_from_q16(io, src[i])) {
            return 0;
        }
    }
    return 1;
}

int psd89_parse_tysh_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len)
{
    psd89_u32 start;
    psd89_u32 end;
    psd89_type_tool *tt;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    start = psd89_io_tell(io);
    end = start + (psd89_u32)len;
    tt = &layer->type_tool;
    memset(tt, 0, sizeof(*tt));
    tt->present = 1U;
    if (!psd89_rd_be16(io, &tt->version) || !psd89_read_q16_doubles(io, tt->transform, 6U) ||
        !psd89_rd_be16(io, &tt->text_version) || !psd89_rd_be32(io, &tt->text_descriptor_version)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_descriptor_parse_io(io, &tt->text.summary)) {
        return PSD89_E_BAD_LAYER;
    }
    if (!psd89_rd_be16(io, &tt->warp_version) || !psd89_rd_be32(io, &tt->warp_descriptor_version)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_descriptor_parse_io(io, &tt->warp.summary)) {
        return PSD89_E_BAD_LAYER;
    }
    if (!psd89_read_q16_doubles(io, tt->bounds, 4U)) {
        return PSD89_E_TRUNCATED;
    }
    if (psd89_io_tell(io) < end && !psd89_skip(io, end - psd89_io_tell(io))) {
        return PSD89_E_TRUNCATED;
    }
    if (psd89_io_tell(io) != end) {
        return PSD89_E_BAD_LAYER;
    }
    return PSD89_OK;
}

int psd89_write_tysh_tag(psd89_io *io, const psd89_layer *layer)
{
    static const char tysh_key[4] = { 'T', 'y', 'S', 'h' };
    const psd89_type_tool *tt;
    psd89_u32 len_pos;
    psd89_u32 start;

    if (layer == 0 || !layer->type_tool.present) {
        return 1;
    }
    tt = &layer->type_tool;
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, tysh_key, 4U)) {
        return 0;
    }
    len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (!psd89_wr_be16(io, tt->version != 0U ? tt->version : 1U) ||
        !psd89_write_q16_doubles(io, tt->transform, 6U) ||
        !psd89_wr_be16(io, tt->text_version != 0U ? tt->text_version : 50U) ||
        !psd89_wr_be32(io, tt->text_descriptor_version != 0U ? tt->text_descriptor_version : 16U)) {
        return 0;
    }
    if (tt->text.authored_raw_size != 0U) {
        if (!psd89_io_write(io, tt->text.authored_raw, (psd89_u32)tt->text.authored_raw_size)) {
            return 0;
        }
    } else if (!psd89_write_empty_descriptor(io)) {
        return 0;
    }
    if (!psd89_wr_be16(io, tt->warp_version != 0U ? tt->warp_version : 1U) ||
        !psd89_wr_be32(io, tt->warp_descriptor_version != 0U ? tt->warp_descriptor_version : 16U)) {
        return 0;
    }
    if (tt->warp.authored_raw_size != 0U) {
        if (!psd89_io_write(io, tt->warp.authored_raw, (psd89_u32)tt->warp.authored_raw_size)) {
            return 0;
        }
    } else if (!psd89_write_empty_descriptor(io)) {
        return 0;
    }
    if (!psd89_write_q16_doubles(io, tt->bounds, 4U)) {
        return 0;
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
}

int psd89_parse_txt2_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len)
{
    psd89_u32 raw_len;
    psd89_u32 start;
    psd89_u32 raw_off;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    start = psd89_io_tell(io);
    memset(&layer->text_engine, 0, sizeof(layer->text_engine));
    layer->text_engine.present = 1U;
    if (!psd89_rd_be32(io, &raw_len)) {
        return PSD89_E_TRUNCATED;
    }
    raw_off = psd89_io_tell(io);
    if ((psd89_u32)raw_len > (psd89_u32)len - 4U) {
        raw_len = len >= 4U ? (psd89_u32)(len - 4U) : 0U;
    }
    layer->text_engine.raw_offset = raw_off;
    layer->text_engine.raw_size = raw_len;
    if (!psd89_skip(io, (psd89_u32)raw_len)) {
        return PSD89_E_TRUNCATED;
    }
    if (psd89_io_tell(io) < start + (psd89_u32)len) {
        if (!psd89_skip(io, start + (psd89_u32)len - psd89_io_tell(io))) {
            return PSD89_E_TRUNCATED;
        }
    }
    return PSD89_OK;
}

int psd89_write_txt2_tag(psd89_io *io, const psd89_layer *layer)
{
    static const char txt2_key[4] = { 'T', 'x', 't', '2' };
    const psd89_text_engine_data *te;
    if (layer == 0 || !layer->text_engine.present || layer->text_engine.authored_raw == 0 || layer->text_engine.authored_raw_size == 0U) {
        return 1;
    }
    te = &layer->text_engine;
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, txt2_key, 4U) ||
        !psd89_wr_be32(io, 4U + te->authored_raw_size) ||
        !psd89_wr_be32(io, te->authored_raw_size) ||
        !psd89_io_write(io, te->authored_raw, (psd89_u32)te->authored_raw_size)) {
        return 0;
    }
    return 1;
}


int psd89_parse_lfx2_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len)
{
    psd89_u32 start;
    psd89_u32 end;
    psd89_object_effects *fx;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    start = psd89_io_tell(io);
    end = start + (psd89_u32)len;
    fx = &layer->object_effects;
    memset(fx, 0, sizeof(*fx));
    fx->present = 1U;
    if (!psd89_rd_be32(io, &fx->object_version) || !psd89_rd_be32(io, &fx->descriptor_version)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_descriptor_parse_io(io, &fx->descriptor.summary)) {
        if (!psd89_io_seek(io, end)) {
            return PSD89_E_TRUNCATED;
        }
        fx->descriptor.summary.parsed = 0U;
        return PSD89_OK;
    }
    if (psd89_io_tell(io) < end && !psd89_skip(io, end - psd89_io_tell(io))) {
        return PSD89_E_TRUNCATED;
    }
    return PSD89_OK;
}

int psd89_write_lfx2_tag(psd89_io *io, const psd89_layer *layer)
{
    static const char key[4] = { 'l', 'f', 'x', '2' };
    const psd89_object_effects *fx;
    psd89_u32 len_pos;
    psd89_u32 start;
    if (layer == 0 || !layer->object_effects.present) {
        return 1;
    }
    fx = &layer->object_effects;
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, key, 4U)) {
        return 0;
    }
    len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (!psd89_wr_be32(io, fx->object_version) || !psd89_wr_be32(io, fx->descriptor_version != 0U ? fx->descriptor_version : 16U)) {
        return 0;
    }
    if (fx->descriptor.authored_raw_size != 0U) {
        if (!psd89_io_write(io, fx->descriptor.authored_raw, (psd89_u32)fx->descriptor.authored_raw_size)) {
            return 0;
        }
    } else if (!psd89_write_empty_descriptor(io)) {
        return 0;
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
}

static int psd89_parse_plld_tag(psd89_smart_object *so, psd89_io *io, psd89_u32 len)
{
    psd89_u32 start;
    psd89_u32 end;
    psd89_u32 desc_version;

    start = psd89_io_tell(io);
    end = start + (psd89_u32)len;
    memset(so, 0, sizeof(*so));
    so->present = 1U;
    so->placed_info_present = 1U;
    memcpy(so->tag_key, "plLd", 4U);
    if (!psd89_io_read(io, so->type, 4U) || !psd89_rd_be32(io, &so->version)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_read_pascal_string(io, so->unique_id, sizeof(so->unique_id), 2U) ||
        !psd89_rd_be32(io, &so->page_number) ||
        !psd89_rd_be32(io, &so->total_pages) ||
        !psd89_rd_be32(io, &so->anti_alias_policy) ||
        !psd89_rd_be32(io, &so->placed_layer_type) ||
        !psd89_read_q16_doubles(io, so->transform, 8U) ||
        !psd89_rd_be32(io, &so->warp_version) ||
        !psd89_rd_be32(io, &desc_version)) {
        return PSD89_E_TRUNCATED;
    }
    so->warp_descriptor_version = desc_version;
    so->descriptor_version = desc_version;
    if (!psd89_descriptor_parse_io(io, &so->warp_descriptor.summary)) {
        if (!psd89_io_seek(io, end)) {
            return PSD89_E_TRUNCATED;
        }
        return PSD89_OK;
    }
    so->descriptor.summary = so->warp_descriptor.summary;
    if (psd89_io_tell(io) < end && !psd89_skip(io, end - psd89_io_tell(io))) {
        return PSD89_E_TRUNCATED;
    }
    return PSD89_OK;
}

int psd89_parse_smart_object_tag(psd89_layer *layer, const char key[4], psd89_io *io, psd89_u32 len)
{
    psd89_u32 start;
    psd89_u32 end;
    psd89_smart_object *so;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (psd89_desc_key_eq(key, "plLd")) {
        return psd89_parse_plld_tag(&layer->smart_object, io, len);
    }
    start = psd89_io_tell(io);
    end = start + (psd89_u32)len;
    so = &layer->smart_object;
    memset(so, 0, sizeof(*so));
    so->present = 1U;
    memcpy(so->tag_key, key, 4U);
    if (!psd89_io_read(io, so->type, 4U) || !psd89_rd_be32(io, &so->version)) {
        return PSD89_E_TRUNCATED;
    }
    if (psd89_desc_key_eq(key, "SoLd") || psd89_desc_key_eq(key, "SoLE")) {
        if (!psd89_rd_be32(io, &so->descriptor_version)) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_descriptor_parse_io(io, &so->descriptor.summary)) {
            if (!psd89_io_seek(io, end)) {
                return PSD89_E_TRUNCATED;
            }
            return PSD89_OK;
        }
        if (psd89_io_tell(io) < end && !psd89_skip(io, end - psd89_io_tell(io))) {
            return PSD89_E_TRUNCATED;
        }
        return PSD89_OK;
    }
    return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
}

int psd89_write_smart_object_tag(psd89_io *io, const psd89_layer *layer)
{
    const psd89_smart_object *so;
    psd89_u32 len_pos;
    psd89_u32 start;
    const char soLD[4] = { 's', 'o', 'L', 'D' };
    const char SoLd[4] = { 'S', 'o', 'L', 'd' };
    const char plcL[4] = { 'p', 'l', 'c', 'L' };
    const char plLd[4] = { 'p', 'l', 'L', 'd' };

    if (layer == 0 || !layer->smart_object.present) {
        return 1;
    }
    so = &layer->smart_object;
    if (psd89_desc_key_eq(so->tag_key, "plLd")) {
        if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, plLd, 4U)) {
            return 0;
        }
        len_pos = psd89_io_tell(io);
        if (!psd89_wr_be32(io, 0U)) {
            return 0;
        }
        start = psd89_io_tell(io);
        if (!psd89_io_write(io, so->type[0] != '\0' ? so->type : plcL, 4U) ||
            !psd89_wr_be32(io, so->version != 0U ? so->version : 3U) ||
            !psd89_write_pascal_string(io, so->unique_id, 2U) ||
            !psd89_wr_be32(io, so->page_number) ||
            !psd89_wr_be32(io, so->total_pages) ||
            !psd89_wr_be32(io, so->anti_alias_policy) ||
            !psd89_wr_be32(io, so->placed_layer_type) ||
            !psd89_write_q16_doubles(io, so->transform, 8U) ||
            !psd89_wr_be32(io, so->warp_version) ||
            !psd89_wr_be32(io, so->warp_descriptor_version != 0U ? so->warp_descriptor_version : 16U)) {
            return 0;
        }
        if (so->warp_descriptor.authored_raw_size != 0U) {
            if (!psd89_io_write(io, so->warp_descriptor.authored_raw, (psd89_u32)so->warp_descriptor.authored_raw_size)) {
                return 0;
            }
        } else if (!psd89_write_empty_descriptor(io)) {
            return 0;
        }
        return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
    }
    if (!(psd89_desc_key_eq(so->tag_key, "SoLd") || psd89_desc_key_eq(so->tag_key, "SoLE"))) {
        return 1;
    }
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) || !psd89_io_write(io, so->tag_key[0] != '\0' ? so->tag_key : SoLd, 4U)) {
        return 0;
    }
    len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (!psd89_io_write(io, so->type[0] != '\0' ? so->type : soLD, 4U) ||
        !psd89_wr_be32(io, so->version != 0U ? so->version : 4U) ||
        !psd89_wr_be32(io, so->descriptor_version != 0U ? so->descriptor_version : 16U)) {
        return 0;
    }
    if (so->descriptor.authored_raw_size != 0U) {
        if (!psd89_io_write(io, so->descriptor.authored_raw, (psd89_u32)so->descriptor.authored_raw_size)) {
            return 0;
        }
    } else if (!psd89_write_empty_descriptor(io)) {
        return 0;
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
}
