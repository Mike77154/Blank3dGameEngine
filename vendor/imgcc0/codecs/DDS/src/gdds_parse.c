#include "gdds_internal.h"

static gdds_parse_mode gdds__parse_mode_or_default(const gdds_parse_options* options) {
    if (options && options->mode == GDDS_PARSE_MODE_STRICT) {
        return GDDS_PARSE_MODE_STRICT;
    }
    return GDDS_PARSE_MODE_PERMISSIVE;
}

static gdds_result gdds__warn_or_fail(gdds_u32* warning_flags,
                                      gdds_u32 flag,
                                      gdds_parse_mode mode) {
    if (warning_flags) {
        *warning_flags |= flag;
    }
    return (mode == GDDS_PARSE_MODE_STRICT) ? GDDS_RESULT_UNSUPPORTED : GDDS_RESULT_OK;
}

static int gdds__validate_masked_format(gdds_u32 bits_per_pixel,
                                        gdds_u32 rmask,
                                        gdds_u32 gmask,
                                        gdds_u32 bmask,
                                        gdds_u32 amask) {
    gdds_u32 rgb_mask = rmask | gmask | bmask;

    if (bits_per_pixel != 16u && bits_per_pixel != 24u && bits_per_pixel != 32u) {
        return 0;
    }
    if (rgb_mask == 0u) {
        return 0;
    }
    if (!gdds__mask_fits_bits(rmask, bits_per_pixel) ||
        !gdds__mask_fits_bits(gmask, bits_per_pixel) ||
        !gdds__mask_fits_bits(bmask, bits_per_pixel) ||
        !gdds__mask_fits_bits(amask, bits_per_pixel)) {
        return 0;
    }
    if (!gdds__mask_is_contiguous(rmask) ||
        !gdds__mask_is_contiguous(gmask) ||
        !gdds__mask_is_contiguous(bmask) ||
        !gdds__mask_is_contiguous(amask)) {
        return 0;
    }
    if ((rmask & gmask) != 0u || (rmask & bmask) != 0u || (gmask & bmask) != 0u) {
        return 0;
    }
    if ((amask & rgb_mask) != 0u) {
        return 0;
    }

    return 1;
}

static gdds_result gdds__map_dx10_format(gdds_u32 dxgi, gdds_parsed* p) {
    switch (dxgi) {
        case GDDS__DXGI_R8G8B8A8_UNORM:
            p->storage = GDDS__STORAGE_RGBA8;
            p->public_format = GDDS_FORMAT_RGBA8;
            p->bits_per_pixel = 32u;
            p->rmask = 0x000000FFu;
            p->gmask = 0x0000FF00u;
            p->bmask = 0x00FF0000u;
            p->amask = 0xFF000000u;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_R8G8B8A8_UNORM_SRGB:
            p->storage = GDDS__STORAGE_RGBA8;
            p->public_format = GDDS_FORMAT_RGBA8_SRGB;
            p->is_srgb = 1;
            p->bits_per_pixel = 32u;
            p->rmask = 0x000000FFu;
            p->gmask = 0x0000FF00u;
            p->bmask = 0x00FF0000u;
            p->amask = 0xFF000000u;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_B8G8R8A8_UNORM:
            p->storage = GDDS__STORAGE_BGRA8;
            p->public_format = GDDS_FORMAT_BGRA8;
            p->bits_per_pixel = 32u;
            p->rmask = 0x00FF0000u;
            p->gmask = 0x0000FF00u;
            p->bmask = 0x000000FFu;
            p->amask = 0xFF000000u;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_B8G8R8A8_UNORM_SRGB:
            p->storage = GDDS__STORAGE_BGRA8;
            p->public_format = GDDS_FORMAT_BGRA8_SRGB;
            p->is_srgb = 1;
            p->bits_per_pixel = 32u;
            p->rmask = 0x00FF0000u;
            p->gmask = 0x0000FF00u;
            p->bmask = 0x000000FFu;
            p->amask = 0xFF000000u;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC1_UNORM:
            p->storage = GDDS__STORAGE_DXT1;
            p->public_format = GDDS_FORMAT_DXT1;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC1_UNORM_SRGB:
            p->storage = GDDS__STORAGE_DXT1;
            p->public_format = GDDS_FORMAT_DXT1_SRGB;
            p->is_srgb = 1;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC2_UNORM:
            p->storage = GDDS__STORAGE_DXT3;
            p->public_format = GDDS_FORMAT_DXT3;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC2_UNORM_SRGB:
            p->storage = GDDS__STORAGE_DXT3;
            p->public_format = GDDS_FORMAT_DXT3_SRGB;
            p->is_srgb = 1;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC3_UNORM:
            p->storage = GDDS__STORAGE_DXT5;
            p->public_format = GDDS_FORMAT_DXT5;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC3_UNORM_SRGB:
            p->storage = GDDS__STORAGE_DXT5;
            p->public_format = GDDS_FORMAT_DXT5_SRGB;
            p->is_srgb = 1;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC4_UNORM:
            p->storage = GDDS__STORAGE_BC4;
            p->public_format = GDDS_FORMAT_BC4_UNORM;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC4_SNORM:
            p->storage = GDDS__STORAGE_BC4_SNORM;
            p->public_format = GDDS_FORMAT_BC4_SNORM;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC5_UNORM:
            p->storage = GDDS__STORAGE_BC5;
            p->public_format = GDDS_FORMAT_BC5_UNORM;
            return GDDS_RESULT_OK;

        case GDDS__DXGI_BC5_SNORM:
            p->storage = GDDS__STORAGE_BC5_SNORM;
            p->public_format = GDDS_FORMAT_BC5_SNORM;
            return GDDS_RESULT_OK;

        default:
            return GDDS_RESULT_UNSUPPORTED;
    }
}

