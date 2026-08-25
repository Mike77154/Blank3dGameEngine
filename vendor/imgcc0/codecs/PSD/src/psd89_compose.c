#include "psd89_internal.h"

#include <string.h>

typedef struct psd89_compose_channel_state {
    int present;
    int from_memory;
    psd89_u16 compression;
    const psd89_u8 *mem_plane;
    psd89_u32 mem_stride;
    psd89_u32 raw_base;
    psd89_u32 counts_offset;
    psd89_u32 data_pos;
    psd89_u32 current_row;
    psd89_u32 cols;
    psd89_u32 rows;
} psd89_compose_channel_state;

typedef struct psd89_compose_layer_state {
    const psd89_layer *layer;
    psd89_u32 rows;
    psd89_u32 cols;
    int active;
    psd89_s32 clip_left;
    psd89_s32 clip_right;
    psd89_s32 clip_top;
    psd89_s32 clip_bottom;
    psd89_u8 has_mask;
    psd89_u8 mask_disabled;
    psd89_u8 mask_default_color;
    psd89_u8 mask_invert;
    psd89_u8 mask_apply_params;
    psd89_u8 mask_density_present;
    psd89_u8 mask_density;
    psd89_u8 mask_feather_present;
    psd89_u8 layer_mask_global;
    psd89_fx16 mask_feather;
    psd89_s32 mask_left;
    psd89_s32 mask_right;
    psd89_s32 mask_top;
    psd89_s32 mask_bottom;
    psd89_u8 has_vector_mask;
    psd89_u8 vector_disabled;
    psd89_u8 vector_density_present;
    psd89_u8 vector_density;
    psd89_u8 vector_feather_present;
    psd89_u8 vector_mask_global;
    psd89_fx16 vector_feather;
    psd89_s32 vector_left;
    psd89_s32 vector_right;
    psd89_s32 vector_top;
    psd89_s32 vector_bottom;
    const psd89_vector_mask *vector_mask;
    const psd89_vector_flatten_result *vector_flat;
    psd89_compose_channel_state color[PSD89_MAX_BASE_CHANNELS];
    psd89_compose_channel_state alpha;
    psd89_compose_channel_state mask;
} psd89_compose_layer_state;

static const char psd89_norm_key_const[4] = { 'n', 'o', 'r', 'm' };

static int psd89_has_nonempty4(const char key[4])
{
    return key[0] != '\0' || key[1] != '\0' || key[2] != '\0' || key[3] != '\0';
}

static int psd89_layer_is_group_start(const psd89_layer *layer)
{
    return layer != 0 && layer->section_divider_present && layer->section_divider_type == 3U;
}

static int psd89_layer_is_group_end(const psd89_layer *layer)
{
    return layer != 0 && layer->section_divider_present &&
           (layer->section_divider_type == 1U || layer->section_divider_type == 2U);
}

static const char *psd89_layer_group_blend_key(const psd89_layer *layer)
{
    if (layer != 0 && psd89_has_nonempty4(layer->section_divider_blend_mode)) {
        return layer->section_divider_blend_mode;
    }
    if (layer != 0 && psd89_has_nonempty4(layer->blend_mode)) {
        return layer->blend_mode;
    }
    return psd89_norm_key_const;
}

