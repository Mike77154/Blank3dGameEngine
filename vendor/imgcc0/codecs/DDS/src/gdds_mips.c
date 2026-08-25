#include "gdds_internal.h"

gdds_result gdds__get_mip_layout_from_parsed(const gdds_parsed* parsed,
                                             gdds_u32 level_index,
                                             gdds_mip_info* out_mip_info) {
    gdds_u32 level;
    gdds_u32 width;
    gdds_u32 height;
    gdds_size offset;
    gdds_size level_size;

    if (!parsed || !out_mip_info) return GDDS_RESULT_INVALID_ARGUMENT;
    if (level_index >= parsed->mip_count) return GDDS_RESULT_INVALID_ARGUMENT;

    memset(out_mip_info, 0, sizeof(*out_mip_info));

    offset = parsed->pixel_offset;
    width = parsed->width;
    height = parsed->height;

    for (level = 0u; level < level_index; ++level) {
        if (level == 0u) {
            level_size = parsed->top_level_size;
        } else if (!gdds__calc_storage_level_size(parsed->storage,
                                                  parsed->bits_per_pixel,
                                                  width,
                                                  height,
                                                  &level_size)) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        if (!gdds__add_size(offset, level_size, &offset)) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        width = gdds__mip_dim(width);
        height = gdds__mip_dim(height);
    }

    if (level_index == 0u) {
        level_size = parsed->top_level_size;
    } else if (!gdds__calc_storage_level_size(parsed->storage,
                                              parsed->bits_per_pixel,
                                              width,
                                              height,
                                              &level_size)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    out_mip_info->level_index = level_index;
    out_mip_info->width = width;
    out_mip_info->height = height;
    out_mip_info->data_offset = offset;
    out_mip_info->data_size = level_size;
    return GDDS_RESULT_OK;
}

gdds_result gdds_get_mip_info(const void* dds_data,
                              gdds_size dds_size,
                              gdds_u32 level_index,
                              gdds_mip_info* out_mip_info) {
    return gdds_get_mip_info_ex(dds_data, dds_size, level_index, NULL, out_mip_info);
}

gdds_result gdds_get_mip_info_ex(const void* dds_data,
                                 gdds_size dds_size,
                                 gdds_u32 level_index,
                                 const gdds_parse_options* options,
                                 gdds_mip_info* out_mip_info) {
    gdds_parsed parsed;
    gdds_result rc;

    if (!out_mip_info) return GDDS_RESULT_INVALID_ARGUMENT;

    rc = gdds__parse_internal_ex(dds_data, dds_size, options, &parsed);
    if (rc != GDDS_RESULT_OK) return rc;

    return gdds__get_mip_layout_from_parsed(&parsed, level_index, out_mip_info);
}

gdds_result gdds_decode_mip_memory(const void* dds_data,
                                   gdds_size dds_size,
                                   gdds_u32 level_index,
                                   gdds_image* out_image) {
    return gdds_decode_mip_memory_ex(dds_data, dds_size, level_index, NULL, out_image);
}

gdds_result gdds_decode_mip_memory_ex(const void* dds_data,
                                      gdds_size dds_size,
                                      gdds_u32 level_index,
                                      const gdds_parse_options* options,
                                      gdds_image* out_image) {
    gdds_parsed parsed;
    gdds_parsed level_parsed;
    gdds_mip_info mip_info;
    gdds_size pixel_count;
    gdds_size byte_count;
    gdds_u8* rgba;
    gdds_result rc;

    if (!dds_data || !out_image) return GDDS_RESULT_INVALID_ARGUMENT;
    memset(out_image, 0, sizeof(*out_image));

    rc = gdds__parse_internal_ex(dds_data, dds_size, options, &parsed);
    if (rc != GDDS_RESULT_OK) return rc;

    rc = gdds__get_mip_layout_from_parsed(&parsed, level_index, &mip_info);
    if (rc != GDDS_RESULT_OK) return rc;

    if (!gdds__mul_size((gdds_size)mip_info.width, (gdds_size)mip_info.height, &pixel_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (!gdds__mul_size(pixel_count, 4u, &byte_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    if (byte_count > GDDS_STATIC_RGBA_CAPACITY) return GDDS_RESULT_ALLOC;
    rgba = gdds__static_decode_rgba;

    level_parsed = parsed;
    level_parsed.width = mip_info.width;
    level_parsed.height = mip_info.height;
    level_parsed.pixel_offset = mip_info.data_offset;
    if (level_index != 0u && !gdds__storage_is_block_compressed(level_parsed.storage)) {
        level_parsed.pitch_or_linear_size = 0u;
    }

    if (level_parsed.storage == GDDS__STORAGE_RGBA8 ||
        level_parsed.storage == GDDS__STORAGE_BGRA8 ||
        level_parsed.storage == GDDS__STORAGE_MASKED_UNCOMPRESSED) {
        rc = gdds__decode_uncompressed_rgba((const gdds_u8*)dds_data, dds_size, &level_parsed, rgba);
    } else if (gdds__storage_is_block_compressed(level_parsed.storage)) {
        rc = gdds__decode_block_compressed((const gdds_u8*)dds_data, dds_size, &level_parsed, rgba);
    } else {
        rc = GDDS_RESULT_UNSUPPORTED;
    }

    if (rc != GDDS_RESULT_OK) {
        return rc;
    }

    out_image->width = mip_info.width;
    out_image->height = mip_info.height;
    out_image->pixel_format = GDDS_FORMAT_RGBA8;
    out_image->source_format = parsed.public_format;
    out_image->alpha_mode = parsed.alpha_mode;
    out_image->is_srgb = parsed.is_srgb;
    out_image->pixels = rgba;
    out_image->size = byte_count;
    return GDDS_RESULT_OK;
}