static gdds_result gdds__map_legacy_format(gdds_u32 pf_flags,
                                           gdds_u32 pf_fourcc,
                                           gdds_u32 pf_rgb_bits,
                                           gdds_u32 pf_rmask,
                                           gdds_u32 pf_gmask,
                                           gdds_u32 pf_bmask,
                                           gdds_u32 pf_amask,
                                           gdds_parsed* p) {
    p->alpha_mode = GDDS_ALPHA_MODE_UNKNOWN;

    if ((pf_flags & GDDS__DDPF_FOURCC) != 0u) {
        switch (pf_fourcc) {
            case GDDS__FOURCC_DXT1:
                p->storage = GDDS__STORAGE_DXT1;
                p->public_format = GDDS_FORMAT_DXT1;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_DXT2:
                p->storage = GDDS__STORAGE_DXT3;
                p->public_format = GDDS_FORMAT_DXT3;
                p->alpha_mode = GDDS_ALPHA_MODE_PREMULTIPLIED;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_DXT3:
                p->storage = GDDS__STORAGE_DXT3;
                p->public_format = GDDS_FORMAT_DXT3;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_DXT4:
                p->storage = GDDS__STORAGE_DXT5;
                p->public_format = GDDS_FORMAT_DXT5;
                p->alpha_mode = GDDS_ALPHA_MODE_PREMULTIPLIED;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_DXT5:
                p->storage = GDDS__STORAGE_DXT5;
                p->public_format = GDDS_FORMAT_DXT5;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_ATI1:
                p->storage = GDDS__STORAGE_BC4;
                p->public_format = GDDS_FORMAT_BC4_UNORM;
                return GDDS_RESULT_OK;

            case GDDS__FOURCC_ATI2:
                p->storage = GDDS__STORAGE_BC5;
                p->public_format = GDDS_FORMAT_BC5_UNORM;
                return GDDS_RESULT_OK;

            default:
                return GDDS_RESULT_UNSUPPORTED;
        }
    }

    if ((pf_flags & GDDS__DDPF_RGB) == 0u) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (!gdds__validate_masked_format(pf_rgb_bits, pf_rmask, pf_gmask, pf_bmask, pf_amask)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    p->storage = GDDS__STORAGE_MASKED_UNCOMPRESSED;
    p->bits_per_pixel = pf_rgb_bits;
    p->rmask = pf_rmask;
    p->gmask = pf_gmask;
    p->bmask = pf_bmask;
    p->amask = pf_amask;

    if (pf_rgb_bits == 32u &&
        pf_rmask == 0x000000FFu &&
        pf_gmask == 0x0000FF00u &&
        pf_bmask == 0x00FF0000u &&
        pf_amask == 0xFF000000u) {
        p->storage = GDDS__STORAGE_RGBA8;
        p->public_format = GDDS_FORMAT_RGBA8;
    } else if (pf_rgb_bits == 32u &&
               pf_rmask == 0x00FF0000u &&
               pf_gmask == 0x0000FF00u &&
               pf_bmask == 0x000000FFu &&
               pf_amask == 0xFF000000u) {
        p->storage = GDDS__STORAGE_BGRA8;
        p->public_format = GDDS_FORMAT_BGRA8;
    }

    return GDDS_RESULT_OK;
}

static void gdds__note_pitch_warning(const gdds_parsed* p, gdds_u32* warning_flags) {
    gdds_size canonical = 0;

    if (!p || !warning_flags || p->pitch_or_linear_size == 0u) {
        return;
    }

    if (gdds__storage_is_block_compressed(p->storage)) {
        if (!gdds__calc_linear_size_block_size(p->width,
                                               p->height,
                                               gdds__block_size_for_storage(p->storage),
                                               &canonical)) {
            return;
        }
    } else {
        if (!gdds__calc_pitch_uncompressed_size(p->width, p->bits_per_pixel, &canonical)) {
            return;
        }
    }

    if ((gdds_size)p->pitch_or_linear_size != canonical) {
        *warning_flags |= GDDS_WARNING_INCONSISTENT_PITCH;
    }
}

static gdds_result gdds__validate_payload_size(gdds_size dds_size, gdds_parsed* p) {
    gdds_u32 level;
    gdds_u32 width;
    gdds_u32 height;
    gdds_size total_bytes = 0;
    gdds_size level_bytes;
    gdds_size required_size;

    if (!p) return GDDS_RESULT_INVALID_ARGUMENT;

    if (!gdds__calc_top_level_span_for_decode(p, &level_bytes)) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    p->top_level_size = level_bytes;
    total_bytes = level_bytes;

    width = gdds__mip_dim(p->width);
    height = gdds__mip_dim(p->height);

    for (level = 1u; level < p->mip_count; ++level) {
        if (!gdds__calc_storage_level_size(p->storage,
                                           p->bits_per_pixel,
                                           width,
                                           height,
                                           &level_bytes)) {
            return GDDS_RESULT_UNSUPPORTED;
        }
        if (!gdds__add_size(total_bytes, level_bytes, &total_bytes)) {
            return GDDS_RESULT_UNSUPPORTED;
        }
        width = gdds__mip_dim(width);
        height = gdds__mip_dim(height);
    }

    p->full_chain_size = total_bytes;

    if (!gdds__add_size(p->pixel_offset, total_bytes, &required_size)) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (dds_size < required_size) {
        return GDDS_RESULT_TRUNCATED;
    }

    return GDDS_RESULT_OK;
}

gdds_result gdds__parse_internal(const void* dds_data, gdds_size dds_size, gdds_parsed* out) {
    return gdds__parse_internal_ex(dds_data, dds_size, NULL, out);
}

gdds_result gdds__parse_internal_ex(const void* dds_data,
                                    gdds_size dds_size,
                                    const gdds_parse_options* options,
                                    gdds_parsed* out) {
    const gdds_u8* data = (const gdds_u8*)dds_data;
    gdds_parse_mode mode = gdds__parse_mode_or_default(options);
    gdds_u32 magic;
    gdds_u32 header_size;
    gdds_u32 flags;
    gdds_u32 width;
    gdds_u32 height;
    gdds_u32 pitch_or_linear_size;
    gdds_u32 depth;
    gdds_u32 mip_count;
    gdds_u32 pf_size;
    gdds_u32 pf_flags;
    gdds_u32 pf_fourcc;
    gdds_u32 pf_rgb_bits;
    gdds_u32 pf_rmask;
    gdds_u32 pf_gmask;
    gdds_u32 pf_bmask;
    gdds_u32 pf_amask;
    gdds_u32 caps;
    gdds_u32 caps2;
    gdds_u32 caps3;
    gdds_u32 caps4;
    gdds_u32 reserved2;
    gdds_u32 warning_flags = 0u;
    int reserved_nonzero = 0;
    gdds_size i;
    gdds_parsed p;
    gdds_result rc;

    if (!dds_data || !out) return GDDS_RESULT_INVALID_ARGUMENT;
    if (dds_size < 128u) return GDDS_RESULT_TRUNCATED;

    magic = gdds__read_u32le(data + 0u);
    if (magic != GDDS__DDS_MAGIC) return GDDS_RESULT_NOT_DDS;

    header_size = gdds__read_u32le(data + 4u);
    if (header_size != 124u) return GDDS_RESULT_UNSUPPORTED;

    flags = gdds__read_u32le(data + 8u);
    height = gdds__read_u32le(data + 12u);
    width = gdds__read_u32le(data + 16u);
    pitch_or_linear_size = gdds__read_u32le(data + 20u);
    depth = gdds__read_u32le(data + 24u);
    mip_count = gdds__read_u32le(data + 28u);
    pf_size = gdds__read_u32le(data + 76u);
    pf_flags = gdds__read_u32le(data + 80u);
    pf_fourcc = gdds__read_u32le(data + 84u);
    pf_rgb_bits = gdds__read_u32le(data + 88u);
    pf_rmask = gdds__read_u32le(data + 92u);
    pf_gmask = gdds__read_u32le(data + 96u);
    pf_bmask = gdds__read_u32le(data + 100u);
    pf_amask = gdds__read_u32le(data + 104u);
    caps = gdds__read_u32le(data + 108u);
    caps2 = gdds__read_u32le(data + 112u);
    caps3 = gdds__read_u32le(data + 116u);
    caps4 = gdds__read_u32le(data + 120u);
    reserved2 = gdds__read_u32le(data + 124u);

    if (pf_size != 32u) return GDDS_RESULT_UNSUPPORTED;
    if (width == 0u || height == 0u) return GDDS_RESULT_UNSUPPORTED;

    for (i = 0; i < 11u; ++i) {
        if (gdds__read_u32le(data + 32u + (gdds_u32)(i * 4u)) != 0u) {
            reserved_nonzero = 1;
            break;
        }
    }
    if (caps3 != 0u || caps4 != 0u || reserved2 != 0u) {
        reserved_nonzero = 1;
    }

    if (mip_count == 0u) {
        mip_count = 1u;
    }
    if (mip_count > gdds__max_mip_count_2d(width, height)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    if ((caps2 & GDDS__DDSCAPS2_VOLUME) != 0u) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (depth > 1u || (flags & GDDS__DDSD_DEPTH) != 0u) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    memset(&p, 0, sizeof(p));
    p.width = width;
    p.height = height;
    p.mip_count = mip_count;
    p.pitch_or_linear_size = pitch_or_linear_size;
    p.pixel_offset = 128u;
    p.bits_per_pixel = pf_rgb_bits;
    p.rmask = pf_rmask;
    p.gmask = pf_gmask;
    p.bmask = pf_bmask;
    p.amask = pf_amask;
    p.alpha_mode = GDDS_ALPHA_MODE_UNKNOWN;
    p.storage = GDDS__STORAGE_UNKNOWN;
    p.public_format = GDDS_FORMAT_UNKNOWN;

    if ((flags & (GDDS__DDSD_CAPS | GDDS__DDSD_HEIGHT | GDDS__DDSD_WIDTH | GDDS__DDSD_PIXELFORMAT)) !=
        (GDDS__DDSD_CAPS | GDDS__DDSD_HEIGHT | GDDS__DDSD_WIDTH | GDDS__DDSD_PIXELFORMAT)) {
        rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_MISSING_HEADER_FLAGS, mode);
        if (rc != GDDS_RESULT_OK) return rc;
    }

    if ((caps & GDDS__DDSCAPS_TEXTURE) == 0u) {
        rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_MISSING_TEXTURE_CAPS, mode);
        if (rc != GDDS_RESULT_OK) return rc;
    }

    if (mip_count > 1u) {
        if ((flags & GDDS__DDSD_MIPMAPCOUNT) == 0u ||
            (caps & (GDDS__DDSCAPS_COMPLEX | GDDS__DDSCAPS_MIPMAP)) !=
                (GDDS__DDSCAPS_COMPLEX | GDDS__DDSCAPS_MIPMAP)) {
            rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_MISSING_MIPMAP_FLAGS, mode);
            if (rc != GDDS_RESULT_OK) return rc;
        }
    }

    if (reserved_nonzero) {
        rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_RESERVED_HEADER_FIELDS, mode);
        if (rc != GDDS_RESULT_OK) return rc;
    }

    if ((pf_flags & GDDS__DDPF_FOURCC) != 0u && pf_fourcc == GDDS__FOURCC_DX10) {
        gdds_u32 dxgi;
        gdds_u32 resource_dimension;
        gdds_u32 misc_flag;
        gdds_u32 array_size;
        gdds_u32 misc_flags2;
        gdds_u32 alpha_mode_bits;

        if (dds_size < 148u) return GDDS_RESULT_TRUNCATED;

        dxgi = gdds__read_u32le(data + 128u);
        resource_dimension = gdds__read_u32le(data + 132u);
        misc_flag = gdds__read_u32le(data + 136u);
        array_size = gdds__read_u32le(data + 140u);
        misc_flags2 = gdds__read_u32le(data + 144u);

        p.pixel_offset = 148u;
        p.has_dx10_header = 1;

        if ((misc_flag & ~GDDS__DDS_RESOURCE_MISC_TEXTURECUBE) != 0u) {
            rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_DX10_RESERVED_BITS, mode);
            if (rc != GDDS_RESULT_OK) return rc;
        }

        if ((misc_flags2 & ~GDDS__DDS_ALPHA_MODE_MASK) != 0u) {
            rc = gdds__warn_or_fail(&warning_flags, GDDS_WARNING_DX10_RESERVED_BITS, mode);
            if (rc != GDDS_RESULT_OK) return rc;
        }

        alpha_mode_bits = misc_flags2 & GDDS__DDS_ALPHA_MODE_MASK;
        if (alpha_mode_bits > GDDS__DDS_ALPHA_MODE_CUSTOM) {
            return GDDS_RESULT_UNSUPPORTED;
        }
        p.alpha_mode = (gdds_alpha_mode)alpha_mode_bits;

        if (array_size != 1u) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        switch (resource_dimension) {
            case GDDS__DDS_DIMENSION_TEXTURE2D:
                if ((misc_flag & GDDS__DDS_RESOURCE_MISC_TEXTURECUBE) != 0u) {
                    return GDDS_RESULT_UNSUPPORTED;
                }
                break;

            case GDDS__DDS_DIMENSION_TEXTURE1D:
            case GDDS__DDS_DIMENSION_TEXTURE3D:
            default:
                return GDDS_RESULT_UNSUPPORTED;
        }

        rc = gdds__map_dx10_format(dxgi, &p);
        if (rc != GDDS_RESULT_OK) return rc;
    } else {
        if (caps2 != 0u) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        rc = gdds__map_legacy_format(pf_flags,
                                     pf_fourcc,
                                     pf_rgb_bits,
                                     pf_rmask,
                                     pf_gmask,
                                     pf_bmask,
                                     pf_amask,
                                     &p);
        if (rc != GDDS_RESULT_OK) return rc;
    }

    gdds__note_pitch_warning(&p, &warning_flags);
    p.warning_flags = warning_flags;

    rc = gdds__validate_payload_size(dds_size, &p);
    if (rc != GDDS_RESULT_OK) return rc;

    *out = p;
    return GDDS_RESULT_OK;
}