static int psd89_build_group_index(const psd89_doc *doc,
                                   int *start_to_end,
                                   int *end_to_start,
                                   int *parent_group,
                                   int *has_groups)
{
    int stack[PSD89_MAX_LAYERS];
    int top;
    int i;

    if (doc == 0 || start_to_end == 0 || end_to_start == 0 || parent_group == 0 || has_groups == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    for (i = 0; i < (int)PSD89_MAX_LAYERS; ++i) {
        start_to_end[i] = -1;
        end_to_start[i] = -1;
        parent_group[i] = -1;
    }
    *has_groups = 0;
    top = 0;
    for (i = 0; i < (int)doc->layer_count; ++i) {
        if (psd89_layer_is_group_start(&doc->layers[i])) {
            if (top >= (int)PSD89_MAX_LAYERS) {
                return PSD89_E_LIMIT;
            }
            stack[top++] = i;
            *has_groups = 1;
        } else if (psd89_layer_is_group_end(&doc->layers[i])) {
            int start_idx;
            if (top <= 0) {
                continue;
            }
            start_idx = stack[--top];
            start_to_end[start_idx] = i;
            end_to_start[i] = start_idx;
            parent_group[i] = top > 0 ? stack[top - 1] : -1;
            *has_groups = 1;
        }
    }
    return PSD89_OK;
}

static int psd89_compose_layer_is_hidden_compat(const psd89_layer *layer)
{
    return (layer->flags & 0x02U) != 0U;
}

static int psd89_key_eq(const char key[4], const char *lit4)
{
    return key[0] == lit4[0] && key[1] == lit4[1] && key[2] == lit4[2] && key[3] == lit4[3];
}

static psd89_fx16 psd89_fx16_clamp_unit(psd89_fx16 v)
{
    if (v <= 0) {
        return 0;
    }
    if (v >= PSD89_FX16_ONE) {
        return PSD89_FX16_ONE;
    }
    return v;
}

static psd89_fx16 psd89_fx16_from_u8(psd89_u8 v)
{
    return (psd89_fx16)((((psd89_u32)v) * 65536U + 127U) / 255U);
}

static psd89_u8 psd89_fx16_to_u8_sat(psd89_fx16 v)
{
    psd89_u32 t;
    if (v <= 0) {
        return 0U;
    }
    if (v >= PSD89_FX16_ONE) {
        return 255U;
    }
    t = ((psd89_u32)v * 255U + 32768U) >> 16;
    if (t > 255U) {
        t = 255U;
    }
    return (psd89_u8)t;
}

static psd89_fx16 psd89_fx16_min(psd89_fx16 a, psd89_fx16 b)
{
    return a < b ? a : b;
}

static psd89_fx16 psd89_fx16_max(psd89_fx16 a, psd89_fx16 b)
{
    return a > b ? a : b;
}

static psd89_fx16 psd89_fx16_absdiff(psd89_fx16 a, psd89_fx16 b)
{
    return a >= b ? (psd89_fx16)(a - b) : (psd89_fx16)(b - a);
}

static psd89_fx16 psd89_blend_multiply(psd89_fx16 cb, psd89_fx16 cs)
{
    return psd89_fx16_mul(cb, cs);
}

static psd89_fx16 psd89_blend_screen(psd89_fx16 cb, psd89_fx16 cs)
{
    psd89_fx16 t;
    t = (psd89_fx16)(cb + cs - psd89_fx16_mul(cb, cs));
    return psd89_fx16_clamp_unit(t);
}

static psd89_fx16 psd89_blend_overlay(psd89_fx16 cb, psd89_fx16 cs)
{
    if (cb <= (PSD89_FX16_ONE >> 1)) {
        return psd89_blend_multiply(cb, (psd89_fx16_clamp_unit((psd89_fx16)(cs << 1))));
    }
    return psd89_blend_screen(cb, (psd89_fx16_clamp_unit((psd89_fx16)((cs << 1) - PSD89_FX16_ONE))));
}

static psd89_fx16 psd89_blend_exclusion(psd89_fx16 cb, psd89_fx16 cs)
{
    psd89_fx16 t;
    t = (psd89_fx16)(cb + cs - (psd89_fx16_mul((psd89_fx16)(cb << 1), cs)));
    return psd89_fx16_clamp_unit(t);
}

static psd89_fx16 psd89_blend_channel(const char key[4], psd89_fx16 cb, psd89_fx16 cs)
{
    if (psd89_key_eq(key, "mul ")) {
        return psd89_blend_multiply(cb, cs);
    }
    if (psd89_key_eq(key, "scrn")) {
        return psd89_blend_screen(cb, cs);
    }
    if (psd89_key_eq(key, "over")) {
        return psd89_blend_overlay(cb, cs);
    }
    if (psd89_key_eq(key, "dark")) {
        return psd89_fx16_min(cb, cs);
    }
    if (psd89_key_eq(key, "lite")) {
        return psd89_fx16_max(cb, cs);
    }
    if (psd89_key_eq(key, "diff")) {
        return psd89_fx16_absdiff(cb, cs);
    }
    if (psd89_key_eq(key, "smud")) {
        return psd89_blend_exclusion(cb, cs);
    }
    return cs;
}

static void psd89_fill_row(psd89_u8 *dst, psd89_u32 n, psd89_u8 v)
{
    memset(dst, (int)v, (size_t)n);
}

static int psd89_io_read_bytes(psd89_io *io, void *dst, psd89_u32 size)
{
    if (io == 0 || io->read == 0) {
        return 0;
    }
    return io->read(io->user, dst, size);
}

static int psd89_io_seek_abs(psd89_io *io, psd89_u32 off)
{
    if (io == 0 || io->seek == 0) {
        return 0;
    }
    return io->seek(io->user, off);
}

static int psd89_rd_u16_be(psd89_io *io, psd89_u16 *v)
{
    psd89_u8 b[2];
    if (!psd89_io_read_bytes(io, b, 2U)) {
        return 0;
    }
    *v = (psd89_u16)(((psd89_u16)b[0] << 8) | (psd89_u16)b[1]);
    return 1;
}

static int psd89_packbits_decode_row_local(psd89_io *io, psd89_u8 *dst, psd89_u32 row_bytes, psd89_u32 encoded_size)
{
    psd89_u32 out;
    psd89_u32 used;
    psd89_u8 ctrl;
    psd89_s8 sc;
    psd89_u32 count;
    psd89_u8 val;

    out = 0U;
    used = 0U;
    while (used < encoded_size && out < row_bytes) {
        if (!psd89_io_read_bytes(io, &ctrl, 1U)) {
            return 0;
        }
        ++used;
        sc = (psd89_s8)ctrl;
        if (sc >= 0) {
            count = (psd89_u32)sc + 1U;
            if (used + count > encoded_size || out + count > row_bytes) {
                return 0;
            }
            if (!psd89_io_read_bytes(io, dst + out, count)) {
                return 0;
            }
            used += count;
            out += count;
        } else if (sc >= -127) {
            count = (psd89_u32)(1 - sc);
            if (out + count > row_bytes) {
                return 0;
            }
            if (!psd89_io_read_bytes(io, &val, 1U)) {
                return 0;
            }
            ++used;
            while (count-- != 0U) {
                dst[out++] = val;
            }
        }
    }
    if (used != encoded_size || out != row_bytes) {
        return 0;
    }
    return 1;
}

static int psd89_compose_advance_rle(psd89_io *io, psd89_compose_channel_state *st, psd89_u32 target_row)
{
    psd89_u16 count16;
    while (st->current_row < target_row) {
        if (!psd89_io_seek_abs(io, st->counts_offset + (psd89_u32)st->current_row * 2U) ||
            !psd89_rd_u16_be(io, &count16)) {
            return 0;
        }
        st->data_pos += (psd89_u32)count16;
        ++st->current_row;
    }
    return 1;
}

static int psd89_compose_decode_row(psd89_io *io,
                                    psd89_compose_channel_state *st,
                                    psd89_u32 row_index,
                                    psd89_u8 *dst)
{
    psd89_u16 count16;

    if (!st->present) {
        psd89_fill_row(dst, st->cols, 0U);
        return 1;
    }
    if (row_index >= st->rows) {
        return 0;
    }
    if (st->from_memory) {
        memcpy(dst,
               st->mem_plane + (psd89_u32)row_index * st->mem_stride,
               (size_t)st->cols);
        return 1;
    }
    if (io == 0) {
        return 0;
    }
    if (st->compression == PSD89_COMP_RAW) {
        if (!psd89_io_seek_abs(io, st->raw_base + (psd89_u32)row_index * (psd89_u32)st->cols)) {
            return 0;
        }
        return psd89_io_read_bytes(io, dst, (psd89_u32)st->cols);
    }
    if (st->compression != PSD89_COMP_RLE) {
        return 0;
    }
    if (!psd89_compose_advance_rle(io, st, row_index)) {
        return 0;
    }
    if (!psd89_io_seek_abs(io, st->counts_offset + (psd89_u32)row_index * 2U) ||
        !psd89_rd_u16_be(io, &count16) ||
        !psd89_io_seek_abs(io, st->data_pos) ||
        !psd89_packbits_decode_row_local(io, dst, (psd89_u32)st->cols, (psd89_u32)count16)) {
        return 0;
    }
    st->data_pos += (psd89_u32)count16;
    st->current_row = row_index + 1U;
    return 1;
}

static int psd89_compose_init_channel_state(psd89_compose_channel_state *st,
                                            psd89_u32 rows,
                                            psd89_u32 cols,
                                            const psd89_layer_channel *ch,
                                            psd89_u32 start_row,
                                            psd89_io *io)
{
    psd89_u16 count16;
    psd89_u32 r;

    memset(st, 0, sizeof(*st));
    st->present = 1;
    st->cols = cols;
    st->rows = rows;
    st->compression = ch->compression;
    if (ch->plane != 0) {
        st->from_memory = 1;
        st->mem_plane = ch->plane;
        st->mem_stride = ch->stride ? ch->stride : cols;
        st->current_row = start_row;
        return 1;
    }
    if (io == 0) {
        return 0;
    }
    if (ch->compression == PSD89_COMP_RAW) {
        st->raw_base = ch->data_offset + 2U;
        st->current_row = start_row;
        return 1;
    }
    if (ch->compression != PSD89_COMP_RLE) {
        return 0;
    }
    st->counts_offset = ch->data_offset + 2U;
    st->data_pos = st->counts_offset + (psd89_u32)rows * 2U;
    st->current_row = 0U;
    for (r = 0U; r < start_row; ++r) {
        if (!psd89_io_seek_abs(io, st->counts_offset + (psd89_u32)r * 2U) ||
            !psd89_rd_u16_be(io, &count16)) {
            return 0;
        }
        st->data_pos += (psd89_u32)count16;
        st->current_row = r + 1U;
    }
    return 1;
}

static const psd89_layer_channel *psd89_find_layer_channel(const psd89_layer *layer, psd89_s16 channel_id)
{
    unsigned int i;
    for (i = 0U; i < layer->channel_count; ++i) {
        if (layer->channels[i].id == channel_id) {
            return &layer->channels[i];
        }
    }
    return 0;
}

static int psd89_doc_has_clipping_layers(const psd89_doc *doc)
{
    unsigned int i;

    if (doc == 0) {
        return 0;
    }
    for (i = 0U; i < doc->layer_count; ++i) {
        if (doc->layers[i].clipping != 0U) {
            return 1;
        }
    }
    return 0;
}

static int psd89_layer_blend_clipped_enabled(const psd89_layer *layer, int has_clipped_members)
{
    if (!has_clipped_members || layer == 0) {
        return 0;
    }
    if (layer->blend_clipped_present) {
        return layer->blend_clipped != 0U;
    }
    return 1;
}

static int psd89_layer_mask_global_enabled(const psd89_layer *layer)
{
    if (layer == 0 || !layer->user_mask.present) {
        return 0;
    }
    if (layer->layer_mask_global_present) {
        return layer->layer_mask_global != 0U;
    }
    return (layer->user_mask.flags & 0x01U) == 0U;
}

static int psd89_vector_mask_global_enabled(const psd89_layer *layer)
{
    if (layer == 0 || !layer->vector_mask.present) {
        return 0;
    }
    if (layer->vector_mask_global_present) {
        return layer->vector_mask_global != 0U;
    }
    return layer->vector_mask.not_link != 0U;
}

static const psd89_layer_channel *psd89_select_effective_mask_channel(const psd89_layer *layer,
                                                                      psd89_s32 *top,
                                                                      psd89_s32 *left,
                                                                      psd89_s32 *bottom,
                                                                      psd89_s32 *right,
                                                                      psd89_u8 *default_color,
                                                                      psd89_u8 *flags,
                                                                      psd89_u8 *apply_params)
{
    const psd89_layer_channel *ch;
    const psd89_layer_channel *real_ch;

    if (apply_params != 0) {
        *apply_params = 0U;
    }
    if (layer == 0) {
        return 0;
    }

    ch = psd89_find_layer_channel(layer, PSD89_CH_LAYER_MASK);
    real_ch = psd89_find_layer_channel(layer, PSD89_CH_REAL_LAYER_MASK);
    if (ch != 0) {
        if (!psd89_layer_mask_info_for_channel(layer, PSD89_CH_LAYER_MASK,
                                               top, left, bottom, right,
                                               default_color, flags)) {
            if (top != 0) *top = layer->top;
            if (left != 0) *left = layer->left;
            if (bottom != 0) *bottom = layer->bottom;
            if (right != 0) *right = layer->right;
            if (default_color != 0) *default_color = 255U;
            if (flags != 0) *flags = 0U;
        }
        if (apply_params != 0) {
            *apply_params = (psd89_u8)(!((layer->user_mask.flags & 0x08U) != 0U && real_ch != 0));
        }
        return ch;
    }

    ch = psd89_find_layer_channel(layer, PSD89_CH_REAL_LAYER_MASK);
    if (ch != 0 && psd89_layer_mask_info_for_channel(layer, PSD89_CH_REAL_LAYER_MASK,
                                                     top, left, bottom, right,
                                                     default_color, flags)) {
        if (apply_params != 0) {
            *apply_params = 1U;
        }
        return ch;
    }

    if (layer->user_mask.present) {
        if (!psd89_layer_mask_info_for_channel(layer, PSD89_CH_LAYER_MASK,
                                               top, left, bottom, right,
                                               default_color, flags)) {
            if (top != 0) *top = layer->top;
            if (left != 0) *left = layer->left;
            if (bottom != 0) *bottom = layer->bottom;
            if (right != 0) *right = layer->right;
            if (default_color != 0) *default_color = layer->user_mask.default_color;
            if (flags != 0) *flags = layer->user_mask.flags;
        }
        if (apply_params != 0) {
            *apply_params = 1U;
        }
    }
    return 0;
}

typedef struct psd89_comp_pair32 {
    psd89_u32 hi;
    psd89_u32 lo;
} psd89_comp_pair32;

static psd89_comp_pair32 psd89_comp_mul_u32_pair(psd89_u32 a, psd89_u32 b)
{
    psd89_u32 a0;
    psd89_u32 a1;
    psd89_u32 b0;
    psd89_u32 b1;
    psd89_u32 p0;
    psd89_u32 p1;
    psd89_u32 p2;
    psd89_u32 p3;
    psd89_u32 middle;
    psd89_comp_pair32 r;

    a0 = a & 0xFFFFU;
    a1 = a >> 16;
    b0 = b & 0xFFFFU;
    b1 = b >> 16;
    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;
    middle = (p0 >> 16) + (p1 & 0xFFFFU) + (p2 & 0xFFFFU);
    r.lo = (p0 & 0xFFFFU) | (middle << 16);
    r.hi = p3 + (p1 >> 16) + (p2 >> 16) + (middle >> 16);
    return r;
}

static psd89_u32 psd89_comp_abs_s32(psd89_s32 v)
{
    psd89_u32 u;
    u = (psd89_u32)v;
    if (v < 0) {
        u = (psd89_u32)(0U - u);
    }
    return u;
}

static psd89_u32 psd89_comp_pair_shr24(psd89_comp_pair32 p)
{
    return (p.hi << 8) | (p.lo >> 24);
}

static int psd89_comp_pair_lt_scale24(psd89_comp_pair32 p)
{
    return p.hi == 0U && p.lo < 0x01000000U;
}

static psd89_s32 psd89_fx24_scale_trunc(psd89_fx24 v, psd89_u32 scale)
{
    psd89_comp_pair32 p;
    psd89_u32 q;
    p = psd89_comp_mul_u32_pair(psd89_comp_abs_s32(v), scale);
    q = psd89_comp_pair_shr24(p);
    if (v < 0) {
        return (psd89_s32)(0U - q);
    }
    return (psd89_s32)q;
}

static psd89_s32 psd89_fx24_scale_plus_one_trunc(psd89_fx24 v, psd89_u32 scale)
{
    psd89_comp_pair32 p;
    psd89_u32 q;
    p = psd89_comp_mul_u32_pair(psd89_comp_abs_s32(v), scale);
    q = psd89_comp_pair_shr24(p);
    if (v >= 0) {
        return (psd89_s32)(q + 1U);
    }
    if (psd89_comp_pair_lt_scale24(p)) {
        return 0;
    }
    return (psd89_s32)(1 - (psd89_s32)q);
}

static void psd89_vector_mask_bounds_doc(const psd89_doc *doc,
                                         const psd89_vector_mask *vm,
                                         psd89_s32 *left,
                                         psd89_s32 *top,
                                         psd89_s32 *right,
                                         psd89_s32 *bottom)
{
    unsigned int i;
    int have;
    psd89_fx24 minx;
    psd89_fx24 miny;
    psd89_fx24 maxx;
    psd89_fx24 maxy;
    psd89_fx24 x;
    psd89_fx24 y;

    if (left != 0) *left = 0;
    if (top != 0) *top = 0;
    if (right != 0) *right = 0;
    if (bottom != 0) *bottom = 0;
    if (doc == 0 || vm == 0 || doc->width == 0U || doc->height == 0U || vm->knot_count == 0U) {
        return;
    }
    have = 0;
    minx = miny = maxx = maxy = 0;
    for (i = 0U; i < vm->knot_count; ++i) {
        x = vm->knots[i].anchor_h;
        y = vm->knots[i].anchor_v;
        if (!have) {
            minx = maxx = x;
            miny = maxy = y;
            have = 1;
        } else {
            if (x < minx) minx = x;
            if (x > maxx) maxx = x;
            if (y < miny) miny = y;
            if (y > maxy) maxy = y;
        }
    }
    if (!have) {
        return;
    }
    if (left != 0) *left = psd89_fx24_scale_trunc(minx, doc->width);
    if (top != 0) *top = psd89_fx24_scale_trunc(miny, doc->height);
    if (right != 0) *right = psd89_fx24_scale_plus_one_trunc(maxx, doc->width);
    if (bottom != 0) *bottom = psd89_fx24_scale_plus_one_trunc(maxy, doc->height);
}

static psd89_fx16 psd89_compose_vector_feather_fx(const psd89_compose_layer_state *st,
                                                  psd89_u32 doc_x,
                                                  psd89_u32 doc_y)
{
    psd89_s32 dist;
    psd89_s32 t;
    psd89_fx16 dfx;

    if (st == 0 || !st->vector_feather_present || st->vector_feather <= 0) {
        return PSD89_FX16_ONE;
    }
    if ((psd89_s32)doc_x < st->vector_left || (psd89_s32)doc_x >= st->vector_right ||
        (psd89_s32)doc_y < st->vector_top || (psd89_s32)doc_y >= st->vector_bottom) {
        return 0;
    }
    dist = (psd89_s32)doc_x - st->vector_left + 1;
    t = st->vector_right - (psd89_s32)doc_x;
    if (t < dist) dist = t;
    t = (psd89_s32)doc_y - st->vector_top + 1;
    if (t < dist) dist = t;
    t = st->vector_bottom - (psd89_s32)doc_y;
    if (t < dist) dist = t;
    if (dist <= 0) {
        return 0;
    }
    dfx = psd89_fx16_from_int((int)dist);
    if (dfx >= st->vector_feather) {
        return PSD89_FX16_ONE;
    }
    return psd89_fx16_clamp_unit(psd89_fx16_div(dfx, st->vector_feather));
}

static psd89_fx16 psd89_compose_vector_mask_sample_fx(const psd89_doc *doc,
                                                      const psd89_compose_layer_state *st,
                                                      psd89_u32 doc_x,
                                                      psd89_u32 doc_y)
{
    psd89_u8 cov;
    psd89_fx16 fx;

    if (doc == 0 || st == 0 || !st->has_vector_mask || st->vector_disabled || st->vector_mask == 0) {
        return PSD89_FX16_ONE;
    }

    if (st->vector_flat != 0) {
        cov = psd89_vector_coverage_u8(st->vector_mask,
                                       st->vector_flat,
                                       (psd89_s32)doc_x,
                                       (psd89_s32)doc_y);
    } else {
        cov = 0U;
    }

    fx = psd89_fx16_from_u8(cov);
    if (fx > 0 && st->vector_density_present) {
        fx = psd89_fx16_mul(fx, psd89_fx16_from_u8(st->vector_density));
    }
    if (fx > 0 && st->vector_feather_present) {
        fx = psd89_fx16_mul(fx, psd89_compose_vector_feather_fx(st, doc_x, doc_y));
    }
    return psd89_fx16_clamp_unit(fx);
}

static psd89_fx16 psd89_compose_mask_feather_fx(const psd89_compose_layer_state *st,
                                                psd89_u32 doc_x,
                                                psd89_u32 doc_y)
{
    psd89_s32 dist;
    psd89_s32 t;
    psd89_fx16 dfx;

    if (st == 0 || !st->mask_feather_present || st->mask_feather <= 0) {
        return PSD89_FX16_ONE;
    }
    if ((psd89_s32)doc_x < st->mask_left || (psd89_s32)doc_x >= st->mask_right ||
        (psd89_s32)doc_y < st->mask_top || (psd89_s32)doc_y >= st->mask_bottom) {
        return 0;
    }

    dist = (psd89_s32)doc_x - st->mask_left + 1;
    t = st->mask_right - (psd89_s32)doc_x;
    if (t < dist) {
        dist = t;
    }
    t = (psd89_s32)doc_y - st->mask_top + 1;
    if (t < dist) {
        dist = t;
    }
    t = st->mask_bottom - (psd89_s32)doc_y;
    if (t < dist) {
        dist = t;
    }
    if (dist <= 0) {
        return 0;
    }
    dfx = psd89_fx16_from_int((int)dist);
    if (dfx >= st->mask_feather) {
        return PSD89_FX16_ONE;
    }
    return psd89_fx16_clamp_unit(psd89_fx16_div(dfx, st->mask_feather));
}

static psd89_fx16 psd89_compose_raster_mask_sample_fx(const psd89_compose_layer_state *st,
                                                      psd89_u32 doc_x,
                                                      psd89_u32 doc_y,
                                                      int mask_row_valid,
                                                      const psd89_u8 *mask_row)
{
    psd89_u8 m;
    psd89_fx16 fx;

    if (st == 0) {
        return PSD89_FX16_ONE;
    }
    if (!st->has_mask || st->mask_disabled) {
        return PSD89_FX16_ONE;
    }

    m = st->mask_default_color;
    if ((psd89_s32)doc_x >= st->mask_left && (psd89_s32)doc_x < st->mask_right &&
        mask_row_valid && mask_row != 0) {
        m = mask_row[(psd89_u32)((psd89_s32)doc_x - st->mask_left)];
    }
    if (st->mask_invert) {
        m = (psd89_u8)(255U - m);
    }
    fx = psd89_fx16_from_u8(m);
    if (st->mask_apply_params && st->mask_density_present) {
        fx = psd89_fx16_mul(fx, psd89_fx16_from_u8(st->mask_density));
    }
    if (st->mask_apply_params && st->mask_feather_present) {
        fx = psd89_fx16_mul(fx, psd89_compose_mask_feather_fx(st, doc_x, doc_y));
    }
    return psd89_fx16_clamp_unit(fx);
}

static psd89_fx16 psd89_compose_shape_mask_sample_fx(const psd89_doc *doc,
                                                     const psd89_compose_layer_state *st,
                                                     psd89_u32 doc_x,
                                                     psd89_u32 doc_y,
                                                     int mask_row_valid,
                                                     const psd89_u8 *mask_row)
{
    psd89_fx16 fx;

    fx = PSD89_FX16_ONE;
    if (st == 0) {
        return fx;
    }
    if (st->has_mask && !st->layer_mask_global) {
        fx = psd89_fx16_mul(fx,
                            psd89_compose_raster_mask_sample_fx(st,
                                                                doc_x,
                                                                doc_y,
                                                                mask_row_valid,
                                                                mask_row));
    }
    if (st->has_vector_mask && !st->vector_mask_global) {
        fx = psd89_fx16_mul(fx,
                            psd89_compose_vector_mask_sample_fx(doc,
                                                                st,
                                                                doc_x,
                                                                doc_y));
    }
    return psd89_fx16_clamp_unit(fx);
}

static psd89_fx16 psd89_compose_final_mask_sample_fx(const psd89_doc *doc,
                                                     const psd89_compose_layer_state *st,
                                                     psd89_u32 doc_x,
                                                     psd89_u32 doc_y,
                                                     int mask_row_valid,
                                                     const psd89_u8 *mask_row)
{
    psd89_fx16 fx;

    fx = PSD89_FX16_ONE;
    if (st == 0) {
        return fx;
    }
    if (st->has_mask && st->layer_mask_global) {
        fx = psd89_fx16_mul(fx,
                            psd89_compose_raster_mask_sample_fx(st,
                                                                doc_x,
                                                                doc_y,
                                                                mask_row_valid,
                                                                mask_row));
    }
    if (st->has_vector_mask && st->vector_mask_global) {
        fx = psd89_fx16_mul(fx,
                            psd89_compose_vector_mask_sample_fx(doc,
                                                                st,
                                                                doc_x,
                                                                doc_y));
    }
    return psd89_fx16_clamp_unit(fx);
}

static void psd89_compose_blend_pixel(psd89_u32 color_channels,
                                      const char key[4],
                                      psd89_u8 **dst_rows,
                                      psd89_u8 *dst_alpha_row,
                                      psd89_u32 dx,
                                      psd89_u8 **src_rows,
                                      psd89_u32 sx,
                                      psd89_fx16 src_alpha,
                                      psd89_fx16 final_mask)
{
    psd89_fx16 ab;
    psd89_fx16 ao;
    psd89_fx16 one_minus_as;
    psd89_fx16 one_minus_ab;
    psd89_u32 c;
    psd89_fx16 dst_premul[PSD89_MAX_BASE_CHANNELS];
    psd89_fx16 cand_c[PSD89_MAX_BASE_CHANNELS];
    psd89_fx16 cand_premul[PSD89_MAX_BASE_CHANNELS];

    if (src_alpha <= 0) {
        return;
    }

    ab = psd89_fx16_from_u8(dst_alpha_row[dx]);
    one_minus_as = (psd89_fx16)(PSD89_FX16_ONE - src_alpha);
    one_minus_ab = (psd89_fx16)(PSD89_FX16_ONE - ab);
    ao = (psd89_fx16)(src_alpha + ab - psd89_fx16_mul(src_alpha, ab));

    for (c = 0U; c < color_channels; ++c) {
        psd89_fx16 cs;
        psd89_fx16 cb;
        psd89_fx16 blended;
        psd89_fx16 mixed;
        psd89_fx16 premul;
        psd89_fx16 co;

        cs = psd89_fx16_from_u8(src_rows[c][sx]);
        cb = psd89_fx16_from_u8(dst_rows[c][dx]);
        dst_premul[c] = psd89_fx16_mul(ab, cb);
        blended = psd89_blend_channel(key, cb, cs);
        mixed = (psd89_fx16)(psd89_fx16_mul(one_minus_ab, cs) + psd89_fx16_mul(ab, blended));
        premul = (psd89_fx16)(psd89_fx16_mul(src_alpha, mixed) +
                              psd89_fx16_mul(psd89_fx16_mul(one_minus_as, ab), cb));
        if (ao > 0) {
            co = psd89_fx16_div(premul, ao);
        } else {
            co = 0;
        }
        cand_c[c] = psd89_fx16_clamp_unit(co);
        cand_premul[c] = psd89_fx16_mul(ao, cand_c[c]);
    }

    if (final_mask >= PSD89_FX16_ONE) {
        for (c = 0U; c < color_channels; ++c) {
            dst_rows[c][dx] = psd89_fx16_to_u8_sat(cand_c[c]);
        }
        dst_alpha_row[dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(ao));
        return;
    }
    if (final_mask <= 0) {
        return;
    }

    {
        psd89_fx16 out_a;
        psd89_fx16 one_minus_m;

        one_minus_m = (psd89_fx16)(PSD89_FX16_ONE - final_mask);
        out_a = (psd89_fx16)(psd89_fx16_mul(one_minus_m, ab) + psd89_fx16_mul(final_mask, ao));
        for (c = 0U; c < color_channels; ++c) {
            psd89_fx16 out_premul;
            psd89_fx16 out_c;

            out_premul = (psd89_fx16)(psd89_fx16_mul(one_minus_m, dst_premul[c]) +
                                      psd89_fx16_mul(final_mask, cand_premul[c]));
            if (out_a > 0) {
                out_c = psd89_fx16_div(out_premul, out_a);
            } else {
                out_c = 0;
            }
            dst_rows[c][dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(out_c));
        }
        dst_alpha_row[dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(out_a));
    }
}

static void psd89_lrfx_solid_rgb(const psd89_lrfx_solid_fill *sf, psd89_u32 color_channels, psd89_u8 *rgb)
{
    unsigned int i;
    for (i = 0U; i < PSD89_MAX_BASE_CHANNELS; ++i) {
        rgb[i] = 0U;
    }
    if (sf == 0) {
        return;
    }
    if (color_channels == 1U) {
        rgb[0] = (psd89_u8)(sf->color[1] >> 8);
        return;
    }
    rgb[0] = (psd89_u8)(sf->color[1] >> 8);
    rgb[1] = (psd89_u8)(sf->color[2] >> 8);
    rgb[2] = (psd89_u8)(sf->color[3] >> 8);
}

static void psd89_apply_lrfx_solid_fill(const psd89_compose_layer_state *st,
                                        psd89_u32 color_channels,
                                        psd89_u8 **dst_rows,
                                        psd89_u8 *dst_alpha_row,
                                        psd89_u32 dx,
                                        psd89_fx16 shape_alpha,
                                        psd89_fx16 final_mask)
{
    psd89_u8 solid_px[PSD89_MAX_BASE_CHANNELS];
    psd89_u8 *src_rows[PSD89_MAX_BASE_CHANNELS];
    psd89_fx16 effect_alpha;
    if (st == 0 || !st->layer->lrfx.present || !st->layer->lrfx.solid_fill.present || !st->layer->lrfx.solid_fill.enabled) {
        return;
    }
    if (st->layer->lrfx.common_state_present && !st->layer->lrfx.common_visible) {
        return;
    }
    psd89_lrfx_solid_rgb(&st->layer->lrfx.solid_fill, color_channels, solid_px);
    src_rows[0] = &solid_px[0];
    src_rows[1] = color_channels > 1U ? &solid_px[1] : &solid_px[0];
    src_rows[2] = color_channels > 2U ? &solid_px[2] : &solid_px[0];
    effect_alpha = psd89_fx16_mul(shape_alpha, psd89_fx16_from_u8(st->layer->lrfx.solid_fill.opacity));
    if (effect_alpha <= 0) {
        return;
    }
    psd89_compose_blend_pixel(color_channels,
                              (st->layer->lrfx.solid_fill.blend_mode[0] != '\0' || st->layer->lrfx.solid_fill.blend_mode[1] != '\0' || st->layer->lrfx.solid_fill.blend_mode[2] != '\0' || st->layer->lrfx.solid_fill.blend_mode[3] != '\0') ? st->layer->lrfx.solid_fill.blend_mode : psd89_norm_key_const,
                              dst_rows,
                              dst_alpha_row,
                              dx,
                              src_rows,
                              0U,
                              effect_alpha,
                              final_mask);
}


static void psd89_lrfx_color_rgb(const psd89_u16 *color,
                                 psd89_u32 color_channels,
                                 psd89_u8 *rgb)
{
    unsigned int i;
    for (i = 0U; i < PSD89_MAX_BASE_CHANNELS; ++i) {
        rgb[i] = 0U;
    }
    if (color == 0) {
        return;
    }
    if (color_channels == 1U) {
        rgb[0] = (psd89_u8)(color[1] >> 8);
        return;
    }
    rgb[0] = (psd89_u8)(color[1] >> 8);
    rgb[1] = (psd89_u8)(color[2] >> 8);
    rgb[2] = (psd89_u8)(color[3] >> 8);
}

static void psd89_compose_blend_const_pixel(psd89_u32 color_channels,
                                            const char key[4],
                                            psd89_u8 **dst_rows,
                                            psd89_u8 *dst_alpha_row,
                                            psd89_u32 dx,
                                            const psd89_u8 *px,
                                            psd89_fx16 effect_alpha,
                                            psd89_fx16 final_mask)
{
    psd89_u8 *src_rows[PSD89_MAX_BASE_CHANNELS];
    src_rows[0] = (psd89_u8 *)&px[0];
    src_rows[1] = color_channels > 1U ? (psd89_u8 *)&px[1] : (psd89_u8 *)&px[0];
    src_rows[2] = color_channels > 2U ? (psd89_u8 *)&px[2] : (psd89_u8 *)&px[0];
    psd89_compose_blend_pixel(color_channels,
                              key,
                              dst_rows,
                              dst_alpha_row,
                              dx,
                              src_rows,
                              0U,
                              effect_alpha,
                              final_mask);
}

static psd89_u32 psd89_lrfx_radius_from_blur(psd89_s32 blur)
{
    if (blur <= 0) {
        return 1U;
    }
    if (blur > 32) {
        return 32U;
    }
    return (psd89_u32)blur;
}

static psd89_fx16 psd89_lrfx_strength_fx(psd89_u8 opacity, psd89_s32 intensity)
{
    psd89_fx16 fx;
    fx = psd89_fx16_from_u8(opacity);
    if (intensity > 0) {
        psd89_fx16 denom;
        psd89_fx16 numer;
        numer = psd89_fx16_from_int((int)(intensity > 100 ? 100 : intensity));
        denom = psd89_fx16_from_int(100);
        fx = psd89_fx16_mul(fx, psd89_fx16_div(numer, denom));
    }
    return psd89_fx16_clamp_unit(fx);
}

static psd89_s32 psd89_lrfx_offset_x(psd89_s32 angle, psd89_s32 distance)
{
    psd89_s32 a;
    if (distance == 0) {
        return 0;
    }
    a = angle % 360;
    if (a < 0) {
        a += 360;
    }
    if (a > 90 && a < 270) {
        return -distance;
    }
    return distance;
}

static psd89_fx16 psd89_lrfx_source_shape_fx(const psd89_doc *doc,
                                             const psd89_compose_layer_state *st,
                                             const psd89_u8 *src_alpha_row,
                                             psd89_s32 src_left,
                                             psd89_u32 src_width,
                                             psd89_u32 y,
                                             psd89_s32 doc_x,
                                             int mask_row_valid,
                                             const psd89_u8 *mask_row)
{
    psd89_u32 sx;
    psd89_fx16 src_cov;
    psd89_fx16 shape_mask;
    int tsly_false;

    if (doc == 0 || st == 0 || src_alpha_row == 0) {
        return 0;
    }
    if (doc_x < 0 || doc_x >= (psd89_s32)doc->width) {
        return 0;
    }
    if (doc_x < src_left || doc_x >= src_left + (psd89_s32)src_width) {
        return 0;
    }
    sx = (psd89_u32)(doc_x - src_left);
    src_cov = psd89_fx16_from_u8(src_alpha_row[sx]);
    shape_mask = psd89_compose_shape_mask_sample_fx(doc, st, (psd89_u32)doc_x, y, mask_row_valid, mask_row);
    if (shape_mask <= 0) {
        return 0;
    }
    tsly_false = st->layer->transparency_shapes_layer_present && !st->layer->transparency_shapes_layer;
    if (tsly_false) {
        return shape_mask;
    }
    return psd89_fx16_mul(src_cov, shape_mask);
}

static psd89_fx16 psd89_lrfx_target_mask_fx(const psd89_doc *doc,
                                            const psd89_compose_layer_state *st,
                                            psd89_u32 y,
                                            psd89_s32 doc_x,
                                            int mask_row_valid,
                                            const psd89_u8 *mask_row,
                                            const psd89_u8 *clip_mask_row,
                                            const int *clip_base_valid)
{
    psd89_fx16 fx;
    if (doc == 0 || st == 0 || doc_x < 0 || doc_x >= (psd89_s32)doc->width) {
        return 0;
    }
    fx = psd89_compose_final_mask_sample_fx(doc, st, (psd89_u32)doc_x, y, mask_row_valid, mask_row);
    if (st->layer->clipping != 0U) {
        if (clip_base_valid == 0 || !*clip_base_valid || clip_mask_row == 0) {
            return 0;
        }
        fx = psd89_fx16_mul(fx, psd89_fx16_from_u8(clip_mask_row[(psd89_u32)doc_x]));
    }
    return psd89_fx16_clamp_unit(fx);
}

static psd89_fx16 psd89_lrfx_nearby_shape_max_fx(const psd89_doc *doc,
                                                 const psd89_compose_layer_state *st,
                                                 const psd89_u8 *src_alpha_row,
                                                 psd89_s32 src_left,
                                                 psd89_u32 src_width,
                                                 psd89_u32 y,
                                                 psd89_s32 center_x,
                                                 psd89_u32 radius,
                                                 int mask_row_valid,
                                                 const psd89_u8 *mask_row)
{
    psd89_s32 sx;
    psd89_fx16 best;
    psd89_fx16 denom;

    if (radius == 0U) {
        return psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, center_x, mask_row_valid, mask_row);
    }
    best = 0;
    denom = psd89_fx16_from_int((int)(radius + 1U));
    for (sx = center_x - (psd89_s32)radius; sx <= center_x + (psd89_s32)radius; ++sx) {
        psd89_fx16 cov;
        psd89_fx16 weight;
        psd89_s32 dist;
        cov = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, sx, mask_row_valid, mask_row);
        if (cov <= 0) {
            continue;
        }
        dist = sx >= center_x ? (sx - center_x) : (center_x - sx);
        weight = psd89_fx16_div(psd89_fx16_from_int((int)(radius + 1U - (psd89_u32)dist)), denom);
        cov = psd89_fx16_mul(cov, weight);
        if (cov > best) {
            best = cov;
        }
    }
    return best;
}

