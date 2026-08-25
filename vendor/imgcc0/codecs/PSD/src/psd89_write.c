#include "psd89_internal.h"

#include <string.h>

static int psd89_write_zeroes(psd89_io *io, psd89_u32 n)
{
    static const psd89_u8 zeros[256] = { 0 };
    psd89_u32 chunk;
    while (n != 0U) {
        chunk = n > (psd89_u32)sizeof(zeros) ? (psd89_u32)sizeof(zeros) : n;
        if (!psd89_io_write(io, zeros, chunk)) {
            return 0;
        }
        n -= chunk;
    }
    return 1;
}

static int psd89_has_nonempty4(const char key[4])
{
    return key[0] != '\0' || key[1] != '\0' || key[2] != '\0' || key[3] != '\0';
}

static psd89_u16 psd89_doc_base_channels(const psd89_doc *doc)
{
    return (psd89_u16)(doc->color_mode == PSD89_MODE_GRAYSCALE ? 1U : 3U);
}

static int psd89_tag_key_eq(const char key[4], const char *lit4)
{
    return key[0] == lit4[0] && key[1] == lit4[1] && key[2] == lit4[2] && key[3] == lit4[3];
}

static int psd89_should_skip_raw_layer_tag(const psd89_layer *layer, const psd89_tag_ref *tag)
{
    if (layer == 0 || tag == 0) {
        return 0;
    }
    if (layer->blend_clipped_present && psd89_tag_key_eq(tag->key, "clbl")) {
        return 1;
    }
    if (layer->blend_interior_present && psd89_tag_key_eq(tag->key, "infx")) {
        return 1;
    }
    if (layer->knockout_present && psd89_tag_key_eq(tag->key, "knko")) {
        return 1;
    }
    if (layer->layer_mask_global_present && psd89_tag_key_eq(tag->key, "lmgm")) {
        return 1;
    }
    if (layer->vector_mask_global_present && psd89_tag_key_eq(tag->key, "vmgm")) {
        return 1;
    }
    if (layer->transparency_shapes_layer_present && psd89_tag_key_eq(tag->key, "tsly")) {
        return 1;
    }
    if (layer->section_divider_present && psd89_tag_key_eq(tag->key, "lsct")) {
        return 1;
    }
    if (layer->lrfx.present && psd89_tag_key_eq(tag->key, "lrFX")) {
        return 1;
    }
    if (layer->type_tool.present && psd89_tag_key_eq(tag->key, "TySh")) {
        return 1;
    }
    if (layer->text_engine.present && layer->text_engine.authored_raw != 0 && layer->text_engine.authored_raw_size != 0U && psd89_tag_key_eq(tag->key, "Txt2")) {
        return 1;
    }
    if (layer->object_effects.present && psd89_tag_key_eq(tag->key, "lfx2")) {
        return 1;
    }
    if (layer->smart_object.present && (psd89_tag_key_eq(tag->key, "SoLd") || psd89_tag_key_eq(tag->key, "SoLE") || psd89_tag_key_eq(tag->key, "plLd"))) {
        return 1;
    }
    return 0;
}

static int psd89_layer_has_authored_mask(const psd89_layer *layer)
{
    return layer != 0 && layer->mask_ref.raw_size == 0U && layer->user_mask.present;
}

static int psd89_layer_has_raw_vector_tag(const psd89_layer *layer)
{
    unsigned int i;
    if (layer == 0) {
        return 0;
    }
    for (i = 0U; i < layer->tag_count; ++i) {
        if (psd89_tag_key_eq(layer->tags[i].key, "vmsk") || psd89_tag_key_eq(layer->tags[i].key, "vsms")) {
            return 1;
        }
    }
    return 0;
}

static int psd89_layer_has_authored_vector_mask(const psd89_layer *layer)
{
    return layer != 0 && layer->vector_mask.present && !psd89_layer_has_raw_vector_tag(layer);
}

static psd89_u8 psd89_layer_mask_param_flags(const psd89_layer_user_mask *mask)
{
    psd89_u8 flags;

    if (mask == 0) {
        return 0U;
    }
    flags = 0U;
    if (mask->user_density_present) {
        flags |= 0x01U;
    }
    if (mask->user_feather_present) {
        flags |= 0x02U;
    }
    if (mask->vector_density_present) {
        flags |= 0x04U;
    }
    if (mask->vector_feather_present) {
        flags |= 0x08U;
    }
    return flags;
}

