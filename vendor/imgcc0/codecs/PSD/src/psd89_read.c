#include "psd89_internal.h"

static int psd89_mode_channel_ok(psd89_u16 mode, psd89_u16 channels)
{
    if (mode == PSD89_MODE_GRAYSCALE) {
        return channels >= 1U && channels <= 56U;
    }
    if (mode == PSD89_MODE_RGB) {
        return channels >= 3U && channels <= 56U;
    }
    return 0;
}

static int psd89_tag_key_eq(const char key[4], const char *lit4)
{
    return key[0] == lit4[0] && key[1] == lit4[1] && key[2] == lit4[2] && key[3] == lit4[3];
}

static void psd89_parse_known_layer_bool_tag(psd89_layer *layer, const char key[4], psd89_u8 v)
{
    if (layer == 0) {
        return;
    }
    if (psd89_tag_key_eq(key, "clbl")) {
        layer->blend_clipped = (psd89_u8)(v != 0U);
        layer->blend_clipped_present = 1U;
    } else if (psd89_tag_key_eq(key, "infx")) {
        layer->blend_interior = (psd89_u8)(v != 0U);
        layer->blend_interior_present = 1U;
    } else if (psd89_tag_key_eq(key, "knko")) {
        layer->knockout = (psd89_u8)(v != 0U);
        layer->knockout_present = 1U;
    } else if (psd89_tag_key_eq(key, "lmgm")) {
        layer->layer_mask_global = (psd89_u8)(v != 0U);
        layer->layer_mask_global_present = 1U;
    } else if (psd89_tag_key_eq(key, "vmgm")) {
        layer->vector_mask_global = (psd89_u8)(v != 0U);
        layer->vector_mask_global_present = 1U;
    } else if (psd89_tag_key_eq(key, "tsly")) {
        layer->transparency_shapes_layer = (psd89_u8)(v != 0U);
        layer->transparency_shapes_layer_present = 1U;
    }
}

static int psd89_parse_section_divider_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len)
{
    psd89_u32 type;
    psd89_u32 sig;
    psd89_u32 subtype;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (len < 4U) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (!psd89_rd_be32(io, &type)) {
        return PSD89_E_TRUNCATED;
    }
    layer->section_divider_type = (psd89_u8)(type & 0xFFU);
    layer->section_divider_present = 1U;
    if (len >= 12U) {
        if (!psd89_rd_be32(io, &sig) || !psd89_io_read(io, layer->section_divider_blend_mode, 4U)) {
            return PSD89_E_TRUNCATED;
        }
        if (sig != PSD89_SIG_8BIM) {
            return PSD89_E_BAD_LAYER;
        }
    }
    if (len >= 16U) {
        if (!psd89_rd_be32(io, &subtype)) {
            return PSD89_E_TRUNCATED;
        }
        layer->section_divider_subtype = (psd89_u8)(subtype & 0xFFU);
        layer->section_divider_subtype_present = 1U;
    }
    if (len > 16U && !psd89_skip(io, (psd89_u32)(len - 16U))) {
        return PSD89_E_TRUNCATED;
    }
    if (len > 4U && len < 12U && !psd89_skip(io, (psd89_u32)(len - 4U))) {
        return PSD89_E_TRUNCATED;
    }
    if (len > 12U && len < 16U && !psd89_skip(io, (psd89_u32)(len - 12U))) {
        return PSD89_E_TRUNCATED;
    }
    return PSD89_OK;
}