static psd89_fx16 psd89_lrfx_inner_edge_fx(const psd89_doc *doc,
                                           const psd89_compose_layer_state *st,
                                           const psd89_u8 *src_alpha_row,
                                           psd89_s32 src_left,
                                           psd89_u32 src_width,
                                           psd89_u32 y,
                                           psd89_s32 doc_x,
                                           psd89_u32 radius,
                                           int mask_row_valid,
                                           const psd89_u8 *mask_row,
                                           int invert)
{
    psd89_fx16 shape_here;
    psd89_u32 dist;
    psd89_fx16 edge;

    shape_here = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, doc_x, mask_row_valid, mask_row);
    if (shape_here <= 0) {
        return 0;
    }
    if (radius == 0U) {
        radius = 1U;
    }
    edge = 0;
    for (dist = 1U; dist <= radius + 1U; ++dist) {
        if (psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, doc_x - (psd89_s32)dist, mask_row_valid, mask_row) <= 0 ||
            psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, doc_x + (psd89_s32)dist, mask_row_valid, mask_row) <= 0) {
            edge = psd89_fx16_div(psd89_fx16_from_int((int)(radius + 2U - dist)), psd89_fx16_from_int((int)(radius + 1U)));
            break;
        }
    }
    if (invert) {
        edge = (psd89_fx16)(PSD89_FX16_ONE - edge);
    }
    return psd89_fx16_mul(shape_here, psd89_fx16_clamp_unit(edge));
}

static int psd89_lrfx_effects_visible(const psd89_compose_layer_state *st)
{
    return !(st == 0 || !st->layer->lrfx.present || (st->layer->lrfx.common_state_present && !st->layer->lrfx.common_visible));
}

static int psd89_compose_decode_row_copy(psd89_io *io,
                                         const psd89_compose_channel_state *st,
                                         psd89_u32 row_index,
                                         psd89_u8 *dst)
{
    psd89_compose_channel_state tmp;

    if (st == 0 || dst == 0) {
        return 0;
    }
    tmp = *st;
    if (!tmp.present) {
        psd89_fill_row(dst, st->cols, 0U);
        return 1;
    }
    if (!tmp.from_memory && tmp.compression == PSD89_COMP_RLE) {
        tmp.current_row = 0U;
        tmp.data_pos = tmp.counts_offset + (psd89_u32)tmp.rows * 2U;
    }
    return psd89_compose_decode_row(io, &tmp, row_index, dst);
}