static psd89_u32 psd89_layer_mask_param_len(const psd89_layer_user_mask *mask)
{
    psd89_u8 param_flags;
    psd89_u32 len;

    param_flags = psd89_layer_mask_param_flags(mask);
    if (param_flags == 0U) {
        return 0U;
    }
    len = 1U;
    if ((param_flags & 0x01U) != 0U) {
        ++len;
    }
    if ((param_flags & 0x02U) != 0U) {
        len += 8U;
    }
    if ((param_flags & 0x04U) != 0U) {
        ++len;
    }
    if ((param_flags & 0x08U) != 0U) {
        len += 8U;
    }
    return len;
}

static int psd89_write_layer_mask_params(psd89_io *io, const psd89_layer_user_mask *mask)
{
    psd89_u8 param_flags;

    param_flags = psd89_layer_mask_param_flags(mask);
    if (param_flags == 0U) {
        return 1;
    }
    if (!psd89_wr_u8(io, param_flags)) {
        return 0;
    }
    if ((param_flags & 0x01U) != 0U && !psd89_wr_u8(io, mask->user_density)) {
        return 0;
    }
    if ((param_flags & 0x02U) != 0U && !psd89_wr_ieee8_from_q16(io, mask->user_feather)) {
        return 0;
    }
    if ((param_flags & 0x04U) != 0U && !psd89_wr_u8(io, mask->vector_density)) {
        return 0;
    }
    if ((param_flags & 0x08U) != 0U && !psd89_wr_ieee8_from_q16(io, mask->vector_feather)) {
        return 0;
    }
    return 1;
}