static int psd89_parse_mask_parameters(psd89_layer_user_mask *mask, psd89_io *io, psd89_u32 *remain)
{
    psd89_u8 param_flags;

    if (mask == 0 || io == 0 || remain == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (*remain == 0U) {
        return PSD89_OK;
    }
    if (!psd89_rd_u8(io, &param_flags)) {
        return PSD89_E_TRUNCATED;
    }
    --(*remain);
    mask->params_flags = param_flags;

    if ((param_flags & 0x01U) != 0U) {
        if (*remain < 1U || !psd89_rd_u8(io, &mask->user_density)) {
            return PSD89_E_TRUNCATED;
        }
        mask->user_density_present = 1U;
        --(*remain);
    }
    if ((param_flags & 0x02U) != 0U) {
        if (*remain < 8U || !psd89_rd_ieee8_q16(io, &mask->user_feather)) {
            return PSD89_E_TRUNCATED;
        }
        mask->user_feather_present = 1U;
        *remain -= 8U;
    }
    if ((param_flags & 0x04U) != 0U) {
        if (*remain < 1U || !psd89_rd_u8(io, &mask->vector_density)) {
            return PSD89_E_TRUNCATED;
        }
        mask->vector_density_present = 1U;
        --(*remain);
    }
    if ((param_flags & 0x08U) != 0U) {
        if (*remain < 8U || !psd89_rd_ieee8_q16(io, &mask->vector_feather)) {
            return PSD89_E_TRUNCATED;
        }
        mask->vector_feather_present = 1U;
        *remain -= 8U;
    }
    return PSD89_OK;
}

static int psd89_parse_layer_mask_data(psd89_layer *layer, psd89_io *io, psd89_u32 mask_len)
{
    psd89_u32 remain;
    int rc;

    if (layer == 0) {
        return mask_len == 0U ? PSD89_OK : (psd89_skip(io, (psd89_u32)mask_len) ? PSD89_OK : PSD89_E_TRUNCATED);
    }
    if (mask_len == 0U) {
        return PSD89_OK;
    }
    if (mask_len < 18U) {
        return psd89_skip(io, (psd89_u32)mask_len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (!psd89_rd_be32s(io, &layer->user_mask.top) ||
        !psd89_rd_be32s(io, &layer->user_mask.left) ||
        !psd89_rd_be32s(io, &layer->user_mask.bottom) ||
        !psd89_rd_be32s(io, &layer->user_mask.right) ||
        !psd89_rd_u8(io, &layer->user_mask.default_color) ||
        !psd89_rd_u8(io, &layer->user_mask.flags)) {
        return PSD89_E_TRUNCATED;
    }
    layer->user_mask.present = 1U;
    remain = (psd89_u32)mask_len - 18U;
    if ((layer->user_mask.flags & 0x10U) != 0U) {
        rc = psd89_parse_mask_parameters(&layer->user_mask, io, &remain);
        if (rc != PSD89_OK) {
            return rc;
        }
    }
    if (remain == 2U) {
        return psd89_skip(io, 2U) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (remain >= 18U) {
        if (!psd89_rd_u8(io, &layer->user_mask.real_flags) ||
            !psd89_rd_u8(io, &layer->user_mask.real_background) ||
            !psd89_rd_be32s(io, &layer->user_mask.real_top) ||
            !psd89_rd_be32s(io, &layer->user_mask.real_left) ||
            !psd89_rd_be32s(io, &layer->user_mask.real_bottom) ||
            !psd89_rd_be32s(io, &layer->user_mask.real_right)) {
            return PSD89_E_TRUNCATED;
        }
        layer->user_mask.real_present = 1U;
        remain -= 18U;
    }
    return psd89_skip(io, remain) ? PSD89_OK : PSD89_E_TRUNCATED;
}

static int psd89_parse_vector_mask_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len, int use_vsms)
{
    psd89_vector_mask *vm;
    psd89_u32 version;
    psd89_u32 flags;
    psd89_u32 remain;
    psd89_u16 selector;
    psd89_u8 rec[24];
    int current_subpath;

    if (layer == 0) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    if (len < 8U) {
        return psd89_skip(io, (psd89_u32)len) ? PSD89_OK : PSD89_E_TRUNCATED;
    }
    vm = &layer->vector_mask;
    memset(vm, 0, sizeof(*vm));
    if (!psd89_rd_be32(io, &version) || !psd89_rd_be32(io, &flags)) {
        return PSD89_E_TRUNCATED;
    }
    vm->present = 1U;
    vm->use_vsms = (psd89_u8)(use_vsms != 0);
    vm->version = (psd89_u8)version;
    vm->flags = (psd89_u8)(flags & 0xFFU);
    vm->invert = (psd89_u8)((flags & 0x01U) != 0U);
    vm->not_link = (psd89_u8)((flags & 0x02U) != 0U);
    vm->disabled = (psd89_u8)((flags & 0x04U) != 0U);
    remain = (psd89_u32)len - 8U;
    current_subpath = -1;
    while (remain >= 26U) {
        if (!psd89_rd_be16(io, &selector)) {
            return PSD89_E_TRUNCATED;
        }
        remain -= 2U;
        if (selector == 0U || selector == 3U) {
            psd89_u16 count;
            if (!psd89_rd_be16(io, &count) || !psd89_skip(io, 22U)) {
                return PSD89_E_TRUNCATED;
            }
            remain -= 24U;
            if (vm->subpath_count < PSD89_MAX_VECTOR_SUBPATHS) {
                vm->subpaths[vm->subpath_count].closed = (psd89_u8)(selector == 0U);
                vm->subpaths[vm->subpath_count].knot_count = 0U;
                vm->subpaths[vm->subpath_count].first_knot = vm->knot_count;
                current_subpath = (int)vm->subpath_count;
                ++vm->subpath_count;
            } else {
                current_subpath = -1;
            }
        } else if (selector == 1U || selector == 2U || selector == 4U || selector == 5U) {
            psd89_vector_knot knot;
            if (!psd89_rd_be32s(io, &knot.preceding_v) ||
                !psd89_rd_be32s(io, &knot.preceding_h) ||
                !psd89_rd_be32s(io, &knot.anchor_v) ||
                !psd89_rd_be32s(io, &knot.anchor_h) ||
                !psd89_rd_be32s(io, &knot.leaving_v) ||
                !psd89_rd_be32s(io, &knot.leaving_h)) {
                return PSD89_E_TRUNCATED;
            }
            remain -= 24U;
            knot.linked = (psd89_u8)(selector == 1U || selector == 4U);
            if (current_subpath >= 0 && vm->knot_count < PSD89_MAX_VECTOR_KNOTS) {
                vm->knots[vm->knot_count] = knot;
                ++vm->subpaths[current_subpath].knot_count;
                ++vm->knot_count;
            }
        } else {
            if (!psd89_io_read(io, rec, 24U)) {
                return PSD89_E_TRUNCATED;
            }
            remain -= 24U;
            if (selector == 6U) {
                vm->path_fill_rule_present = 1U;
            } else if (selector == 8U) {
                vm->initial_fill_rule_present = 1U;
                vm->initial_fill_rule = (psd89_u8)((rec[0] != 0U || rec[1] != 0U) ? 1U : 0U);
            }
        }
    }
    if (remain != 0U && !psd89_skip(io, remain)) {
        return PSD89_E_TRUNCATED;
    }
    return PSD89_OK;
}


static int psd89_parse_image_resources(psd89_doc *doc, psd89_io *io, psd89_u32 end)
{
    psd89_u32 sig;
    psd89_u16 id;
    psd89_u32 size;
    psd89_u32 block_start;
    psd89_u32 block_end;
    while (psd89_io_tell(io) < end) {
        block_start = psd89_io_tell(io);
        if (!psd89_rd_be32(io, &sig) || !psd89_rd_be16(io, &id)) {
            return PSD89_E_TRUNCATED;
        }
        if (sig != PSD89_SIG_8BIM) {
            return PSD89_E_BAD_RESOURCE;
        }
        if (!psd89_read_pascal_string(io,
                                      doc->resource_count < PSD89_MAX_IMAGE_RESOURCES ? doc->resources[doc->resource_count].name : 0,
                                      PSD89_MAX_RESOURCE_NAME_CHARS + 1U,
                                      2U)) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_rd_be32(io, &size)) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_skip(io, (psd89_u32)size + (psd89_u32)(size & 1U))) {
            return PSD89_E_TRUNCATED;
        }
        block_end = psd89_io_tell(io);
        if (doc->resource_count < PSD89_MAX_IMAGE_RESOURCES) {
            doc->resources[doc->resource_count].id = id;
            doc->resources[doc->resource_count].raw_offset = block_start;
            doc->resources[doc->resource_count].raw_size = (psd89_u32)(block_end - block_start);
            ++doc->resource_count;
        }
    }
    if (psd89_io_tell(io) != end) {
        return PSD89_E_BAD_RESOURCE;
    }
    return PSD89_OK;
}

static int psd89_parse_tag_block(psd89_tag_ref *ref, psd89_io *io, psd89_layer *layer)
{
    psd89_u32 sig;
    psd89_u32 len;
    psd89_u32 start;
    psd89_u8 bool_v;

    start = psd89_io_tell(io);
    if (!psd89_rd_be32(io, &sig)) {
        return PSD89_E_TRUNCATED;
    }
    if (sig != PSD89_SIG_8BIM && sig != 0x38423634U) {
        return PSD89_E_BAD_LAYER;
    }
    if (!psd89_io_read(io, ref->key, 4U)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_rd_be32(io, &len)) {
        return PSD89_E_TRUNCATED;
    }
    if (layer != 0 && len != 0U && (psd89_tag_key_eq(ref->key, "vmsk") || psd89_tag_key_eq(ref->key, "vsms"))) {
        int use_vsms;
        use_vsms = psd89_tag_key_eq(ref->key, "vsms");
        {
            int vrc;
            vrc = psd89_parse_vector_mask_tag(layer, io, len, use_vsms);
            if (vrc != PSD89_OK) {
                return vrc;
            }
        }
    } else if (layer != 0 && len != 0U && psd89_tag_key_eq(ref->key, "lsct")) {
        int lrc;
        lrc = psd89_parse_section_divider_tag(layer, io, len);
        if (lrc != PSD89_OK) {
            return lrc;
        }
    } else if (layer != 0 && len != 0U && (psd89_tag_key_eq(ref->key, "clbl") || psd89_tag_key_eq(ref->key, "infx") || psd89_tag_key_eq(ref->key, "knko") || psd89_tag_key_eq(ref->key, "lmgm") || psd89_tag_key_eq(ref->key, "vmgm") || psd89_tag_key_eq(ref->key, "tsly"))) {
        if (!psd89_rd_u8(io, &bool_v)) {
            return PSD89_E_TRUNCATED;
        }
        psd89_parse_known_layer_bool_tag(layer, ref->key, bool_v);
        if (len > 1U && !psd89_skip(io, (psd89_u32)(len - 1U))) {
            return PSD89_E_TRUNCATED;
        }
    } else if (layer != 0 && len != 0U && psd89_tag_key_eq(ref->key, "lrFX")) {
        return psd89_parse_lrfx_tag(layer, io, len);
    } else if (layer != 0 && len != 0U && psd89_tag_key_eq(ref->key, "TySh")) {
        return psd89_parse_tysh_tag(layer, io, len);
    } else if (layer != 0 && len != 0U && psd89_tag_key_eq(ref->key, "Txt2")) {
        return psd89_parse_txt2_tag(layer, io, len);
    } else if (layer != 0 && len != 0U && psd89_tag_key_eq(ref->key, "lfx2")) {
        return psd89_parse_lfx2_tag(layer, io, len);
    } else if (layer != 0 && len != 0U && (psd89_tag_key_eq(ref->key, "SoLd") || psd89_tag_key_eq(ref->key, "SoLE") || psd89_tag_key_eq(ref->key, "plLd"))) {
        return psd89_parse_smart_object_tag(layer, ref->key, io, len);
    } else if (!psd89_skip(io, (psd89_u32)len)) {
        return PSD89_E_TRUNCATED;
    }
    if ((len & 1U) != 0U && !psd89_skip(io, 1U)) {
        return PSD89_E_TRUNCATED;
    }
    ref->raw_offset = start;
    ref->raw_size = (psd89_u32)(psd89_io_tell(io) - start);
    return PSD89_OK;
}

static int psd89_parse_layers(psd89_doc *doc, psd89_io *io, psd89_u32 layer_info_end)
{
    psd89_s16 layer_count_s16;
    unsigned int i;
    unsigned int j;
    int rc;
    psd89_u32 extra_end;
    psd89_u32 extra_len;
    psd89_u32 mask_len;
    psd89_u32 blend_len;
    psd89_u32 block_start;
    unsigned int abs_count;

    if (!psd89_rd_be16s(io, &layer_count_s16)) {
        return PSD89_E_TRUNCATED;
    }
    doc->merged_alpha_in_first_channel = layer_count_s16 < 0 ? 1 : 0;
    abs_count = (unsigned int)(layer_count_s16 < 0 ? -layer_count_s16 : layer_count_s16);
    if (abs_count > PSD89_MAX_LAYERS) {
        return PSD89_E_LIMIT;
    }
    doc->layer_count = (psd89_u16)abs_count;

    for (i = 0U; i < abs_count; ++i) {
        psd89_layer *layer;
        psd89_u32 blend_sig;
        psd89_u8 filler;
        psd89_u16 ch_count;
        layer = &doc->layers[i];
        if (!psd89_rd_be32s(io, &layer->top) ||
            !psd89_rd_be32s(io, &layer->left) ||
            !psd89_rd_be32s(io, &layer->bottom) ||
            !psd89_rd_be32s(io, &layer->right)) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_rd_be16(io, &ch_count)) {
            return PSD89_E_TRUNCATED;
        }
        if (ch_count > PSD89_MAX_CHANNELS_PER_LAYER) {
            return PSD89_E_LIMIT;
        }
        layer->channel_count = ch_count;
        for (j = 0U; j < ch_count; ++j) {
            if (!psd89_rd_be16s(io, &layer->channels[j].id) ||
                !psd89_rd_be32(io, &layer->channels[j].data_length)) {
                return PSD89_E_TRUNCATED;
            }
        }
        if (!psd89_rd_be32(io, &blend_sig)) {
            return PSD89_E_TRUNCATED;
        }
        if (blend_sig != PSD89_SIG_8BIM) {
            return PSD89_E_BAD_LAYER;
        }
        if (!psd89_io_read(io, layer->blend_mode, 4U) ||
            !psd89_rd_u8(io, &layer->opacity) ||
            !psd89_rd_u8(io, &layer->clipping) ||
            !psd89_rd_u8(io, &layer->flags) ||
            !psd89_rd_u8(io, &filler) ||
            !psd89_rd_be32(io, &extra_len)) {
            return PSD89_E_TRUNCATED;
        }
        extra_end = psd89_io_tell(io) + (psd89_u32)extra_len;
        block_start = (psd89_u32)psd89_io_tell(io);
        if (!psd89_rd_be32(io, &mask_len)) {
            return PSD89_E_TRUNCATED;
        }
        layer->mask_ref.present = mask_len != 0U;
        layer->mask_ref.raw_offset = block_start;
        layer->mask_ref.raw_size = 4U + mask_len;
        rc = psd89_parse_layer_mask_data(layer, io, mask_len);
        if (rc != PSD89_OK) {
            return rc;
        }
        block_start = (psd89_u32)psd89_io_tell(io);
        if (!psd89_rd_be32(io, &blend_len)) {
            return PSD89_E_TRUNCATED;
        }
        layer->blend_ranges_ref.raw_offset = block_start;
        layer->blend_ranges_ref.raw_size = 4U + blend_len;
        if (!psd89_skip(io, (psd89_u32)blend_len)) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_read_pascal_string(io, layer->name, PSD89_MAX_NAME_CHARS + 1U, 4U)) {
            return PSD89_E_TRUNCATED;
        }
        while (psd89_io_tell(io) < extra_end) {
            if (layer->tag_count >= PSD89_MAX_TAG_BLOCKS_PER_LAYER) {
                psd89_tag_ref throwaway;
                rc = psd89_parse_tag_block(&throwaway, io, layer);
            } else {
                rc = psd89_parse_tag_block(&layer->tags[layer->tag_count], io, layer);
                if (rc == PSD89_OK) {
                    ++layer->tag_count;
                }
            }
            if (rc != PSD89_OK) {
                return rc;
            }
        }
        if (psd89_io_tell(io) != extra_end) {
            return PSD89_E_BAD_LAYER;
        }
    }

    for (i = 0U; i < abs_count; ++i) {
        psd89_layer *layer;
        layer = &doc->layers[i];
        for (j = 0U; j < layer->channel_count; ++j) {
            if (layer->channels[j].data_length < 2U) {
                return PSD89_E_BAD_LAYER;
            }
            layer->channels[j].data_offset = psd89_io_tell(io);
            if (!psd89_rd_be16(io, &layer->channels[j].compression)) {
                return PSD89_E_TRUNCATED;
            }
            if (!psd89_skip(io, (psd89_u32)(layer->channels[j].data_length - 2U))) {
                return PSD89_E_TRUNCATED;
            }
        }
    }

    if (psd89_io_tell(io) > layer_info_end) {
        return PSD89_E_BAD_LAYER;
    }
    if (psd89_io_tell(io) < layer_info_end && !psd89_io_seek(io, layer_info_end)) {
        return PSD89_E_TRUNCATED;
    }
    return PSD89_OK;
}