static psd89_s32 psd89_lrfx_offset_y(psd89_s32 angle, psd89_s32 distance)
{
    psd89_s32 a;
    if (distance == 0) {
        return 0;
    }
    a = angle % 360;
    if (a < 0) {
        a += 360;
    }
    if (a > 0 && a < 180) {
        return -distance;
    }
    if (a > 180 && a < 360) {
        return distance;
    }
    return 0;
}

static psd89_fx16 psd89_lrfx_kernel_weight_fx(psd89_u32 dist, psd89_u32 radius)
{
    if (radius == 0U) {
        return PSD89_FX16_ONE;
    }
    if (dist > radius) {
        return 0;
    }
    return psd89_fx16_div(psd89_fx16_from_int((int)(radius + 1U - dist)),
                          psd89_fx16_from_int((int)(radius + 1U)));
}

static psd89_fx16 psd89_lrfx_expand_core_fx(psd89_fx16 fx, psd89_s32 intensity, psd89_u32 radius)
{
    psd89_fx16 s;
    if (fx <= 0 || intensity <= 0 || radius <= 1U) {
        return psd89_fx16_clamp_unit(fx);
    }
    if (intensity > 100) {
        intensity = 100;
    }
    s = psd89_fx16_div(psd89_fx16_from_int((int)intensity), psd89_fx16_from_int(100));
    fx = (psd89_fx16)(fx + psd89_fx16_mul(psd89_fx16_mul(fx, (psd89_fx16)(PSD89_FX16_ONE - fx)), s));
    return psd89_fx16_clamp_unit(fx);
}

static int psd89_lrfx_prepare_sample_rows(const psd89_compose_layer_state *st,
                                          psd89_io *io,
                                          psd89_u32 current_y,
                                          psd89_u32 sample_y,
                                          const psd89_u8 *current_alpha_row,
                                          int current_mask_row_valid,
                                          const psd89_u8 *current_mask_row,
                                          psd89_u8 *alpha_temp,
                                          const psd89_u8 **alpha_row_out,
                                          int *mask_row_valid_out,
                                          psd89_u8 *mask_temp,
                                          const psd89_u8 **mask_row_out)
{
    psd89_u32 row_index;
    psd89_u32 mask_row_index;

    if (st == 0 || alpha_row_out == 0 || mask_row_valid_out == 0 || mask_row_out == 0) {
        return 0;
    }
    if ((psd89_s32)sample_y < st->layer->top || (psd89_s32)sample_y >= st->layer->bottom) {
        return 0;
    }

    if (sample_y == current_y && current_alpha_row != 0) {
        *alpha_row_out = current_alpha_row;
    } else {
        row_index = (psd89_u32)((psd89_s32)sample_y - st->layer->top);
        if (st->alpha.present) {
            if (!psd89_compose_decode_row_copy(io, &st->alpha, row_index, alpha_temp)) {
                return 0;
            }
        } else {
            psd89_fill_row(alpha_temp, st->cols, 255U);
        }
        *alpha_row_out = alpha_temp;
    }

    *mask_row_valid_out = 0;
    *mask_row_out = 0;
    if (st->has_mask && !st->mask_disabled && st->mask.present &&
        (psd89_s32)sample_y >= st->mask_top && (psd89_s32)sample_y < st->mask_bottom) {
        if (sample_y == current_y && current_mask_row_valid && current_mask_row != 0) {
            *mask_row_valid_out = 1;
            *mask_row_out = current_mask_row;
        } else {
            mask_row_index = (psd89_u32)((psd89_s32)sample_y - st->mask_top);
            if (!psd89_compose_decode_row_copy(io, &st->mask, mask_row_index, mask_temp)) {
                return 0;
            }
            *mask_row_valid_out = 1;
            *mask_row_out = mask_temp;
        }
    }
    return 1;
}

static psd89_fx16 psd89_lrfx_nearby_shape_max_2d_fx(const psd89_doc *doc,
                                                    psd89_io *io,
                                                    const psd89_compose_layer_state *st,
                                                    const psd89_u8 *current_alpha_row,
                                                    psd89_s32 src_left,
                                                    psd89_u32 src_width,
                                                    psd89_u32 y,
                                                    psd89_s32 center_x,
                                                    psd89_u32 radius,
                                                    int current_mask_row_valid,
                                                    const psd89_u8 *current_mask_row,
                                                    psd89_u8 *alpha_temp,
                                                    psd89_u8 *mask_temp)
{
    psd89_s32 sy;
    psd89_fx16 best;

    if (radius == 0U) {
        return psd89_lrfx_source_shape_fx(doc,
                                          st,
                                          current_alpha_row,
                                          src_left,
                                          src_width,
                                          y,
                                          center_x,
                                          current_mask_row_valid,
                                          current_mask_row);
    }
    best = 0;
    for (sy = (psd89_s32)y - (psd89_s32)radius; sy <= (psd89_s32)y + (psd89_s32)radius; ++sy) {
        psd89_s32 sx;
        psd89_u32 ady;
        psd89_fx16 wy;
        const psd89_u8 *alpha_row;
        const psd89_u8 *mask_row;
        int mask_row_valid;

        if (sy < 0 || sy >= (psd89_s32)doc->height) {
            continue;
        }
        if (!psd89_lrfx_prepare_sample_rows(st,
                                            io,
                                            y,
                                            (psd89_u32)sy,
                                            current_alpha_row,
                                            current_mask_row_valid,
                                            current_mask_row,
                                            alpha_temp,
                                            &alpha_row,
                                            &mask_row_valid,
                                            mask_temp,
                                            &mask_row)) {
            continue;
        }
        ady = (psd89_u32)(sy >= (psd89_s32)y ? (sy - (psd89_s32)y) : ((psd89_s32)y - sy));
        wy = psd89_lrfx_kernel_weight_fx(ady, radius);
        for (sx = center_x - (psd89_s32)radius; sx <= center_x + (psd89_s32)radius; ++sx) {
            psd89_u32 adx;
            psd89_fx16 wx;
            psd89_fx16 cov;

            adx = (psd89_u32)(sx >= center_x ? (sx - center_x) : (center_x - sx));
            wx = psd89_lrfx_kernel_weight_fx(adx, radius);
            cov = psd89_lrfx_source_shape_fx(doc,
                                             st,
                                             alpha_row,
                                             src_left,
                                             src_width,
                                             (psd89_u32)sy,
                                             sx,
                                             mask_row_valid,
                                             mask_row);
            if (cov <= 0) {
                continue;
            }
            cov = psd89_fx16_mul(cov, psd89_fx16_mul(wx, wy));
            if (cov > best) {
                best = cov;
            }
        }
    }
    return best;
}

static psd89_fx16 psd89_lrfx_inner_edge_2d_fx(const psd89_doc *doc,
                                              psd89_io *io,
                                              const psd89_compose_layer_state *st,
                                              const psd89_u8 *current_alpha_row,
                                              psd89_s32 src_left,
                                              psd89_u32 src_width,
                                              psd89_u32 y,
                                              psd89_s32 doc_x,
                                              psd89_u32 radius,
                                              int current_mask_row_valid,
                                              const psd89_u8 *current_mask_row,
                                              int invert,
                                              psd89_u8 *alpha_temp,
                                              psd89_u8 *mask_temp)
{
    psd89_fx16 shape_here;
    psd89_fx16 edge;
    psd89_s32 sy;
    psd89_u32 search_radius;

    shape_here = psd89_lrfx_source_shape_fx(doc,
                                            st,
                                            current_alpha_row,
                                            src_left,
                                            src_width,
                                            y,
                                            doc_x,
                                            current_mask_row_valid,
                                            current_mask_row);
    if (shape_here <= 0) {
        return 0;
    }
    search_radius = radius == 0U ? 1U : (radius + 1U);
    edge = 0;
    for (sy = (psd89_s32)y - (psd89_s32)search_radius; sy <= (psd89_s32)y + (psd89_s32)search_radius; ++sy) {
        psd89_s32 sx;
        psd89_u32 ady;
        psd89_fx16 wy;
        const psd89_u8 *alpha_row;
        const psd89_u8 *mask_row;
        int mask_row_valid;

        ady = (psd89_u32)(sy >= (psd89_s32)y ? (sy - (psd89_s32)y) : ((psd89_s32)y - sy));
        wy = psd89_lrfx_kernel_weight_fx(ady, search_radius);

        if (sy < 0 || sy >= (psd89_s32)doc->height ||
            (psd89_s32)sy < st->layer->top || (psd89_s32)sy >= st->layer->bottom) {
            if (wy > edge) {
                edge = wy;
            }
            continue;
        }
        if (!psd89_lrfx_prepare_sample_rows(st,
                                            io,
                                            y,
                                            (psd89_u32)sy,
                                            current_alpha_row,
                                            current_mask_row_valid,
                                            current_mask_row,
                                            alpha_temp,
                                            &alpha_row,
                                            &mask_row_valid,
                                            mask_temp,
                                            &mask_row)) {
            continue;
        }
        for (sx = doc_x - (psd89_s32)search_radius; sx <= doc_x + (psd89_s32)search_radius; ++sx) {
            psd89_u32 adx;
            psd89_fx16 wx;
            psd89_fx16 cand;
            psd89_fx16 cov;

            if (sx == doc_x && sy == (psd89_s32)y) {
                continue;
            }
            adx = (psd89_u32)(sx >= doc_x ? (sx - doc_x) : (doc_x - sx));
            wx = psd89_lrfx_kernel_weight_fx(adx, search_radius);
            cand = psd89_fx16_mul(wx, wy);
            cov = psd89_lrfx_source_shape_fx(doc,
                                             st,
                                             alpha_row,
                                             src_left,
                                             src_width,
                                             (psd89_u32)sy,
                                             sx,
                                             mask_row_valid,
                                             mask_row);
            if (cov <= 0 && cand > edge) {
                edge = cand;
            }
        }
    }
    if (invert) {
        edge = (psd89_fx16)(PSD89_FX16_ONE - edge);
    }
    return psd89_fx16_mul(shape_here, psd89_fx16_clamp_unit(edge));
}

static psd89_fx16 psd89_lrfx_bevel_sample_fx(const psd89_doc *doc,
                                             psd89_io *io,
                                             const psd89_compose_layer_state *st,
                                             const psd89_u8 *current_alpha_row,
                                             psd89_s32 src_left,
                                             psd89_u32 src_width,
                                             psd89_u32 current_y,
                                             psd89_s32 doc_x,
                                             psd89_u32 sample_y,
                                             int current_mask_row_valid,
                                             const psd89_u8 *current_mask_row,
                                             psd89_u8 *alpha_temp,
                                             psd89_u8 *mask_temp)
{
    const psd89_u8 *alpha_row;
    const psd89_u8 *mask_row;
    int mask_row_valid;

    if (sample_y == current_y) {
        return psd89_lrfx_source_shape_fx(doc,
                                          st,
                                          current_alpha_row,
                                          src_left,
                                          src_width,
                                          current_y,
                                          doc_x,
                                          current_mask_row_valid,
                                          current_mask_row);
    }
    if (sample_y >= doc->height) {
        return 0;
    }
    if (!psd89_lrfx_prepare_sample_rows(st,
                                        io,
                                        current_y,
                                        sample_y,
                                        current_alpha_row,
                                        current_mask_row_valid,
                                        current_mask_row,
                                        alpha_temp,
                                        &alpha_row,
                                        &mask_row_valid,
                                        mask_temp,
                                        &mask_row)) {
        return 0;
    }
    return psd89_lrfx_source_shape_fx(doc,
                                      st,
                                      alpha_row,
                                      src_left,
                                      src_width,
                                      sample_y,
                                      doc_x,
                                      mask_row_valid,
                                      mask_row);
}