static int psd89_write_layer_mask_block(psd89_io *io, const psd89_layer *layer)
{
    psd89_u32 size;
    psd89_u32 param_len;
    psd89_u8 flags;
    psd89_u8 real_flags;
    psd89_u8 real_background;
    psd89_s32 real_top;
    psd89_s32 real_left;
    psd89_s32 real_bottom;
    psd89_s32 real_right;
    int emit_real;

    if (layer == 0 || !layer->user_mask.present) {
        return psd89_wr_be32(io, 0U);
    }

    param_len = psd89_layer_mask_param_len(&layer->user_mask);
    emit_real = layer->user_mask.real_present || param_len != 0U;
    flags = layer->user_mask.flags;
    if (param_len != 0U) {
        flags = (psd89_u8)(flags | 0x10U);
    } else {
        flags = (psd89_u8)(flags & (psd89_u8)~0x10U);
    }
    size = 18U + param_len + (emit_real ? 18U : 2U);

    if (!psd89_wr_be32(io, size) ||
        !psd89_wr_be32(io, (psd89_u32)layer->user_mask.top) ||
        !psd89_wr_be32(io, (psd89_u32)layer->user_mask.left) ||
        !psd89_wr_be32(io, (psd89_u32)layer->user_mask.bottom) ||
        !psd89_wr_be32(io, (psd89_u32)layer->user_mask.right) ||
        !psd89_wr_u8(io, layer->user_mask.default_color) ||
        !psd89_wr_u8(io, flags)) {
        return 0;
    }
    if (!psd89_write_layer_mask_params(io, &layer->user_mask)) {
        return 0;
    }
    if (emit_real) {
        if (layer->user_mask.real_present) {
            real_flags = layer->user_mask.real_flags;
            real_background = layer->user_mask.real_background;
            real_top = layer->user_mask.real_top;
            real_left = layer->user_mask.real_left;
            real_bottom = layer->user_mask.real_bottom;
            real_right = layer->user_mask.real_right;
        } else {
            real_flags = flags;
            real_background = layer->user_mask.default_color;
            real_top = layer->user_mask.top;
            real_left = layer->user_mask.left;
            real_bottom = layer->user_mask.bottom;
            real_right = layer->user_mask.right;
        }
        if (!psd89_wr_u8(io, real_flags) ||
            !psd89_wr_u8(io, real_background) ||
            !psd89_wr_be32(io, (psd89_u32)real_top) ||
            !psd89_wr_be32(io, (psd89_u32)real_left) ||
            !psd89_wr_be32(io, (psd89_u32)real_bottom) ||
            !psd89_wr_be32(io, (psd89_u32)real_right)) {
            return 0;
        }
    } else {
        if (!psd89_wr_be16(io, 0U)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_validate_write_doc(const psd89_doc *doc)
{
    unsigned int i;
    unsigned int j;
    unsigned int base_channels;

    if (doc == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (doc->width == 0U || doc->height == 0U || doc->width > 30000U || doc->height > 30000U) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (doc->depth != 8U) {
        return PSD89_E_UNSUPPORTED_DEPTH;
    }
    if (doc->color_mode != PSD89_MODE_GRAYSCALE && doc->color_mode != PSD89_MODE_RGB) {
        return PSD89_E_UNSUPPORTED_MODE;
    }
    base_channels = psd89_doc_base_channels(doc);
    if (doc->channels < base_channels || doc->channels > 56U) {
        return PSD89_E_UNSUPPORTED_CHANNELS;
    }
    if (doc->merged_alpha_in_first_channel && doc->channels <= base_channels) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (doc->resource_count != 0U && !doc->has_passthrough_source) {
        return PSD89_E_BAD_STATE;
    }
    if ((doc->global_tag_count != 0U || doc->global_mask_raw_size != 0U) && !doc->has_passthrough_source) {
        return PSD89_E_BAD_STATE;
    }
    for (i = 0U; i < doc->layer_count; ++i) {
        const psd89_layer *layer;
        layer = &doc->layers[i];
        if (layer->right < layer->left || layer->bottom < layer->top) {
            return PSD89_E_BAD_ARGUMENT;
        }
        if (layer->channel_count > PSD89_MAX_CHANNELS_PER_LAYER) {
            return PSD89_E_LIMIT;
        }
        if ((layer->tag_count != 0U || layer->blend_ranges_ref.raw_size != 0U ||
             (layer->mask_ref.raw_size != 0U && !psd89_layer_has_authored_mask(layer))) &&
            !doc->has_passthrough_source) {
            return PSD89_E_BAD_STATE;
        }
        for (j = 0U; j < layer->channel_count; ++j) {
            if (layer->channels[j].compression != PSD89_COMP_RAW && layer->channels[j].compression != PSD89_COMP_RLE &&
                layer->channels[j].compression != PSD89_COMP_ZIP && layer->channels[j].compression != PSD89_COMP_ZIP_PRED) {
                return PSD89_E_UNSUPPORTED_COMPRESSION;
            }
        }
    }
    if (doc->composite_write_compression != PSD89_COMP_RAW && doc->composite_write_compression != PSD89_COMP_RLE &&
        doc->composite_write_compression != PSD89_COMP_ZIP && doc->composite_write_compression != PSD89_COMP_ZIP_PRED) {
        return PSD89_E_UNSUPPORTED_COMPRESSION;
    }
    return PSD89_OK;
}

static int psd89_write_header(psd89_io *io, const psd89_doc *doc)
{
    if (!psd89_wr_be32(io, PSD89_SIG_8BPS) ||
        !psd89_wr_be16(io, 1U) ||
        !psd89_write_zeroes(io, 6U) ||
        !psd89_wr_be16(io, doc->channels) ||
        !psd89_wr_be32(io, doc->height) ||
        !psd89_wr_be32(io, doc->width) ||
        !psd89_wr_be16(io, doc->depth) ||
        !psd89_wr_be16(io, doc->color_mode)) {
        return 0;
    }
    return 1;
}

static int psd89_copy_resource_blocks(psd89_io *io, const psd89_doc *doc)
{
    unsigned int i;
    for (i = 0U; i < doc->resource_count; ++i) {
        if (!psd89_copy_bytes(io,
                              (psd89_io *)&doc->passthrough_source,
                              doc->resources[i].raw_offset,
                              doc->resources[i].raw_size)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_copy_tag_blocks(psd89_io *io,
                                  const psd89_doc *doc,
                                  const psd89_tag_ref *tags,
                                  unsigned int count,
                                  const psd89_layer *layer)
{
    unsigned int i;
    for (i = 0U; i < count; ++i) {
        if (psd89_should_skip_raw_layer_tag(layer, &tags[i])) {
            continue;
        }
        if (!psd89_copy_bytes(io,
                              (psd89_io *)&doc->passthrough_source,
                              tags[i].raw_offset,
                              tags[i].raw_size)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_bool_tag(psd89_io *io, const char key[4], psd89_u8 value)
{
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) ||
        !psd89_io_write(io, key, 4U) ||
        !psd89_wr_be32(io, 4U) ||
        !psd89_wr_u8(io, (psd89_u8)(value != 0U)) ||
        !psd89_wr_u8(io, 0U) ||
        !psd89_wr_u8(io, 0U) ||
        !psd89_wr_u8(io, 0U)) {
        return 0;
    }
    return 1;
}

static psd89_u32 psd89_vector_mask_record_count(const psd89_vector_mask *vm)
{
    psd89_u32 count;
    unsigned int i;

    if (vm == 0 || !vm->present) {
        return 0U;
    }
    count = 1U;
    if (vm->initial_fill_rule_present) {
        ++count;
    }
    for (i = 0U; i < vm->subpath_count; ++i) {
        count += 1U + (psd89_u32)vm->subpaths[i].knot_count;
    }
    return count;
}

static int psd89_write_vector_record_zero(psd89_io *io, psd89_u16 selector, psd89_u8 first_u16_nonzero)
{
    if (!psd89_wr_be16(io, selector)) {
        return 0;
    }
    if (first_u16_nonzero != 0U) {
        if (!psd89_wr_be16(io, 1U)) {
            return 0;
        }
        return psd89_write_zeroes(io, 22U);
    }
    return psd89_write_zeroes(io, 24U);
}

static int psd89_write_vector_mask_tag(psd89_io *io, const psd89_layer *layer)
{
    const psd89_vector_mask *vm;
    unsigned int i;
    unsigned int j;
    psd89_u32 len;
    psd89_u32 flags;
    const char vmsk[4] = { 'v', 'm', 's', 'k' };
    const char vsms[4] = { 'v', 's', 'm', 's' };

    vm = &layer->vector_mask;
    len = 8U + 26U * (psd89_u32)psd89_vector_mask_record_count(vm);
    flags = (psd89_u32)vm->flags;
    if (flags == 0U) {
        if (vm->invert) { flags |= 0x01U; }
        if (vm->not_link) { flags |= 0x02U; }
        if (vm->disabled) { flags |= 0x04U; }
    }
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) ||
        !psd89_io_write(io, vm->use_vsms ? vsms : vmsk, 4U) ||
        !psd89_wr_be32(io, (psd89_u32)len) ||
        !psd89_wr_be32(io, vm->version != 0U ? (psd89_u32)vm->version : 3U) ||
        !psd89_wr_be32(io, flags)) {
        return 0;
    }
    if (!psd89_write_vector_record_zero(io, 6U, 0U)) {
        return 0;
    }
    if (vm->initial_fill_rule_present) {
        if (!psd89_write_vector_record_zero(io, 8U, vm->initial_fill_rule)) {
            return 0;
        }
    }
    for (i = 0U; i < vm->subpath_count; ++i) {
        const psd89_vector_subpath *sp;
        sp = &vm->subpaths[i];
        if (!psd89_wr_be16(io, (psd89_u16)(sp->closed ? 0U : 3U)) ||
            !psd89_wr_be16(io, sp->knot_count) ||
            !psd89_write_zeroes(io, 22U)) {
            return 0;
        }
        for (j = 0U; j < sp->knot_count; ++j) {
            const psd89_vector_knot *knot;
            psd89_u16 selector;
            unsigned int idx;

            idx = (unsigned int)sp->first_knot + j;
            if (idx >= vm->knot_count) {
                return 0;
            }
            knot = &vm->knots[idx];
            if (sp->closed) {
                selector = (psd89_u16)(knot->linked ? 1U : 2U);
            } else {
                selector = (psd89_u16)(knot->linked ? 4U : 5U);
            }
            if (!psd89_wr_be16(io, selector) ||
                !psd89_wr_be32(io, (psd89_u32)knot->preceding_v) ||
                !psd89_wr_be32(io, (psd89_u32)knot->preceding_h) ||
                !psd89_wr_be32(io, (psd89_u32)knot->anchor_v) ||
                !psd89_wr_be32(io, (psd89_u32)knot->anchor_h) ||
                !psd89_wr_be32(io, (psd89_u32)knot->leaving_v) ||
                !psd89_wr_be32(io, (psd89_u32)knot->leaving_h)) {
                return 0;
            }
        }
    }
    return 1;
}

static int psd89_write_section_divider_tag(psd89_io *io, const psd89_layer *layer)
{
    static const char lsct[4] = { 'l', 's', 'c', 't' };
    psd89_u32 len;
    const char *blend_key;

    if (layer == 0 || !layer->section_divider_present) {
        return 1;
    }
    len = 4U;
    blend_key = 0;
    if (psd89_has_nonempty4(layer->section_divider_blend_mode)) {
        len = 12U;
        blend_key = layer->section_divider_blend_mode;
    }
    if (layer->section_divider_subtype_present) {
        len = 16U;
        if (blend_key == 0) {
            blend_key = "norm";
        }
    }
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM) ||
        !psd89_io_write(io, lsct, 4U) ||
        !psd89_wr_be32(io, len) ||
        !psd89_wr_be32(io, (psd89_u32)layer->section_divider_type)) {
        return 0;
    }
    if (len >= 12U) {
        if (!psd89_wr_be32(io, PSD89_SIG_8BIM) ||
            !psd89_io_write(io, blend_key, 4U)) {
            return 0;
        }
    }
    if (len >= 16U) {
        if (!psd89_wr_be32(io, (psd89_u32)layer->section_divider_subtype)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_layer_authored_tags(psd89_io *io, const psd89_layer *layer)
{
    static const char clbl[4] = { 'c', 'l', 'b', 'l' };
    static const char infx[4] = { 'i', 'n', 'f', 'x' };
    static const char knko[4] = { 'k', 'n', 'k', 'o' };
    static const char lmgm[4] = { 'l', 'm', 'g', 'm' };
    static const char vmgm[4] = { 'v', 'm', 'g', 'm' };
    static const char tsly[4] = { 't', 's', 'l', 'y' };

    if (layer->blend_clipped_present && !psd89_write_bool_tag(io, clbl, layer->blend_clipped)) {
        return 0;
    }
    if (layer->blend_interior_present && !psd89_write_bool_tag(io, infx, layer->blend_interior)) {
        return 0;
    }
    if (layer->knockout_present && !psd89_write_bool_tag(io, knko, layer->knockout)) {
        return 0;
    }
    if (layer->layer_mask_global_present && !psd89_write_bool_tag(io, lmgm, layer->layer_mask_global)) {
        return 0;
    }
    if (layer->vector_mask_global_present && !psd89_write_bool_tag(io, vmgm, layer->vector_mask_global)) {
        return 0;
    }
    if (layer->transparency_shapes_layer_present && !psd89_write_bool_tag(io, tsly, layer->transparency_shapes_layer)) {
        return 0;
    }
    if (layer->section_divider_present && !psd89_write_section_divider_tag(io, layer)) {
        return 0;
    }
    if (psd89_layer_has_authored_vector_mask(layer) && !psd89_write_vector_mask_tag(io, layer)) {
        return 0;
    }
    if (layer->lrfx.present && !psd89_write_lrfx_tag(io, layer)) {
        return 0;
    }
    if (layer->type_tool.present && !psd89_write_tysh_tag(io, layer)) {
        return 0;
    }
    if (layer->text_engine.present && layer->text_engine.authored_raw != 0 && layer->text_engine.authored_raw_size != 0U && !psd89_write_txt2_tag(io, layer)) {
        return 0;
    }
    if (layer->object_effects.present && !psd89_write_lfx2_tag(io, layer)) {
        return 0;
    }
    if (layer->smart_object.present && !psd89_write_smart_object_tag(io, layer)) {
        return 0;
    }
    return 1;
}

static int psd89_write_image_resources(psd89_io *io, const psd89_doc *doc)
{
    psd89_u32 len_pos;
    psd89_u32 start;
    len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (doc->resource_count != 0U) {
        if (!psd89_copy_resource_blocks(io, doc)) {
            return 0;
        }
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)(psd89_io_tell(io) - start));
}

static int psd89_write_layer_record(psd89_io *io,
                                    const psd89_doc *doc,
                                    const psd89_layer *layer,
                                    psd89_u32 *channel_len_pos)
{
    unsigned int j;
    psd89_u32 extra_len_pos;
    psd89_u32 extra_start;
    psd89_u32 extra_len;
    const char norm[4] = { 'n', 'o', 'r', 'm' };

    if (!psd89_wr_be32(io, (psd89_u32)layer->top) ||
        !psd89_wr_be32(io, (psd89_u32)layer->left) ||
        !psd89_wr_be32(io, (psd89_u32)layer->bottom) ||
        !psd89_wr_be32(io, (psd89_u32)layer->right) ||
        !psd89_wr_be16(io, layer->channel_count)) {
        return 0;
    }
    for (j = 0U; j < layer->channel_count; ++j) {
        if (!psd89_wr_be16(io, (psd89_u16)layer->channels[j].id)) {
            return 0;
        }
        channel_len_pos[j] = psd89_io_tell(io);
        if (!psd89_wr_be32(io, 0U)) {
            return 0;
        }
    }
    if (!psd89_wr_be32(io, PSD89_SIG_8BIM)) {
        return 0;
    }
    if (psd89_has_nonempty4(layer->blend_mode)) {
        if (!psd89_io_write(io, layer->blend_mode, 4U)) {
            return 0;
        }
    } else if (!psd89_io_write(io, norm, 4U)) {
        return 0;
    }
    if (!psd89_wr_u8(io, layer->opacity) ||
        !psd89_wr_u8(io, layer->clipping) ||
        !psd89_wr_u8(io, layer->flags) ||
        !psd89_wr_u8(io, 0U)) {
        return 0;
    }
    extra_len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    extra_start = psd89_io_tell(io);

    if (layer->mask_ref.raw_size != 0U) {
        if (!psd89_copy_bytes(io,
                              (psd89_io *)&doc->passthrough_source,
                              layer->mask_ref.raw_offset,
                              layer->mask_ref.raw_size)) {
            return 0;
        }
    } else if (psd89_layer_has_authored_mask(layer)) {
        if (!psd89_write_layer_mask_block(io, layer)) {
            return 0;
        }
    } else if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }

    if (layer->blend_ranges_ref.raw_size != 0U) {
        if (!psd89_copy_bytes(io,
                              (psd89_io *)&doc->passthrough_source,
                              layer->blend_ranges_ref.raw_offset,
                              layer->blend_ranges_ref.raw_size)) {
            return 0;
        }
    } else if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }

    if (!psd89_write_pascal_string(io, layer->name, 4U)) {
        return 0;
    }
    if (layer->tag_count != 0U) {
        if (!psd89_copy_tag_blocks(io, doc, layer->tags, layer->tag_count, layer)) {
            return 0;
        }
    }
    if (!psd89_write_layer_authored_tags(io, layer)) {
        return 0;
    }

    extra_len = (psd89_u32)(psd89_io_tell(io) - extra_start);
    return psd89_patch_be32(io, extra_len_pos, extra_len);
}

static psd89_u32 psd89_rle_zero_row_len(psd89_u32 row_bytes)
{
    return ((row_bytes + 127U) / 128U) * 2U;
}

static int psd89_write_plane_raw(psd89_io *io,
                                 const psd89_u8 *plane,
                                 psd89_u32 stride,
                                 psd89_u32 rows,
                                 psd89_u32 cols)
{
    psd89_u32 y;
    for (y = 0U; y < rows; ++y) {
        if (plane != 0) {
            if (!psd89_io_write(io, plane + (psd89_u32)y * stride, (psd89_u32)cols)) {
                return 0;
            }
        } else if (!psd89_write_zeroes(io, (psd89_u32)cols)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_plane_rle(psd89_io *io,
                                 const psd89_u8 *plane,
                                 psd89_u32 stride,
                                 psd89_u32 rows,
                                 psd89_u32 cols)
{
    psd89_u32 y;
    psd89_u32 row_len;
    for (y = 0U; y < rows; ++y) {
        if (plane != 0) {
            row_len = psd89_packbits_encoded_len(plane + (psd89_u32)y * stride, (psd89_u32)cols);
        } else {
            row_len = psd89_rle_zero_row_len((psd89_u32)cols);
        }
        if (row_len > 65535U) {
            return 0;
        }
        if (!psd89_wr_be16(io, (psd89_u16)row_len)) {
            return 0;
        }
    }
    for (y = 0U; y < rows; ++y) {
        if (plane != 0) {
            if (!psd89_packbits_write_row(io, plane + (psd89_u32)y * stride, (psd89_u32)cols)) {
                return 0;
            }
        } else if (!psd89_packbits_write_zero_row(io, (psd89_u32)cols)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_write_layer_channel_data(psd89_io *io,
                                          const psd89_layer *layer,
                                          const psd89_layer_channel *ch,
                                          psd89_u32 len_pos)
{
    psd89_u32 start;
    psd89_u32 size;
    psd89_u32 rows;
    psd89_u32 cols;
    if (!psd89_layer_channel_dims(layer, ch->id, &rows, &cols)) {
        return 0;
    }
    start = psd89_io_tell(io);
    if (!psd89_wr_be16(io, ch->compression)) {
        return 0;
    }
    if (ch->compression == PSD89_COMP_RAW) {
        if (!psd89_write_plane_raw(io, ch->plane, ch->stride ? ch->stride : cols, rows, cols)) {
            return 0;
        }
    } else if (ch->compression == PSD89_COMP_RLE) {
        if (!psd89_write_plane_rle(io, ch->plane, ch->stride ? ch->stride : cols, rows, cols)) {
            return 0;
        }
    } else if (ch->compression == PSD89_COMP_ZIP || ch->compression == PSD89_COMP_ZIP_PRED) {
        if (!psd89_zip_write_plane(io, ch->plane, ch->stride ? ch->stride : cols, rows, cols, ch->compression == PSD89_COMP_ZIP_PRED)) {
            return 0;
        }
    } else {
        return 0;
    }
    size = psd89_io_tell(io) - start;
    if ((size & 1U) != 0U) {
        if (!psd89_wr_u8(io, 0U)) {
            return 0;
        }
        ++size;
    }
    return psd89_patch_be32(io, len_pos, (psd89_u32)size);
}

static int psd89_write_layers_section(psd89_io *io, const psd89_doc *doc)
{
    psd89_u32 section_len_pos;
    psd89_u32 section_start;
    psd89_u32 layer_info_len_pos;
    psd89_u32 layer_info_start;
    psd89_u32 channel_len_pos[PSD89_MAX_LAYERS][PSD89_MAX_CHANNELS_PER_LAYER];
    unsigned int i;
    unsigned int j;
    psd89_u32 layer_info_len;
    psd89_u32 section_len;
    psd89_s16 layer_count_s16;

    section_len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    section_start = psd89_io_tell(io);

    layer_info_len_pos = psd89_io_tell(io);
    if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }
    layer_info_start = psd89_io_tell(io);
    if (doc->layer_count != 0U) {
        layer_count_s16 = (psd89_s16)doc->layer_count;
        if (doc->merged_alpha_in_first_channel && doc->channels > psd89_doc_base_channels(doc)) {
            layer_count_s16 = (psd89_s16)(-layer_count_s16);
        }
        if (!psd89_wr_be16(io, (psd89_u16)layer_count_s16)) {
            return 0;
        }
        for (i = 0U; i < doc->layer_count; ++i) {
            if (!psd89_write_layer_record(io, doc, &doc->layers[i], channel_len_pos[i])) {
                return 0;
            }
        }
        for (i = 0U; i < doc->layer_count; ++i) {
            for (j = 0U; j < doc->layers[i].channel_count; ++j) {
                if (!psd89_write_layer_channel_data(io,
                                                   &doc->layers[i],
                                                   &doc->layers[i].channels[j],
                                                   channel_len_pos[i][j])) {
                    return 0;
                }
            }
        }
    }
    if (((psd89_io_tell(io) - layer_info_start) & 1U) != 0U) {
        if (!psd89_wr_u8(io, 0U)) {
            return 0;
        }
    }
    layer_info_len = psd89_io_tell(io) - layer_info_start;
    if (!psd89_patch_be32(io, layer_info_len_pos, (psd89_u32)layer_info_len)) {
        return 0;
    }

    if (doc->global_mask_raw_size != 0U) {
        if (!psd89_copy_bytes(io,
                              (psd89_io *)&doc->passthrough_source,
                              doc->global_mask_raw_offset,
                              doc->global_mask_raw_size)) {
            return 0;
        }
    } else if (!psd89_wr_be32(io, 0U)) {
        return 0;
    }

    if (doc->global_tag_count != 0U) {
        if (!psd89_copy_tag_blocks(io, doc, doc->global_tags, doc->global_tag_count, 0)) {
            return 0;
        }
    }

    section_len = psd89_io_tell(io) - section_start;
    return psd89_patch_be32(io, section_len_pos, (psd89_u32)section_len);
}

static int psd89_write_composite_data(psd89_io *io, const psd89_doc *doc)
{
    psd89_u32 c;
    psd89_u32 rows;
    psd89_u32 cols;
    rows = doc->height;
    cols = doc->width;
    if (!psd89_wr_be16(io, doc->composite_write_compression)) {
        return 0;
    }
    if (doc->composite_write_compression == PSD89_COMP_RAW) {
        for (c = 0U; c < doc->channels; ++c) {
            if (!psd89_write_plane_raw(io,
                                       doc->composite_planes[c],
                                       doc->composite_stride ? doc->composite_stride : cols,
                                       rows,
                                       cols)) {
                return 0;
            }
        }
        return 1;
    }
    if (doc->composite_write_compression == PSD89_COMP_RLE) {
        for (c = 0U; c < doc->channels; ++c) {
            psd89_u32 y;
            psd89_u32 row_len;
            const psd89_u8 *plane;
            plane = doc->composite_planes[c];
            for (y = 0U; y < rows; ++y) {
                if (plane != 0) {
                    row_len = psd89_packbits_encoded_len(plane + (psd89_u32)y * (doc->composite_stride ? doc->composite_stride : cols),
                                                         (psd89_u32)cols);
                } else {
                    row_len = psd89_rle_zero_row_len((psd89_u32)cols);
                }
                if (row_len > 65535U) {
                    return 0;
                }
                if (!psd89_wr_be16(io, (psd89_u16)row_len)) {
                    return 0;
                }
            }
        }
        for (c = 0U; c < doc->channels; ++c) {
            const psd89_u8 *plane;
            psd89_u32 y;
            plane = doc->composite_planes[c];
            for (y = 0U; y < rows; ++y) {
                if (plane != 0) {
                    if (!psd89_packbits_write_row(io,
                                                  plane + (psd89_u32)y * (doc->composite_stride ? doc->composite_stride : cols),
                                                  (psd89_u32)cols)) {
                        return 0;
                    }
                } else if (!psd89_packbits_write_zero_row(io, (psd89_u32)cols)) {
                    return 0;
                }
            }
        }
        return 1;
    }
    if (doc->composite_write_compression == PSD89_COMP_ZIP || doc->composite_write_compression == PSD89_COMP_ZIP_PRED) {
        return psd89_zip_write_composite(io, (const psd89_u8 **)doc->composite_planes, doc->composite_stride ? doc->composite_stride : cols, doc->channels, rows, cols, doc->composite_write_compression == PSD89_COMP_ZIP_PRED);
    }
    return 0;
}

int psd89_write(psd89_io *io, const psd89_doc *doc)
{
    int rc;
    if (io == 0 || doc == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    rc = psd89_validate_write_doc(doc);
    if (rc != PSD89_OK) {
        return rc;
    }
    if (!psd89_write_header(io, doc)) {
        return PSD89_E_IO;
    }
    if (!psd89_wr_be32(io, 0U)) {
        return PSD89_E_IO;
    }
    if (!psd89_write_image_resources(io, doc)) {
        return PSD89_E_IO;
    }
    if (!psd89_write_layers_section(io, doc)) {
        return PSD89_E_IO;
    }
    if (!psd89_write_composite_data(io, doc)) {
        return PSD89_E_IO;
    }
    return PSD89_OK;
}