gdds_result gdds_inspect_memory(const void* dds_data,
                                gdds_size dds_size,
                                gdds_info* out_info) {
    return gdds_inspect_memory_ex(dds_data, dds_size, NULL, out_info);
}

gdds_result gdds_inspect_memory_ex(const void* dds_data,
                                   gdds_size dds_size,
                                   const gdds_parse_options* options,
                                   gdds_info* out_info) {
    gdds_parsed p;
    gdds_result rc;

    if (!out_info) return GDDS_RESULT_INVALID_ARGUMENT;
    memset(out_info, 0, sizeof(*out_info));

    rc = gdds__parse_internal_ex(dds_data, dds_size, options, &p);
    if (rc != GDDS_RESULT_OK) return rc;

    out_info->width = p.width;
    out_info->height = p.height;
    out_info->mip_count = p.mip_count;
    out_info->source_format = p.public_format;
    out_info->alpha_mode = p.alpha_mode;
    out_info->warning_flags = p.warning_flags;
    out_info->data_offset = p.pixel_offset;
    out_info->top_level_size = p.top_level_size;
    out_info->full_chain_size = p.full_chain_size;
    out_info->is_srgb = p.is_srgb;
    out_info->has_dx10_header = p.has_dx10_header;
    return GDDS_RESULT_OK;
}