static void psd89_apply_lrfx_under_effects_row(const psd89_doc *doc,
                                               psd89_io *io,
                                               const psd89_compose_layer_state *st,
                                               psd89_u32 color_channels,
                                               psd89_u32 y,
                                               const psd89_u8 *src_alpha_row,
                                               psd89_s32 src_left,
                                               psd89_u32 src_width,
                                               psd89_u8 **dst_rows,
                                               psd89_u8 *dst_alpha_row,
                                               int mask_row_valid,
                                               const psd89_u8 *mask_row,
                                               const psd89_u8 *clip_mask_row,
                                               const int *clip_base_valid)
{
    psd89_s32 start_x;
    psd89_s32 end_x;
    psd89_s32 x;
    psd89_u8 px[PSD89_MAX_BASE_CHANNELS];
    psd89_fx16 layer_opacity;
    psd89_u8 alpha_temp[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 mask_temp[PSD89_MAX_COMPOSE_ROW_BYTES];

    if (!psd89_lrfx_effects_visible(st) || doc == 0 || dst_rows == 0 || dst_alpha_row == 0 || src_alpha_row == 0) {
        return;
    }
    layer_opacity = psd89_fx16_from_u8(st->layer->opacity);
    start_x = src_left;
    end_x = src_left + (psd89_s32)src_width;

    if (st->layer->lrfx.drop_shadow.present && st->layer->lrfx.drop_shadow.enabled) {
        psd89_u32 radius = psd89_lrfx_radius_from_blur(st->layer->lrfx.drop_shadow.blur);
        psd89_s32 offx = psd89_lrfx_offset_x(st->layer->lrfx.drop_shadow.angle, st->layer->lrfx.drop_shadow.distance);
        psd89_s32 offy = psd89_lrfx_offset_y(st->layer->lrfx.drop_shadow.angle, st->layer->lrfx.drop_shadow.distance);
        psd89_fx16 strength = psd89_lrfx_strength_fx(st->layer->lrfx.drop_shadow.opacity, st->layer->lrfx.drop_shadow.intensity);
        const char *key = psd89_has_nonempty4(st->layer->lrfx.drop_shadow.blend_mode) ? st->layer->lrfx.drop_shadow.blend_mode : "mul ";
        psd89_lrfx_color_rgb(st->layer->lrfx.drop_shadow.color, color_channels, px);
        if (start_x - (psd89_s32)radius + offx < start_x) start_x = start_x - (psd89_s32)radius + offx;
        if (src_left + (psd89_s32)src_width + (psd89_s32)radius + offx > end_x) end_x = src_left + (psd89_s32)src_width + (psd89_s32)radius + offx;
        for (x = start_x; x < end_x; ++x) {
            psd89_fx16 base;
            psd89_fx16 mask;
            psd89_fx16 alpha;
            if (x < 0 || x >= (psd89_s32)doc->width) {
                continue;
            }
            base = psd89_lrfx_nearby_shape_max_2d_fx(doc,
                                                     io,
                                                     st,
                                                     src_alpha_row,
                                                     src_left,
                                                     src_width,
                                                     y,
                                                     x - offx,
                                                     radius,
                                                     mask_row_valid,
                                                     mask_row,
                                                     alpha_temp,
                                                     mask_temp);
            if (offy != 0) {
                psd89_s32 sy;
                sy = (psd89_s32)y - offy;
                if (sy < 0 || sy >= (psd89_s32)doc->height) {
                    base = 0;
                } else {
                    base = psd89_lrfx_nearby_shape_max_2d_fx(doc,
                                                             io,
                                                             st,
                                                             src_alpha_row,
                                                             src_left,
                                                             src_width,
                                                             (psd89_u32)sy,
                                                             x - offx,
                                                             radius,
                                                             mask_row_valid,
                                                             mask_row,
                                                             alpha_temp,
                                                             mask_temp);
                }
            }
            if (base <= 0) {
                continue;
            }
            base = psd89_lrfx_expand_core_fx(base, st->layer->lrfx.drop_shadow.intensity, radius);
            mask = psd89_lrfx_target_mask_fx(doc, st, y, x, mask_row_valid, mask_row, clip_mask_row, clip_base_valid);
            if (mask <= 0) {
                continue;
            }
            alpha = psd89_fx16_mul(layer_opacity, psd89_fx16_mul(strength, base));
            psd89_compose_blend_const_pixel(color_channels, key, dst_rows, dst_alpha_row, (psd89_u32)x, px, alpha, mask);
        }
    }

    start_x = src_left;
    end_x = src_left + (psd89_s32)src_width;
    if (st->layer->lrfx.outer_glow.present && st->layer->lrfx.outer_glow.enabled) {
        psd89_u32 radius = psd89_lrfx_radius_from_blur(st->layer->lrfx.outer_glow.blur);
        psd89_fx16 strength = psd89_lrfx_strength_fx(st->layer->lrfx.outer_glow.opacity, st->layer->lrfx.outer_glow.intensity);
        const char *key = psd89_has_nonempty4(st->layer->lrfx.outer_glow.blend_mode) ? st->layer->lrfx.outer_glow.blend_mode : "scrn";
        psd89_lrfx_color_rgb(st->layer->lrfx.outer_glow.color, color_channels, px);
        start_x -= (psd89_s32)radius;
        end_x += (psd89_s32)radius;
        for (x = start_x; x < end_x; ++x) {
            psd89_fx16 shape_here;
            psd89_fx16 prox;
            psd89_fx16 mask;
            psd89_fx16 alpha;
            if (x < 0 || x >= (psd89_s32)doc->width) {
                continue;
            }
            shape_here = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, x, mask_row_valid, mask_row);
            if (shape_here > 0) {
                continue;
            }
            prox = psd89_lrfx_nearby_shape_max_2d_fx(doc,
                                                     io,
                                                     st,
                                                     src_alpha_row,
                                                     src_left,
                                                     src_width,
                                                     y,
                                                     x,
                                                     radius,
                                                     mask_row_valid,
                                                     mask_row,
                                                     alpha_temp,
                                                     mask_temp);
            if (prox <= 0) {
                continue;
            }
            prox = psd89_lrfx_expand_core_fx(prox, st->layer->lrfx.outer_glow.intensity, radius);
            mask = psd89_lrfx_target_mask_fx(doc, st, y, x, mask_row_valid, mask_row, clip_mask_row, clip_base_valid);
            if (mask <= 0) {
                continue;
            }
            alpha = psd89_fx16_mul(layer_opacity, psd89_fx16_mul(strength, prox));
            psd89_compose_blend_const_pixel(color_channels, key, dst_rows, dst_alpha_row, (psd89_u32)x, px, alpha, mask);
        }
    }
}

static void psd89_apply_lrfx_over_effects_row(const psd89_doc *doc,
                                              psd89_io *io,
                                              const psd89_compose_layer_state *st,
                                              psd89_u32 color_channels,
                                              psd89_u32 y,
                                              const psd89_u8 *src_alpha_row,
                                              psd89_s32 src_left,
                                              psd89_u32 src_width,
                                              psd89_u8 **dst_rows,
                                              psd89_u8 *dst_alpha_row,
                                              int mask_row_valid,
                                              const psd89_u8 *mask_row,
                                              const psd89_u8 *clip_mask_row,
                                              const int *clip_base_valid)
{
    psd89_s32 x;
    psd89_s32 end_x;
    psd89_fx16 layer_opacity;
    psd89_u8 px[PSD89_MAX_BASE_CHANNELS];
    psd89_u8 alpha_temp[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 mask_temp[PSD89_MAX_COMPOSE_ROW_BYTES];

    if (!psd89_lrfx_effects_visible(st) || doc == 0 || dst_rows == 0 || dst_alpha_row == 0 || src_alpha_row == 0) {
        return;
    }
    layer_opacity = psd89_fx16_from_u8(st->layer->opacity);
    end_x = src_left + (psd89_s32)src_width;

    if (st->layer->lrfx.inner_shadow.present && st->layer->lrfx.inner_shadow.enabled) {
        psd89_u32 radius = psd89_lrfx_radius_from_blur(st->layer->lrfx.inner_shadow.blur);
        psd89_s32 offx = psd89_lrfx_offset_x(st->layer->lrfx.inner_shadow.angle, st->layer->lrfx.inner_shadow.distance);
        psd89_s32 offy = psd89_lrfx_offset_y(st->layer->lrfx.inner_shadow.angle, st->layer->lrfx.inner_shadow.distance);
        psd89_fx16 strength = psd89_lrfx_strength_fx(st->layer->lrfx.inner_shadow.opacity, st->layer->lrfx.inner_shadow.intensity);
        const char *key = psd89_has_nonempty4(st->layer->lrfx.inner_shadow.blend_mode) ? st->layer->lrfx.inner_shadow.blend_mode : "mul ";
        psd89_lrfx_color_rgb(st->layer->lrfx.inner_shadow.color, color_channels, px);
        for (x = src_left; x < end_x; ++x) {
            psd89_fx16 shape_here;
            psd89_fx16 shifted;
            psd89_fx16 edge;
            psd89_fx16 mask;
            psd89_fx16 alpha;
            psd89_s32 sy;
            shape_here = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, x, mask_row_valid, mask_row);
            if (shape_here <= 0) {
                continue;
            }
            sy = (psd89_s32)y - offy;
            if (sy < 0 || sy >= (psd89_s32)doc->height) {
                shifted = 0;
            } else if (st->rows > 1U) {
                shifted = psd89_lrfx_nearby_shape_max_2d_fx(doc,
                                                            io,
                                                            st,
                                                            src_alpha_row,
                                                            src_left,
                                                            src_width,
                                                            (psd89_u32)sy,
                                                            x - offx,
                                                            radius,
                                                            mask_row_valid,
                                                            mask_row,
                                                            alpha_temp,
                                                            mask_temp);
            } else {
                shifted = psd89_lrfx_nearby_shape_max_fx(doc, st, src_alpha_row, src_left, src_width, y, x - offx, radius, mask_row_valid, mask_row);
            }
            edge = psd89_fx16_mul(shape_here, (psd89_fx16)(PSD89_FX16_ONE - psd89_fx16_clamp_unit(shifted)));
            edge = psd89_lrfx_expand_core_fx(edge, st->layer->lrfx.inner_shadow.intensity, radius);
            if (edge <= 0) {
                continue;
            }
            mask = psd89_lrfx_target_mask_fx(doc, st, y, x, mask_row_valid, mask_row, clip_mask_row, clip_base_valid);
            if (mask <= 0) {
                continue;
            }
            alpha = psd89_fx16_mul(layer_opacity, psd89_fx16_mul(strength, edge));
            psd89_compose_blend_const_pixel(color_channels, key, dst_rows, dst_alpha_row, (psd89_u32)x, px, alpha, mask);
        }
    }

    if (st->layer->lrfx.inner_glow.present && st->layer->lrfx.inner_glow.enabled) {
        psd89_u32 radius = psd89_lrfx_radius_from_blur(st->layer->lrfx.inner_glow.blur);
        psd89_fx16 strength = psd89_lrfx_strength_fx(st->layer->lrfx.inner_glow.opacity, st->layer->lrfx.inner_glow.intensity);
        const char *key = psd89_has_nonempty4(st->layer->lrfx.inner_glow.blend_mode) ? st->layer->lrfx.inner_glow.blend_mode : "scrn";
        psd89_lrfx_color_rgb(st->layer->lrfx.inner_glow.color, color_channels, px);
        for (x = src_left; x < end_x; ++x) {
            psd89_fx16 edge;
            psd89_fx16 mask;
            psd89_fx16 alpha;
            if (st->rows > 1U) {
                edge = psd89_lrfx_inner_edge_2d_fx(doc,
                                                   io,
                                                   st,
                                                   src_alpha_row,
                                                   src_left,
                                                   src_width,
                                                   y,
                                                   x,
                                                   radius,
                                                   mask_row_valid,
                                                   mask_row,
                                                   st->layer->lrfx.inner_glow.invert != 0U,
                                                   alpha_temp,
                                                   mask_temp);
            } else {
                edge = psd89_lrfx_inner_edge_fx(doc, st, src_alpha_row, src_left, src_width, y, x, radius, mask_row_valid, mask_row, st->layer->lrfx.inner_glow.invert != 0U);
            }
            edge = psd89_lrfx_expand_core_fx(edge, st->layer->lrfx.inner_glow.intensity, radius);
            if (edge <= 0) {
                continue;
            }
            mask = psd89_lrfx_target_mask_fx(doc, st, y, x, mask_row_valid, mask_row, clip_mask_row, clip_base_valid);
            if (mask <= 0) {
                continue;
            }
            alpha = psd89_fx16_mul(layer_opacity, psd89_fx16_mul(strength, edge));
            psd89_compose_blend_const_pixel(color_channels, key, dst_rows, dst_alpha_row, (psd89_u32)x, px, alpha, mask);
        }
    }

    if (st->layer->lrfx.bevel.present && st->layer->lrfx.bevel.enabled) {
        psd89_s32 step = st->layer->lrfx.bevel.blur > 1 ? st->layer->lrfx.bevel.blur : 1;
        psd89_fx16 hilite_strength = psd89_fx16_mul(layer_opacity, psd89_fx16_from_u8(st->layer->lrfx.bevel.highlight_opacity));
        psd89_fx16 shadow_strength = psd89_fx16_mul(layer_opacity, psd89_fx16_from_u8(st->layer->lrfx.bevel.shadow_opacity));
        psd89_fx16 depth_scale = PSD89_FX16_ONE;
        psd89_s32 lx = -psd89_lrfx_offset_x(st->layer->lrfx.bevel.angle, 1);
        psd89_s32 ly = -psd89_lrfx_offset_y(st->layer->lrfx.bevel.angle, 1);
        if (st->layer->lrfx.bevel.strength > 0) {
            psd89_s32 s = st->layer->lrfx.bevel.strength > 100 ? 100 : st->layer->lrfx.bevel.strength;
            depth_scale = psd89_fx16_div(psd89_fx16_from_int((int)s), psd89_fx16_from_int(100));
        }
        if (lx == 0 && ly == 0) {
            lx = -1;
        }
        for (x = src_left; x < end_x; ++x) {
            psd89_fx16 shape_here;
            psd89_fx16 left_s;
            psd89_fx16 right_s;
            psd89_fx16 up_s;
            psd89_fx16 down_s;
            psd89_fx16 gx;
            psd89_fx16 gy;
            psd89_fx16 dot;
            psd89_fx16 mask;
            shape_here = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, x, mask_row_valid, mask_row);
            if (shape_here <= 0) {
                continue;
            }
            if (st->rows > 1U) {
                left_s = psd89_lrfx_bevel_sample_fx(doc, io, st, src_alpha_row, src_left, src_width, y, x - step, y, mask_row_valid, mask_row, alpha_temp, mask_temp);
                right_s = psd89_lrfx_bevel_sample_fx(doc, io, st, src_alpha_row, src_left, src_width, y, x + step, y, mask_row_valid, mask_row, alpha_temp, mask_temp);
                up_s = y >= (psd89_u32)step ? psd89_lrfx_bevel_sample_fx(doc, io, st, src_alpha_row, src_left, src_width, y, x, y - (psd89_u32)step, mask_row_valid, mask_row, alpha_temp, mask_temp) : 0;
                down_s = (y + (psd89_u32)step) < doc->height ? psd89_lrfx_bevel_sample_fx(doc, io, st, src_alpha_row, src_left, src_width, y, x, y + (psd89_u32)step, mask_row_valid, mask_row, alpha_temp, mask_temp) : 0;
                gx = (psd89_fx16)(left_s - right_s);
                gy = (psd89_fx16)(up_s - down_s);
                dot = 0;
                if (lx != 0) {
                    dot = (psd89_fx16)(dot + (lx > 0 ? gx : -gx));
                }
                if (ly != 0) {
                    dot = (psd89_fx16)(dot + (ly > 0 ? gy : -gy));
                }
            } else {
                psd89_s32 offx;
                psd89_fx16 near_l;
                psd89_fx16 near_r;
                offx = psd89_lrfx_offset_x(st->layer->lrfx.bevel.angle, 1);
                near_l = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, x - offx, mask_row_valid, mask_row);
                near_r = psd89_lrfx_source_shape_fx(doc, st, src_alpha_row, src_left, src_width, y, x + offx, mask_row_valid, mask_row);
                dot = (psd89_fx16)(near_r - near_l);
            }
            if (dot == 0) {
                continue;
            }
            mask = psd89_lrfx_target_mask_fx(doc, st, y, x, mask_row_valid, mask_row, clip_mask_row, clip_base_valid);
            if (mask <= 0) {
                continue;
            }
            if (dot > 0) {
                psd89_lrfx_color_rgb(st->layer->lrfx.bevel.highlight_color, color_channels, px);
                psd89_compose_blend_const_pixel(color_channels,
                                                psd89_has_nonempty4(st->layer->lrfx.bevel.highlight_blend_mode) ? st->layer->lrfx.bevel.highlight_blend_mode : "scrn",
                                                dst_rows,
                                                dst_alpha_row,
                                                (psd89_u32)x,
                                                px,
                                                psd89_fx16_mul(psd89_fx16_mul(hilite_strength, depth_scale), psd89_fx16_clamp_unit(dot)),
                                                mask);
            } else {
                psd89_lrfx_color_rgb(st->layer->lrfx.bevel.shadow_color, color_channels, px);
                psd89_compose_blend_const_pixel(color_channels,
                                                psd89_has_nonempty4(st->layer->lrfx.bevel.shadow_blend_mode) ? st->layer->lrfx.bevel.shadow_blend_mode : "mul ",
                                                dst_rows,
                                                dst_alpha_row,
                                                (psd89_u32)x,
                                                px,
                                                psd89_fx16_mul(psd89_fx16_mul(shadow_strength, depth_scale), psd89_fx16_clamp_unit((psd89_fx16)(-dot))),
                                                mask);
            }
        }
    }
}