static int psd89_parse_layer_and_mask(psd89_doc *doc, psd89_io *io, psd89_u32 end)
{
    psd89_u32 layer_info_len;
    psd89_u32 global_mask_len;
    psd89_u32 layer_info_end;
    int rc;

    if (psd89_io_tell(io) == end) {
        return PSD89_OK;
    }
    if (!psd89_rd_be32(io, &layer_info_len)) {
        return PSD89_E_TRUNCATED;
    }
    layer_info_end = psd89_io_tell(io) + (psd89_u32)layer_info_len;
    if (layer_info_end > end) {
        return PSD89_E_BAD_LAYER;
    }
    if (layer_info_len != 0U) {
        rc = psd89_parse_layers(doc, io, layer_info_end);
        if (rc != PSD89_OK) {
            return rc;
        }
    }
    doc->global_mask_raw_offset = psd89_io_tell(io);
    if (!psd89_rd_be32(io, &global_mask_len)) {
        return PSD89_E_TRUNCATED;
    }
    doc->global_mask_raw_size = 4U + global_mask_len;
    if (!psd89_skip(io, (psd89_u32)global_mask_len)) {
        return PSD89_E_TRUNCATED;
    }
    while (psd89_io_tell(io) < end) {
        psd89_tag_ref tmp;
        rc = psd89_parse_tag_block(&tmp, io, 0);
        if (rc != PSD89_OK) {
            return rc;
        }
        if (doc->global_tag_count < PSD89_MAX_GLOBAL_TAG_BLOCKS) {
            doc->global_tags[doc->global_tag_count] = tmp;
            ++doc->global_tag_count;
        }
    }
    if (psd89_io_tell(io) != end) {
        return PSD89_E_BAD_LAYER;
    }
    return PSD89_OK;
}