static void psd89_replace_from_src_pixel(psd89_u32 color_channels,
                                         psd89_u8 **dst_rows,
                                         psd89_u8 *dst_alpha_row,
                                         psd89_u32 dx,
                                         psd89_u8 **src_rows,
                                         const psd89_u8 *src_alpha_row,
                                         psd89_u32 sx,
                                         psd89_fx16 mix)
{
    psd89_fx16 one_minus_mix;
    psd89_fx16 dst_a;
    psd89_fx16 src_a;
    psd89_fx16 out_a;
    psd89_u32 c;
    if (mix <= 0) {
        return;
    }
    if (mix >= PSD89_FX16_ONE) {
        for (c = 0U; c < color_channels; ++c) {
            dst_rows[c][dx] = src_rows[c][sx];
        }
        dst_alpha_row[dx] = src_alpha_row[sx];
        return;
    }
    one_minus_mix = (psd89_fx16)(PSD89_FX16_ONE - mix);
    dst_a = psd89_fx16_from_u8(dst_alpha_row[dx]);
    src_a = psd89_fx16_from_u8(src_alpha_row[sx]);
    out_a = (psd89_fx16)(psd89_fx16_mul(one_minus_mix, dst_a) + psd89_fx16_mul(mix, src_a));
    for (c = 0U; c < color_channels; ++c) {
        psd89_fx16 dv;
        psd89_fx16 sv;
        psd89_fx16 ov;
        dv = psd89_fx16_from_u8(dst_rows[c][dx]);
        sv = psd89_fx16_from_u8(src_rows[c][sx]);
        ov = (psd89_fx16)(psd89_fx16_mul(one_minus_mix, dv) + psd89_fx16_mul(mix, sv));
        dst_rows[c][dx] = psd89_fx16_to_u8_sat(ov);
    }
    dst_alpha_row[dx] = psd89_fx16_to_u8_sat(out_a);
}

static int psd89_render_leaf_row(const psd89_doc *doc,
                                 psd89_io *io,
                                 psd89_compose_layer_state *st,
                                 psd89_u32 color_channels,
                                 psd89_u32 y,
                                 psd89_u8 **dst_c_row,
                                 psd89_u8 *dst_a_row,
                                 psd89_u8 *clip_mask_row,
                                 int *clip_base_valid,
                                 psd89_u8 **scratch_color,
                                 psd89_u8 *scratch_alpha,
                                 psd89_u8 *mask_row)
{
    psd89_u32 layer_row;
    psd89_s32 draw_x0;
    psd89_s32 draw_x1;
    psd89_u32 src_x;
    psd89_u32 count;
    psd89_u32 x;
    psd89_fx16 opacity_fx;
    int hidden;
    psd89_u32 mask_row_index;
    int mask_row_valid;
    int tsly_false;
    psd89_u32 c;

    if (doc == 0 || st == 0 || !st->active) {
        return PSD89_OK;
    }
    hidden = psd89_compose_layer_is_hidden_compat(st->layer);
    if (hidden) {
        return PSD89_OK;
    }
    if ((psd89_s32)y < st->clip_top || (psd89_s32)y >= st->clip_bottom) {
        return PSD89_OK;
    }

    layer_row = (psd89_u32)((psd89_s32)y - st->layer->top);
    draw_x0 = st->clip_left;
    draw_x1 = st->clip_right;
    src_x = draw_x0 > st->layer->left ? (psd89_u32)(draw_x0 - st->layer->left) : 0U;
    count = (psd89_u32)(draw_x1 - draw_x0);
    if (count == 0U) {
        return PSD89_OK;
    }
    for (c = 0U; c < color_channels; ++c) {
        if (st->color[c].present) {
            if (!psd89_compose_decode_row(io, &st->color[c], layer_row, scratch_color[c])) {
                return PSD89_E_IO;
            }
        } else {
            psd89_fill_row(scratch_color[c], st->cols, 0U);
        }
    }
    if (st->alpha.present) {
        if (!psd89_compose_decode_row(io, &st->alpha, layer_row, scratch_alpha)) {
            return PSD89_E_IO;
        }
    } else {
        psd89_fill_row(scratch_alpha, st->cols, 255U);
    }
    opacity_fx = psd89_fx16_from_u8(st->layer->opacity);
    tsly_false = st->layer->transparency_shapes_layer_present && !st->layer->transparency_shapes_layer;
    mask_row_valid = 0;
    if (st->has_mask && !st->mask_disabled && st->mask.present &&
        (psd89_s32)y >= st->mask_top && (psd89_s32)y < st->mask_bottom) {
        mask_row_index = (psd89_u32)((psd89_s32)y - st->mask_top);
        if (!psd89_compose_decode_row(io, &st->mask, mask_row_index, mask_row)) {
            return PSD89_E_IO;
        }
        mask_row_valid = 1;
    }

    if (st->layer->clipping == 0U && clip_mask_row != 0 && clip_base_valid != 0) {
        for (x = 0U; x < count; ++x) {
            psd89_u32 dx;
            psd89_u32 sx;
            psd89_fx16 base_as;

            dx = (psd89_u32)draw_x0 + x;
            sx = src_x + x;
            base_as = psd89_fx16_mul(psd89_fx16_from_u8(scratch_alpha[sx]), opacity_fx);
            base_as = psd89_fx16_mul(base_as,
                                     psd89_compose_shape_mask_sample_fx(doc, st, dx, y, mask_row_valid, mask_row));
            clip_mask_row[dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(base_as));
        }
        *clip_base_valid = 1;
    } else if (st->layer->clipping != 0U && clip_base_valid != 0 && !*clip_base_valid) {
        return PSD89_OK;
    }

    psd89_apply_lrfx_under_effects_row(doc,
                                       io,
                                       st,
                                       color_channels,
                                       y,
                                       scratch_alpha,
                                       st->layer->left,
                                       st->cols,
                                       dst_c_row,
                                       dst_a_row,
                                       mask_row_valid,
                                       mask_row,
                                       clip_mask_row,
                                       clip_base_valid);

    for (x = 0U; x < count; ++x) {
        psd89_u32 dx;
        psd89_u32 sx;
        psd89_fx16 as;
        psd89_fx16 final_mask;
        psd89_fx16 shape_mask;
        psd89_fx16 src_cov;

        dx = (psd89_u32)draw_x0 + x;
        sx = src_x + x;
        src_cov = psd89_fx16_from_u8(scratch_alpha[sx]);
        shape_mask = psd89_compose_shape_mask_sample_fx(doc, st, dx, y, mask_row_valid, mask_row);
        if (tsly_false) {
            as = psd89_fx16_mul(opacity_fx, shape_mask);
        } else {
            as = psd89_fx16_mul(src_cov, opacity_fx);
            as = psd89_fx16_mul(as, shape_mask);
        }
        if (st->layer->clipping != 0U) {
            as = psd89_fx16_mul(as, psd89_fx16_from_u8(clip_mask_row[dx]));
        }
        if (as <= 0) {
            continue;
        }
        final_mask = psd89_compose_final_mask_sample_fx(doc, st, dx, y, mask_row_valid, mask_row);
        if (tsly_false) {
            final_mask = psd89_fx16_mul(final_mask, src_cov);
        }
        psd89_compose_blend_pixel(color_channels,
                                  st->layer->blend_mode,
                                  dst_c_row,
                                  dst_a_row,
                                  dx,
                                  scratch_color,
                                  sx,
                                  as,
                                  final_mask);
        psd89_apply_lrfx_solid_fill(st, color_channels, dst_c_row, dst_a_row, dx, as, final_mask);
    }
    psd89_apply_lrfx_over_effects_row(doc,
                                      io,
                                      st,
                                      color_channels,
                                      y,
                                      scratch_alpha,
                                      st->layer->left,
                                      st->cols,
                                      dst_c_row,
                                      dst_a_row,
                                      mask_row_valid,
                                      mask_row,
                                      clip_mask_row,
                                      clip_base_valid);
    return PSD89_OK;
}

static int psd89_render_buffer_row_as_item(const psd89_doc *doc,
                                           psd89_io *io,
                                           psd89_compose_layer_state *st,
                                           psd89_u32 color_channels,
                                           psd89_u32 y,
                                           psd89_u8 **src_color,
                                           psd89_u8 *src_alpha,
                                           psd89_u8 **dst_c_row,
                                           psd89_u8 *dst_a_row,
                                           psd89_u8 *clip_mask_row,
                                           int *clip_base_valid,
                                           psd89_u8 *mask_row)
{
    psd89_fx16 opacity_fx;
    psd89_u32 x;
    int mask_row_valid;
    psd89_u32 mask_row_index;
    psd89_fx16 shape_mask;
    psd89_fx16 final_mask;
    psd89_fx16 src_cov;
    psd89_fx16 as;
    int tsly_false;
    const char *blend_key;

    (void)io;
    if (doc == 0 || st == 0 || src_color == 0 || src_alpha == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (psd89_compose_layer_is_hidden_compat(st->layer)) {
        return PSD89_OK;
    }

    opacity_fx = psd89_fx16_from_u8(st->layer->opacity);
    tsly_false = st->layer->transparency_shapes_layer_present && !st->layer->transparency_shapes_layer;
    mask_row_valid = 0;
    if (st->has_mask && !st->mask_disabled && st->mask.present &&
        (psd89_s32)y >= st->mask_top && (psd89_s32)y < st->mask_bottom) {
        mask_row_index = (psd89_u32)((psd89_s32)y - st->mask_top);
        if (!psd89_compose_decode_row(io, &st->mask, mask_row_index, mask_row)) {
            return PSD89_E_IO;
        }
        mask_row_valid = 1;
    }
    blend_key = psd89_layer_group_blend_key(st->layer);
    if (psd89_key_eq(blend_key, "pass")) {
        blend_key = psd89_norm_key_const;
    }
    tsly_false = st->layer->transparency_shapes_layer_present && !st->layer->transparency_shapes_layer;

    psd89_apply_lrfx_under_effects_row(doc,
                                       io,
                                       st,
                                       color_channels,
                                       y,
                                       src_alpha,
                                       0,
                                       doc->width,
                                       dst_c_row,
                                       dst_a_row,
                                       mask_row_valid,
                                       mask_row,
                                       clip_mask_row,
                                       clip_base_valid);

    for (x = 0U; x < doc->width; ++x) {
        src_cov = psd89_fx16_from_u8(src_alpha[x]);
        if (src_cov <= 0) {
            continue;
        }
        shape_mask = psd89_compose_shape_mask_sample_fx(doc, st, x, y, mask_row_valid, mask_row);
        final_mask = psd89_compose_final_mask_sample_fx(doc, st, x, y, mask_row_valid, mask_row);
        if (tsly_false) {
            as = psd89_fx16_mul(opacity_fx, shape_mask);
            final_mask = psd89_fx16_mul(final_mask, src_cov);
        } else {
            as = psd89_fx16_mul(src_cov, opacity_fx);
            as = psd89_fx16_mul(as, shape_mask);
        }
        if (st->layer->clipping == 0U && clip_mask_row != 0 && clip_base_valid != 0) {
            clip_mask_row[x] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(psd89_fx16_mul(src_cov, opacity_fx)));
            *clip_base_valid = 1;
        } else if (st->layer->clipping != 0U) {
            if (clip_base_valid != 0 && !*clip_base_valid) {
                continue;
            }
            as = psd89_fx16_mul(as, psd89_fx16_from_u8(clip_mask_row[x]));
        }
        if (as <= 0) {
            continue;
        }
        if (st->layer->knockout_present && st->layer->knockout) {
            psd89_fx16 mix;
            mix = psd89_fx16_mul(as, final_mask);
            psd89_replace_from_src_pixel(color_channels,
                                         dst_c_row,
                                         dst_a_row,
                                         x,
                                         src_color,
                                         src_alpha,
                                         x,
                                         mix);
        } else {
            psd89_compose_blend_pixel(color_channels,
                                      blend_key,
                                      dst_c_row,
                                      dst_a_row,
                                      x,
                                      src_color,
                                      x,
                                      as,
                                      final_mask);
        }
        psd89_apply_lrfx_solid_fill(st, color_channels, dst_c_row, dst_a_row, x, as, final_mask);
    }
    psd89_apply_lrfx_over_effects_row(doc,
                                      io,
                                      st,
                                      color_channels,
                                      y,
                                      src_alpha,
                                      0,
                                      doc->width,
                                      dst_c_row,
                                      dst_a_row,
                                      mask_row_valid,
                                      mask_row,
                                      clip_mask_row,
                                      clip_base_valid);
    return PSD89_OK;
}

static int psd89_compose_span_grouped(const psd89_doc *doc,
                                      psd89_io *io,
                                      psd89_compose_layer_state *states,
                                      const int *start_to_end,
                                      const int *end_to_start,
                                      psd89_u32 color_channels,
                                      psd89_u32 y,
                                      int start,
                                      int end,
                                      psd89_u8 **dst_c_row,
                                      psd89_u8 *dst_a_row,
                                      psd89_u8 temp_color[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_BASE_CHANNELS][PSD89_MAX_COMPOSE_ROW_BYTES],
                                      psd89_u8 temp_alpha[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_COMPOSE_ROW_BYTES],
                                      psd89_u8 clip_masks[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_COMPOSE_ROW_BYTES],
                                      psd89_u8 **scratch_color,
                                      psd89_u8 *scratch_alpha,
                                      psd89_u8 *mask_row,
                                      unsigned int depth,
                                      const psd89_u8 *inherit_clip_mask_row,
                                      int inherit_clip_base_valid)
{
    psd89_u8 *clip_mask_row;
    int clip_base_valid;
    int li;
    int rc;

    if (depth >= PSD89_MAX_GROUP_DEPTH) {
        return PSD89_E_LIMIT;
    }
    clip_mask_row = clip_masks[depth];
    if (inherit_clip_mask_row != 0 && inherit_clip_base_valid) {
        memcpy(clip_mask_row, inherit_clip_mask_row, (size_t)doc->width);
        clip_base_valid = 1;
    } else {
        psd89_fill_row(clip_mask_row, doc->width, 0U);
        clip_base_valid = 0;
    }

    for (li = start; li < end; ) {
        if (start_to_end[li] >= 0 && start_to_end[li] < end) {
            int ge;
            psd89_compose_layer_state *gst;
            int hidden;
            int pass_through;
            const char *group_key;
            psd89_u8 *sub_c_row[PSD89_MAX_BASE_CHANNELS];
            psd89_u8 *sub_a_row;
            psd89_u32 c;

            ge = start_to_end[li];
            gst = &states[ge];
            hidden = psd89_compose_layer_is_hidden_compat(gst->layer);
            if (!hidden) {
                group_key = psd89_layer_group_blend_key(gst->layer);
                pass_through = psd89_key_eq(group_key, "pass") &&
                               gst->layer->opacity == 255U &&
                               gst->layer->clipping == 0U &&
                               !(gst->layer->knockout_present && gst->layer->knockout) &&
                               !(gst->layer->transparency_shapes_layer_present && !gst->layer->transparency_shapes_layer) &&
                               !(gst->layer->blend_interior_present && !gst->layer->blend_interior) &&
                               !gst->layer->lrfx.present;
                if (pass_through) {
                    rc = psd89_compose_span_grouped(doc, io, states, start_to_end, end_to_start,
                                                    color_channels, y, li + 1, ge, dst_c_row, dst_a_row,
                                                    temp_color, temp_alpha, clip_masks,
                                                    scratch_color, scratch_alpha, mask_row, depth + 1U,
                                                    clip_mask_row, clip_base_valid);
                    if (rc != PSD89_OK) {
                        return rc;
                    }
                } else {
                    for (c = 0U; c < color_channels; ++c) {
                        sub_c_row[c] = temp_color[depth][c];
                        if (gst->layer->knockout_present && gst->layer->knockout) {
                            memcpy(sub_c_row[c], dst_c_row[c], (size_t)doc->width);
                        } else {
                            psd89_fill_row(sub_c_row[c], doc->width, 0U);
                        }
                    }
                    sub_a_row = temp_alpha[depth];
                    if (gst->layer->knockout_present && gst->layer->knockout) {
                        memcpy(sub_a_row, dst_a_row, (size_t)doc->width);
                    } else {
                        psd89_fill_row(sub_a_row, doc->width, 0U);
                    }
                    rc = psd89_compose_span_grouped(doc, io, states, start_to_end, end_to_start,
                                                    color_channels, y, li + 1, ge, sub_c_row, sub_a_row,
                                                    temp_color, temp_alpha, clip_masks,
                                                    scratch_color, scratch_alpha, mask_row, depth + 1U,
                                                    0, 0);
                    if (rc != PSD89_OK) {
                        return rc;
                    }
                    rc = psd89_render_buffer_row_as_item(doc, io, gst, color_channels, y, sub_c_row, sub_a_row,
                                                         dst_c_row, dst_a_row, clip_mask_row, &clip_base_valid, mask_row);
                    if (rc != PSD89_OK) {
                        return rc;
                    }
                }
            }
            li = ge + 1;
            continue;
        }
        if (end_to_start[li] >= 0) {
            ++li;
            continue;
        }
        rc = psd89_render_leaf_row(doc, io, &states[li], color_channels, y, dst_c_row, dst_a_row,
                                   clip_mask_row, &clip_base_valid, scratch_color, scratch_alpha, mask_row);
        if (rc != PSD89_OK) {
            return rc;
        }
        ++li;
    }
    return PSD89_OK;
}

void psd89_compose_options_init(psd89_compose_options *opt)
{
    if (opt == 0) {
        return;
    }
    memset(opt, 0, sizeof(*opt));
    opt->clear_output = 1;
    opt->respect_hidden_layers = 1;
    opt->layer_order_bottom_to_top = 1;
}

psd89_u32 psd89_compose_max_row_bytes(const psd89_doc *doc)
{
    psd89_u32 max_row;
    unsigned int i;
    psd89_u32 cols;

    if (doc == 0) {
        return 0U;
    }
    max_row = doc->width;
    for (i = 0U; i < doc->layer_count; ++i) {
        if (doc->layers[i].right > doc->layers[i].left) {
            cols = (psd89_u32)(doc->layers[i].right - doc->layers[i].left);
            if (cols > max_row) {
                max_row = cols;
            }
        }
        if (doc->layers[i].user_mask.present && doc->layers[i].user_mask.right > doc->layers[i].user_mask.left) {
            cols = (psd89_u32)(doc->layers[i].user_mask.right - doc->layers[i].user_mask.left);
            if (cols > max_row) {
                max_row = cols;
            }
        }
        if (doc->layers[i].user_mask.real_present && doc->layers[i].user_mask.real_right > doc->layers[i].user_mask.real_left) {
            cols = (psd89_u32)(doc->layers[i].user_mask.real_right - doc->layers[i].user_mask.real_left);
            if (cols > max_row) {
                max_row = cols;
            }
        }
    }
    return max_row;
}