int psd89_read(psd89_doc *doc, psd89_io *io)
{
    psd89_u32 sig;
    psd89_u16 version;
    psd89_u8 reserved[6];
    psd89_u32 len;
    psd89_u32 section_end;
    int rc;

    if (doc == 0 || io == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    psd89_doc_init(doc);
    if (!psd89_rd_be32(io, &sig)) {
        return PSD89_E_TRUNCATED;
    }
    if (sig != PSD89_SIG_8BPS) {
        return PSD89_E_BAD_SIGNATURE;
    }
    if (!psd89_rd_be16(io, &version)) {
        return PSD89_E_TRUNCATED;
    }
    if (version != 1U) {
        return PSD89_E_BAD_VERSION;
    }
    if (!psd89_io_read(io, reserved, 6U)) {
        return PSD89_E_TRUNCATED;
    }
    if (reserved[0] != 0U || reserved[1] != 0U || reserved[2] != 0U ||
        reserved[3] != 0U || reserved[4] != 0U || reserved[5] != 0U) {
        return PSD89_E_BAD_RESERVED;
    }
    if (!psd89_rd_be16(io, &doc->channels) ||
        !psd89_rd_be32(io, &doc->height) ||
        !psd89_rd_be32(io, &doc->width) ||
        !psd89_rd_be16(io, &doc->depth) ||
        !psd89_rd_be16(io, &doc->color_mode)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_mode_channel_ok(doc->color_mode, doc->channels)) {
        return PSD89_E_UNSUPPORTED_CHANNELS;
    }
    if (doc->depth != 8U) {
        return PSD89_E_UNSUPPORTED_DEPTH;
    }
    if (doc->color_mode != PSD89_MODE_GRAYSCALE && doc->color_mode != PSD89_MODE_RGB) {
        return PSD89_E_UNSUPPORTED_MODE;
    }

    if (!psd89_rd_be32(io, &len)) {
        return PSD89_E_TRUNCATED;
    }
    if (!psd89_skip(io, (psd89_u32)len)) {
        return PSD89_E_TRUNCATED;
    }

    if (!psd89_rd_be32(io, &len)) {
        return PSD89_E_TRUNCATED;
    }
    section_end = psd89_io_tell(io) + (psd89_u32)len;
    rc = psd89_parse_image_resources(doc, io, section_end);
    if (rc != PSD89_OK) {
        return rc;
    }

    if (!psd89_rd_be32(io, &len)) {
        return PSD89_E_TRUNCATED;
    }
    section_end = psd89_io_tell(io) + (psd89_u32)len;
    rc = psd89_parse_layer_and_mask(doc, io, section_end);
    if (rc != PSD89_OK) {
        return rc;
    }

    doc->composite_data_offset = psd89_io_tell(io);
    if (!psd89_rd_be16(io, &doc->composite_compression)) {
        return PSD89_E_TRUNCATED;
    }
    if (doc->composite_compression != PSD89_COMP_RAW && doc->composite_compression != PSD89_COMP_RLE &&
        doc->composite_compression != PSD89_COMP_ZIP && doc->composite_compression != PSD89_COMP_ZIP_PRED) {
        return PSD89_E_UNSUPPORTED_COMPRESSION;
    }
    doc->has_passthrough_source = 1;
    doc->passthrough_source = *io;
    return PSD89_OK;
}

static int psd89_decode_plane_raw(psd89_io *io, psd89_u8 *dst, psd89_u32 stride, psd89_u32 rows, psd89_u32 row_bytes)
{
    psd89_u32 y;
    for (y = 0U; y < rows; ++y) {
        if (!psd89_io_read(io, dst + (psd89_u32)y * stride, (psd89_u32)row_bytes)) {
            return 0;
        }
    }
    return 1;
}

static int psd89_decode_plane_rle(psd89_io *io,
                                  psd89_u32 counts_offset,
                                  psd89_u32 data_offset,
                                  psd89_u8 *dst,
                                  psd89_u32 stride,
                                  psd89_u32 rows,
                                  psd89_u32 row_bytes)
{
    psd89_u32 y;
    psd89_u16 count16;
    psd89_u32 count_pos;
    psd89_u32 data_pos;
    data_pos = data_offset;
    for (y = 0U; y < rows; ++y) {
        count_pos = counts_offset + (psd89_u32)y * 2U;
        if (!psd89_io_seek(io, count_pos) || !psd89_rd_be16(io, &count16)) {
            return 0;
        }
        if (!psd89_io_seek(io, data_pos)) {
            return 0;
        }
        if (!psd89_packbits_decode_row(io,
                                       dst + (psd89_u32)y * stride,
                                       (psd89_u32)row_bytes,
                                       (psd89_u32)count16)) {
            return 0;
        }
        data_pos += (psd89_u32)count16;
    }
    return 1;
}

static psd89_u32 psd89_io_length(psd89_io *io)
{
    psd89_u32 pos;
    psd89_u32 end;

    if (io == 0 || io->seek == 0 || io->tell == 0) {
        return 0U;
    }
    pos = psd89_io_tell(io);
    if (!psd89_io_seek(io, 0xFFFFFFFFU)) {
        return 0U;
    }
    end = psd89_io_tell(io);
    if (!psd89_io_seek(io, pos)) {
        return 0U;
    }
    return end;
}

int psd89_decode_composite_u8(const psd89_doc *doc, psd89_io *io, psd89_u8 **planes, psd89_u32 stride)
{
    psd89_u16 comp;
    psd89_u32 base;
    psd89_u32 counts_offset;
    psd89_u32 data_offset;
    psd89_u32 c;
    psd89_u32 y;
    psd89_u16 count16;
    psd89_u32 count_pos;
    psd89_u32 row_data_pos;

    if (doc == 0 || io == 0 || planes == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (!psd89_io_seek(io, doc->composite_data_offset) || !psd89_rd_be16(io, &comp)) {
        return PSD89_E_TRUNCATED;
    }
    base = doc->composite_data_offset + 2U;
    if (comp == PSD89_COMP_RAW) {
        for (c = 0U; c < doc->channels; ++c) {
            if (planes[c] == 0) {
                return PSD89_E_BAD_ARGUMENT;
            }
            if (!psd89_decode_plane_raw(io, planes[c], stride, doc->height, doc->width)) {
                return PSD89_E_TRUNCATED;
            }
        }
        return PSD89_OK;
    }
    if (comp == PSD89_COMP_RLE) {
        counts_offset = base;
        data_offset = counts_offset + (psd89_u32)doc->height * (psd89_u32)doc->channels * 2U;
        row_data_pos = data_offset;
        for (c = 0U; c < doc->channels; ++c) {
            if (planes[c] == 0) {
                return PSD89_E_BAD_ARGUMENT;
            }
            for (y = 0U; y < doc->height; ++y) {
                count_pos = counts_offset + ((psd89_u32)c * (psd89_u32)doc->height + (psd89_u32)y) * 2U;
                if (!psd89_io_seek(io, count_pos) || !psd89_rd_be16(io, &count16)) {
                    return PSD89_E_TRUNCATED;
                }
                if (!psd89_io_seek(io, row_data_pos)) {
                    return PSD89_E_TRUNCATED;
                }
                if (!psd89_packbits_decode_row(io,
                                               planes[c] + (psd89_u32)y * stride,
                                               (psd89_u32)doc->width,
                                               (psd89_u32)count16)) {
                    return PSD89_E_RLE;
                }
                row_data_pos += (psd89_u32)count16;
            }
        }
        return PSD89_OK;
    }
    if (comp == PSD89_COMP_ZIP || comp == PSD89_COMP_ZIP_PRED) {
        psd89_u32 file_size;
        file_size = psd89_io_length(io);
        if (file_size <= base) {
            return PSD89_E_TRUNCATED;
        }
        if (!psd89_zip_decode_composite(io, base, file_size - base, planes, stride, doc->channels, doc->height, doc->width, comp == PSD89_COMP_ZIP_PRED)) {
            return PSD89_E_IO;
        }
        return PSD89_OK;
    }
    return PSD89_E_UNSUPPORTED_COMPRESSION;
}

int psd89_decode_layer_channel_u8(const psd89_doc *doc,
                                  psd89_io *io,
                                  unsigned int layer_index,
                                  psd89_s16 channel_id,
                                  psd89_u8 *dst,
                                  psd89_u32 stride)
{
    const psd89_layer *layer;
    const psd89_layer_channel *ch;
    psd89_u32 rows;
    psd89_u32 cols;
    psd89_u16 comp;
    unsigned int i;
    psd89_u32 base;
    psd89_u32 counts_offset;
    psd89_u32 data_offset;

    if (doc == 0 || io == 0 || dst == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (layer_index >= doc->layer_count) {
        return PSD89_E_BAD_ARGUMENT;
    }
    layer = &doc->layers[layer_index];
    ch = 0;
    for (i = 0U; i < layer->channel_count; ++i) {
        if (layer->channels[i].id == channel_id) {
            ch = &layer->channels[i];
            break;
        }
    }
    if (ch == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    if (!psd89_layer_channel_dims(layer, channel_id, &rows, &cols)) {
        return PSD89_E_BAD_LAYER;
    }
    if (!psd89_io_seek(io, ch->data_offset) || !psd89_rd_be16(io, &comp)) {
        return PSD89_E_TRUNCATED;
    }
    if (comp == PSD89_COMP_RAW) {
        if (!psd89_decode_plane_raw(io, dst, stride, rows, cols)) {
            return PSD89_E_TRUNCATED;
        }
        return PSD89_OK;
    }
    if (comp == PSD89_COMP_RLE) {
        base = ch->data_offset + 2U;
        counts_offset = base;
        data_offset = base + (psd89_u32)rows * 2U;
        if (!psd89_decode_plane_rle(io, counts_offset, data_offset, dst, stride, rows, cols)) {
            return PSD89_E_RLE;
        }
        return PSD89_OK;
    }
    if (comp == PSD89_COMP_ZIP || comp == PSD89_COMP_ZIP_PRED) {
        base = ch->data_offset + 2U;
        if (!psd89_zip_decode_plane(io, base, (psd89_u32)(ch->data_length - 2U), dst, stride, rows, cols, comp == PSD89_COMP_ZIP_PRED)) {
            return PSD89_E_IO;
        }
        return PSD89_OK;
    }
    return PSD89_E_UNSUPPORTED_COMPRESSION;
}