int psd89_compose_layers_u8(const psd89_doc *doc,
                            psd89_io *io,
                            psd89_u8 **dst_color,
                            psd89_u8 *dst_alpha,
                            psd89_u32 dst_stride,
                            psd89_u8 **scratch_color,
                            psd89_u8 *scratch_alpha,
                            psd89_u32 scratch_stride,
                            const psd89_compose_options *opt_in)
{
    psd89_compose_options opt_local;
    const psd89_compose_options *opt;
    psd89_compose_layer_state states[PSD89_MAX_LAYERS];
    psd89_vector_flatten_result vector_flats[PSD89_MAX_LAYERS];
    psd89_u8 vector_flat_valid[PSD89_MAX_LAYERS];
    psd89_u8 clip_mask_row[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 mask_row[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_alpha_row[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_color0[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_color1[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_color2[PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_temp_color[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_BASE_CHANNELS][PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_temp_alpha[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_COMPOSE_ROW_BYTES];
    psd89_u8 group_clip_mask[PSD89_MAX_GROUP_DEPTH][PSD89_MAX_COMPOSE_ROW_BYTES];
    int group_start_to_end[PSD89_MAX_LAYERS];
    int group_end_to_start[PSD89_MAX_LAYERS];
    int group_parent_end[PSD89_MAX_LAYERS];
    int has_groups;
    psd89_u8 *group_color[PSD89_MAX_BASE_CHANNELS];
    psd89_u8 *dst_c_row[PSD89_MAX_BASE_CHANNELS];
    psd89_u32 max_row;
    psd89_u32 color_channels;
    unsigned int i;
    unsigned int c;
    psd89_u32 y;
    int step;
    int start_index;
    int rc;
    int end_index;
    int li;
    int clip_base_valid;

    if (doc == 0 || dst_color == 0 || dst_alpha == 0 || scratch_color == 0 || scratch_alpha == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (doc->depth != 8U) {
        return PSD89_E_UNSUPPORTED_DEPTH;
    }
    if (doc->color_mode == PSD89_MODE_GRAYSCALE) {
        color_channels = 1U;
    } else if (doc->color_mode == PSD89_MODE_RGB) {
        color_channels = 3U;
    } else {
        return PSD89_E_UNSUPPORTED_MODE;
    }
    for (c = 0U; c < color_channels; ++c) {
        if (dst_color[c] == 0 || scratch_color[c] == 0) {
            return PSD89_E_BAD_ARGUMENT;
        }
    }
    if (doc->width > PSD89_MAX_COMPOSE_ROW_BYTES) {
        return PSD89_E_ROW_TOO_LARGE;
    }
    if (dst_stride < doc->width) {
        return PSD89_E_BAD_ARGUMENT;
    }
    max_row = psd89_compose_max_row_bytes(doc);
    if (scratch_stride < max_row) {
        return PSD89_E_ROW_TOO_LARGE;
    }

    if (opt_in == 0) {
        psd89_compose_options_init(&opt_local);
        opt = &opt_local;
    } else {
        opt = opt_in;
    }

    if (!opt->layer_order_bottom_to_top && psd89_doc_has_clipping_layers(doc)) {
        return PSD89_E_BAD_ARGUMENT;
    }

    group_color[0] = group_color0;
    group_color[1] = group_color1;
    group_color[2] = group_color2;

    if (opt->clear_output) {
        for (y = 0U; y < doc->height; ++y) {
            for (c = 0U; c < color_channels; ++c) {
                psd89_fill_row(dst_color[c] + (psd89_u32)y * dst_stride,
                               doc->width,
                               opt->backdrop_values[c]);
            }
            psd89_fill_row(dst_alpha + (psd89_u32)y * dst_stride,
                           doc->width,
                           opt->backdrop_alpha);
        }
    }

    memset(states, 0, sizeof(states));
    memset(vector_flat_valid, 0, sizeof(vector_flat_valid));
    for (i = 0U; i < doc->layer_count; ++i) {
        const psd89_layer *layer;
        psd89_u32 start_row;
        const psd89_layer_channel *ch;

        layer = &doc->layers[i];
        states[i].layer = layer;
        if (layer->right <= layer->left || layer->bottom <= layer->top) {
            continue;
        }
        states[i].rows = (psd89_u32)(layer->bottom - layer->top);
        states[i].cols = (psd89_u32)(layer->right - layer->left);
        states[i].clip_left = layer->left < 0 ? 0 : layer->left;
        states[i].clip_right = layer->right > (psd89_s32)doc->width ? (psd89_s32)doc->width : layer->right;
        states[i].clip_top = layer->top < 0 ? 0 : layer->top;
        states[i].clip_bottom = layer->bottom > (psd89_s32)doc->height ? (psd89_s32)doc->height : layer->bottom;
        if (states[i].clip_left >= states[i].clip_right || states[i].clip_top >= states[i].clip_bottom) {
            continue;
        }
        start_row = states[i].clip_top > layer->top ? (psd89_u32)(states[i].clip_top - layer->top) : 0U;
        for (c = 0U; c < color_channels; ++c) {
            ch = psd89_find_layer_channel(layer, (psd89_s16)c);
            if (ch != 0) {
                if (!psd89_compose_init_channel_state(&states[i].color[c], states[i].rows, states[i].cols, ch, start_row, io)) {
                    return ch->plane != 0 ? PSD89_E_BAD_STATE : PSD89_E_IO;
                }
            } else {
                states[i].color[c].cols = states[i].cols;
                states[i].color[c].rows = states[i].rows;
            }
        }
        ch = psd89_find_layer_channel(layer, PSD89_CH_ALPHA);
        if (ch != 0) {
            if (!psd89_compose_init_channel_state(&states[i].alpha, states[i].rows, states[i].cols, ch, start_row, io)) {
                return ch->plane != 0 ? PSD89_E_BAD_STATE : PSD89_E_IO;
            }
        } else {
            states[i].alpha.cols = states[i].cols;
            states[i].alpha.rows = states[i].rows;
        }
        {
            psd89_s32 mask_top;
            psd89_s32 mask_left;
            psd89_s32 mask_bottom;
            psd89_s32 mask_right;
            psd89_u8 mask_default;
            psd89_u8 mask_flags;
            psd89_u8 mask_apply_params;
            psd89_u32 mask_rows;
            psd89_u32 mask_cols;
            const psd89_layer_channel *mask_ch;

            mask_top = 0;
            mask_left = 0;
            mask_bottom = 0;
            mask_right = 0;
            mask_default = 255U;
            mask_flags = 0U;
            mask_apply_params = 0U;
            mask_ch = psd89_select_effective_mask_channel(layer,
                                                          &mask_top,
                                                          &mask_left,
                                                          &mask_bottom,
                                                          &mask_right,
                                                          &mask_default,
                                                          &mask_flags,
                                                          &mask_apply_params);
            if (mask_ch != 0 || layer->user_mask.present) {
                states[i].has_mask = 1U;
                states[i].mask_left = mask_left;
                states[i].mask_right = mask_right;
                states[i].mask_top = mask_top;
                states[i].mask_bottom = mask_bottom;
                states[i].mask_default_color = mask_default;
                states[i].mask_disabled = (psd89_u8)((mask_flags & 0x02U) != 0U);
                states[i].mask_invert = (psd89_u8)((mask_flags & 0x04U) != 0U);
                states[i].mask_apply_params = mask_apply_params;
                states[i].layer_mask_global = (psd89_u8)psd89_layer_mask_global_enabled(layer);
                if (mask_apply_params) {
                    states[i].mask_density_present = layer->user_mask.user_density_present;
                    states[i].mask_density = layer->user_mask.user_density;
                    states[i].mask_feather_present = layer->user_mask.user_feather_present;
                    states[i].mask_feather = layer->user_mask.user_feather;
                }
                if (mask_ch != 0 && mask_right > mask_left && mask_bottom > mask_top) {
                    mask_rows = (psd89_u32)(mask_bottom - mask_top);
                    mask_cols = (psd89_u32)(mask_right - mask_left);
                    start_row = states[i].mask_top < 0 ? (psd89_u32)(-states[i].mask_top) : 0U;
                    if (!psd89_compose_init_channel_state(&states[i].mask, mask_rows, mask_cols, mask_ch, start_row, io)) {
                        return mask_ch->plane != 0 ? PSD89_E_BAD_STATE : PSD89_E_IO;
                    }
                }
            }
            if (layer->vector_mask.present) {
                psd89_vector_flatten_options flat_opt;
                int frc;

                states[i].has_vector_mask = 1U;
                states[i].vector_disabled = layer->vector_mask.disabled;
                states[i].vector_mask_global = (psd89_u8)psd89_vector_mask_global_enabled(layer);
                states[i].vector_mask = &layer->vector_mask;
                states[i].vector_density_present = layer->user_mask.vector_density_present;
                states[i].vector_density = layer->user_mask.vector_density;
                states[i].vector_feather_present = layer->user_mask.vector_feather_present;
                states[i].vector_feather = layer->user_mask.vector_feather;
                psd89_vector_mask_bounds_doc(doc, &layer->vector_mask,
                                             &states[i].vector_left,
                                             &states[i].vector_top,
                                             &states[i].vector_right,
                                             &states[i].vector_bottom);
                psd89_vector_flatten_options_init(&flat_opt);
                flat_opt.flatness = (psd89_fx16)(1 << 15);
                frc = psd89_vector_flatten_mask(doc->width,
                                                doc->height,
                                                &layer->vector_mask,
                                                &flat_opt,
                                                &vector_flats[i]);
                if (frc != PSD89_OK) {
                    return frc;
                }
                vector_flat_valid[i] = 1U;
                states[i].vector_flat = &vector_flats[i];
            }
        }
        states[i].active = 1;
    }

    rc = psd89_build_group_index(doc, group_start_to_end, group_end_to_start, group_parent_end, &has_groups);
    if (rc != PSD89_OK) {
        return rc;
    }

    if (has_groups) {
        for (y = 0U; y < doc->height; ++y) {
            psd89_u8 *row_alpha;
            for (c = 0U; c < color_channels; ++c) {
                dst_c_row[c] = dst_color[c] + (psd89_u32)y * dst_stride;
            }
            row_alpha = dst_alpha + (psd89_u32)y * dst_stride;
            rc = psd89_compose_span_grouped(doc,
                                            io,
                                            states,
                                            group_start_to_end,
                                            group_end_to_start,
                                            color_channels,
                                            y,
                                            0,
                                            (int)doc->layer_count,
                                            dst_c_row,
                                            row_alpha,
                                            group_temp_color,
                                            group_temp_alpha,
                                            group_clip_mask,
                                            scratch_color,
                                            scratch_alpha,
                                            mask_row,
                                            0U,
                                            0,
                                            0);
            if (rc != PSD89_OK) {
                return rc;
            }
        }
        return PSD89_OK;
    }

    if (opt->layer_order_bottom_to_top) {
        start_index = 0;
        end_index = (int)doc->layer_count;
        step = 1;
    } else {
        start_index = (int)doc->layer_count - 1;
        end_index = -1;
        step = -1;
    }

    for (y = 0U; y < doc->height; ++y) {
        psd89_fill_row(clip_mask_row, doc->width, 0U);
        clip_base_valid = 0;

        for (li = start_index; li != end_index; ) {
            psd89_compose_layer_state *st;
            int advance;
            int group_has_clipped;
            int group_end;
            int probe;

            st = &states[li];
            advance = 1;
            if (!st->active) {
                li += step;
                continue;
            }

            group_has_clipped = 0;
            group_end = li;
            if (st->layer->clipping == 0U) {
                for (probe = li + step; probe != end_index; probe += step) {
                    if (doc->layers[probe].clipping == 0U) {
                        break;
                    }
                    group_has_clipped = 1;
                    group_end = probe;
                }
            }

            if (st->layer->clipping == 0U && psd89_layer_blend_clipped_enabled(st->layer, group_has_clipped)) {
                int gj;
                int clip_group_valid;
                psd89_u8 *dst_a_row;

                clip_group_valid = 0;
                psd89_fill_row(clip_mask_row, doc->width, 0U);
                for (c = 0U; c < color_channels; ++c) {
                    psd89_fill_row(group_color[c], doc->width, 0U);
                    dst_c_row[c] = dst_color[c] + (psd89_u32)y * dst_stride;
                }
                psd89_fill_row(group_alpha_row, doc->width, 0U);
                dst_a_row = dst_alpha + (psd89_u32)y * dst_stride;

                for (gj = li; ; gj += step) {
                    psd89_compose_layer_state *gst;
                    psd89_u32 layer_row;
                    psd89_s32 draw_x0;
                    psd89_s32 draw_x1;
                    psd89_u32 src_x;
                    psd89_u32 count;
                    psd89_u32 x;
                    psd89_fx16 opacity_fx;
                    int hidden;
                    psd89_u32 mask_row_index;
                    int mask_row_valid;
                    const char *blend_key;

                    gst = &states[gj];
                    if (gst->active) {
                        hidden = opt->respect_hidden_layers && psd89_compose_layer_is_hidden_compat(gst->layer);
                        if (!hidden && (psd89_s32)y >= gst->clip_top && (psd89_s32)y < gst->clip_bottom) {
                            layer_row = (psd89_u32)((psd89_s32)y - gst->layer->top);
                            draw_x0 = gst->clip_left;
                            draw_x1 = gst->clip_right;
                            src_x = draw_x0 > gst->layer->left ? (psd89_u32)(draw_x0 - gst->layer->left) : 0U;
                            count = (psd89_u32)(draw_x1 - draw_x0);
                            if (count != 0U) {
                                for (c = 0U; c < color_channels; ++c) {
                                    if (gst->color[c].present) {
                                        if (!psd89_compose_decode_row(io, &gst->color[c], layer_row, scratch_color[c])) {
                                            return PSD89_E_IO;
                                        }
                                    } else {
                                        psd89_fill_row(scratch_color[c], gst->cols, 0U);
                                    }
                                }
                                if (gst->alpha.present) {
                                    if (!psd89_compose_decode_row(io, &gst->alpha, layer_row, scratch_alpha)) {
                                        return PSD89_E_IO;
                                    }
                                } else {
                                    psd89_fill_row(scratch_alpha, gst->cols, 255U);
                                }
                                opacity_fx = psd89_fx16_from_u8(gst->layer->opacity);
                                mask_row_valid = 0;
                                if (gst->has_mask && !gst->mask_disabled && gst->mask.present &&
                                    (psd89_s32)y >= gst->mask_top && (psd89_s32)y < gst->mask_bottom) {
                                    mask_row_index = (psd89_u32)((psd89_s32)y - gst->mask_top);
                                    if (!psd89_compose_decode_row(io, &gst->mask, mask_row_index, mask_row)) {
                                        return PSD89_E_IO;
                                    }
                                    mask_row_valid = 1;
                                }
                                if (gst->layer->clipping == 0U) {
                                    for (x = 0U; x < count; ++x) {
                                        psd89_u32 dx;
                                        psd89_u32 sx;
                                        psd89_fx16 base_as;

                                        dx = (psd89_u32)draw_x0 + x;
                                        sx = src_x + x;
                                        base_as = psd89_fx16_mul(psd89_fx16_from_u8(scratch_alpha[sx]), opacity_fx);
                                        base_as = psd89_fx16_mul(base_as,
                                                                 psd89_compose_shape_mask_sample_fx(doc,
                                                                                                    gst,
                                                                                                    dx,
                                                                                                    y,
                                                                                                    mask_row_valid,
                                                                                                    mask_row));
                                        clip_mask_row[dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(base_as));
                                    }
                                    blend_key = psd89_norm_key_const;
                                    clip_group_valid = 1;
                                } else {
                                    blend_key = gst->layer->blend_mode;
                                }
                                psd89_apply_lrfx_under_effects_row(doc,
                                       io,
                                                                   gst,
                                                                   color_channels,
                                                                   y,
                                                                   scratch_alpha,
                                                                   gst->layer->left,
                                                                   gst->cols,
                                                                   group_color,
                                                                   group_alpha_row,
                                                                   mask_row_valid,
                                                                   mask_row,
                                                                   clip_mask_row,
                                                                   &clip_group_valid);
                                for (x = 0U; x < count; ++x) {
                                    psd89_u32 dx;
                                    psd89_u32 sx;
                                    psd89_fx16 as;
                                    psd89_fx16 final_mask;

                                    dx = (psd89_u32)draw_x0 + x;
                                    sx = src_x + x;
                                    as = psd89_fx16_mul(psd89_fx16_from_u8(scratch_alpha[sx]), opacity_fx);
                                    as = psd89_fx16_mul(as,
                                                        psd89_compose_shape_mask_sample_fx(doc,
                                                                                           gst,
                                                                                           dx,
                                                                                           y,
                                                                                           mask_row_valid,
                                                                                           mask_row));
                                    if (gst->layer->clipping != 0U) {
                                        as = psd89_fx16_mul(as, psd89_fx16_from_u8(clip_mask_row[dx]));
                                    }
                                    if (as <= 0) {
                                        continue;
                                    }
                                    final_mask = psd89_compose_final_mask_sample_fx(doc,
                                                                                     gst,
                                                                                     dx,
                                                                                     y,
                                                                                     mask_row_valid,
                                                                                     mask_row);
                                    psd89_compose_blend_pixel(color_channels,
                                                              blend_key,
                                                              group_color,
                                                              group_alpha_row,
                                                              dx,
                                                              scratch_color,
                                                              sx,
                                                              as,
                                                              final_mask);
                                    psd89_apply_lrfx_solid_fill(gst, color_channels, group_color, group_alpha_row, dx, as, final_mask);
                                }
                                psd89_apply_lrfx_over_effects_row(doc,
                                      io,
                                                                  gst,
                                                                  color_channels,
                                                                  y,
                                                                  scratch_alpha,
                                                                  gst->layer->left,
                                                                  gst->cols,
                                                                  group_color,
                                                                  group_alpha_row,
                                                                  mask_row_valid,
                                                                  mask_row,
                                                                  clip_mask_row,
                                                                  &clip_group_valid);
                            }
                        }
                    }
                    if (gj == group_end) {
                        break;
                    }
                }

                for (c = 0U; c < color_channels; ++c) {
                    dst_c_row[c] = dst_color[c] + (psd89_u32)y * dst_stride;
                }
                for (probe = 0; probe < (int)doc->width; ++probe) {
                    psd89_fx16 as;

                    as = psd89_fx16_from_u8(group_alpha_row[probe]);
                    if (as <= 0) {
                        continue;
                    }
                    psd89_compose_blend_pixel(color_channels,
                                              st->layer->blend_mode,
                                              dst_c_row,
                                              dst_a_row,
                                              (psd89_u32)probe,
                                              group_color,
                                              (psd89_u32)probe,
                                              as,
                                              PSD89_FX16_ONE);
                }

                clip_base_valid = 0;
                li = group_end + step;
                continue;
            }

            if (st->layer->clipping == 0U) {
                psd89_fill_row(clip_mask_row, doc->width, 0U);
                clip_base_valid = 1;
            } else if (!clip_base_valid) {
                li += step;
                continue;
            }

            {
                psd89_u32 layer_row;
                psd89_s32 draw_x0;
                psd89_s32 draw_x1;
                psd89_u32 src_x;
                psd89_u32 count;
                psd89_u32 x;
                psd89_fx16 opacity_fx;
                int hidden;
                psd89_u32 mask_row_index;
                int mask_row_valid;
                int tsly_false;
                psd89_u8 *dst_a_row;

                hidden = opt->respect_hidden_layers && psd89_compose_layer_is_hidden_compat(st->layer);
                if ((psd89_s32)y < st->clip_top || (psd89_s32)y >= st->clip_bottom || hidden) {
                    li += step;
                    continue;
                }

                layer_row = (psd89_u32)((psd89_s32)y - st->layer->top);
                draw_x0 = st->clip_left;
                draw_x1 = st->clip_right;
                src_x = draw_x0 > st->layer->left ? (psd89_u32)(draw_x0 - st->layer->left) : 0U;
                count = (psd89_u32)(draw_x1 - draw_x0);
                if (count == 0U) {
                    li += step;
                    continue;
                }
                for (c = 0U; c < color_channels; ++c) {
                    if (st->color[c].present) {
                        if (!psd89_compose_decode_row(io, &st->color[c], layer_row, scratch_color[c])) {
                            return PSD89_E_IO;
                        }
                    } else {
                        psd89_fill_row(scratch_color[c], st->cols, 0U);
                    }
                    dst_c_row[c] = dst_color[c] + (psd89_u32)y * dst_stride;
                }
                if (st->alpha.present) {
                    if (!psd89_compose_decode_row(io, &st->alpha, layer_row, scratch_alpha)) {
                        return PSD89_E_IO;
                    }
                } else {
                    psd89_fill_row(scratch_alpha, st->cols, 255U);
                }
                dst_a_row = dst_alpha + (psd89_u32)y * dst_stride;
                opacity_fx = psd89_fx16_from_u8(st->layer->opacity);
                tsly_false = st->layer->transparency_shapes_layer_present && !st->layer->transparency_shapes_layer;
                mask_row_valid = 0;
                if (st->has_mask && !st->mask_disabled && st->mask.present &&
                    (psd89_s32)y >= st->mask_top && (psd89_s32)y < st->mask_bottom) {
                    mask_row_index = (psd89_u32)((psd89_s32)y - st->mask_top);
                    if (!psd89_compose_decode_row(io, &st->mask, mask_row_index, mask_row)) {
                        return PSD89_E_IO;
                    }
                    mask_row_valid = 1;
                }

                if (st->layer->clipping == 0U) {
                    for (x = 0U; x < count; ++x) {
                        psd89_u32 dx;
                        psd89_u32 sx;
                        psd89_fx16 base_as;
                        psd89_fx16 src_cov;
                        psd89_fx16 shape_mask;

                        dx = (psd89_u32)draw_x0 + x;
                        sx = src_x + x;
                        src_cov = psd89_fx16_from_u8(scratch_alpha[sx]);
                        shape_mask = psd89_compose_shape_mask_sample_fx(doc,
                                                                        st,
                                                                        dx,
                                                                        y,
                                                                        mask_row_valid,
                                                                        mask_row);
                        if (tsly_false) {
                            base_as = psd89_fx16_mul(opacity_fx, shape_mask);
                        } else {
                            base_as = psd89_fx16_mul(src_cov, opacity_fx);
                            base_as = psd89_fx16_mul(base_as, shape_mask);
                        }
                        clip_mask_row[dx] = psd89_fx16_to_u8_sat(psd89_fx16_clamp_unit(base_as));
                    }
                }

                psd89_apply_lrfx_under_effects_row(doc,
                                       io,
                                                   st,
                                                   color_channels,
                                                   y,
                                                   scratch_alpha,
                                                   st->layer->left,
                                                   st->cols,
                                                   dst_c_row,
                                                   dst_a_row,
                                                   mask_row_valid,
                                                   mask_row,
                                                   clip_mask_row,
                                                   &clip_base_valid);

                for (x = 0U; x < count; ++x) {
                    psd89_u32 dx;
                    psd89_u32 sx;
                    psd89_fx16 as;
                    psd89_fx16 final_mask;
                    psd89_fx16 src_cov;
                    psd89_fx16 shape_mask;

                    dx = (psd89_u32)draw_x0 + x;
                    sx = src_x + x;
                    src_cov = psd89_fx16_from_u8(scratch_alpha[sx]);
                    shape_mask = psd89_compose_shape_mask_sample_fx(doc,
                                                                       st,
                                                                       dx,
                                                                       y,
                                                                       mask_row_valid,
                                                                       mask_row);
                    if (tsly_false) {
                        as = psd89_fx16_mul(opacity_fx, shape_mask);
                    } else {
                        as = psd89_fx16_mul(src_cov, opacity_fx);
                        as = psd89_fx16_mul(as, shape_mask);
                    }
                    if (st->layer->clipping != 0U) {
                        as = psd89_fx16_mul(as, psd89_fx16_from_u8(clip_mask_row[dx]));
                    }
                    if (as <= 0) {
                        continue;
                    }
                    final_mask = psd89_compose_final_mask_sample_fx(doc,
                                                                     st,
                                                                     dx,
                                                                     y,
                                                                     mask_row_valid,
                                                                     mask_row);
                    if (tsly_false) {
                        final_mask = psd89_fx16_mul(final_mask, src_cov);
                    }
                    psd89_compose_blend_pixel(color_channels,
                                              st->layer->blend_mode,
                                              dst_c_row,
                                              dst_a_row,
                                              dx,
                                              scratch_color,
                                              sx,
                                              as,
                                              final_mask);
                    psd89_apply_lrfx_solid_fill(st, color_channels, dst_c_row, dst_a_row, dx, as, final_mask);
                }
                psd89_apply_lrfx_over_effects_row(doc,
                                      io,
                                                  st,
                                                  color_channels,
                                                  y,
                                                  scratch_alpha,
                                                  st->layer->left,
                                                  st->cols,
                                                  dst_c_row,
                                                  dst_a_row,
                                                  mask_row_valid,
                                                  mask_row,
                                                  clip_mask_row,
                                                  &clip_base_valid);
            }

            if (advance) {
                li += step;
            }
        }
    }

    return PSD89_OK;
}

