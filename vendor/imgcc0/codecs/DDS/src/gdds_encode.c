#include "gdds_internal.h"

void gdds__write_legacy_header(gdds_u8* dst,
                               gdds_u32 width,
                               gdds_u32 height,
                               gdds_format fmt,
                               gdds_u32 pitch_or_linear_size,
                               gdds_u32 mip_count) {
    gdds_u32 flags = GDDS__DDSD_CAPS | GDDS__DDSD_HEIGHT | GDDS__DDSD_WIDTH | GDDS__DDSD_PIXELFORMAT;
    gdds_u32 pf_flags = 0u;
    gdds_u32 pf_fourcc = 0u;
    gdds_u32 pf_rgb_bits = 0u;
    gdds_u32 pf_rmask = 0u;
    gdds_u32 pf_gmask = 0u;
    gdds_u32 pf_bmask = 0u;
    gdds_u32 pf_amask = 0u;
    gdds_u32 caps = GDDS__DDSCAPS_TEXTURE;
    gdds_format base_format = gdds__format_base_variant(fmt);

    memset(dst, 0, 128u);
    gdds__write_u32le(dst + 0, GDDS__DDS_MAGIC);
    gdds__write_u32le(dst + 4, 124u);

    if (base_format == GDDS_FORMAT_RGBA8 || base_format == GDDS_FORMAT_BGRA8) {
        flags |= GDDS__DDSD_PITCH;
        pf_flags = GDDS__DDPF_RGB | GDDS__DDPF_ALPHAPIXELS;
        pf_rgb_bits = 32u;
        if (base_format == GDDS_FORMAT_RGBA8) {
            pf_rmask = 0x000000FFu;
            pf_gmask = 0x0000FF00u;
            pf_bmask = 0x00FF0000u;
            pf_amask = 0xFF000000u;
        } else {
            pf_rmask = 0x00FF0000u;
            pf_gmask = 0x0000FF00u;
            pf_bmask = 0x000000FFu;
            pf_amask = 0xFF000000u;
        }
    } else {
        flags |= GDDS__DDSD_LINEARSIZE;
        pf_flags = GDDS__DDPF_FOURCC;
        if (base_format == GDDS_FORMAT_DXT1) pf_fourcc = GDDS__FOURCC_DXT1;
        if (base_format == GDDS_FORMAT_DXT3) pf_fourcc = GDDS__FOURCC_DXT3;
        if (base_format == GDDS_FORMAT_DXT5) pf_fourcc = GDDS__FOURCC_DXT5;
    }

    if (mip_count > 1u) {
        flags |= GDDS__DDSD_MIPMAPCOUNT;
        caps |= GDDS__DDSCAPS_COMPLEX | GDDS__DDSCAPS_MIPMAP;
    }

    gdds__write_u32le(dst + 8, flags);
    gdds__write_u32le(dst + 12, height);
    gdds__write_u32le(dst + 16, width);
    gdds__write_u32le(dst + 20, pitch_or_linear_size);
    gdds__write_u32le(dst + 24, 0u);
    gdds__write_u32le(dst + 28, (mip_count > 1u) ? mip_count : 0u);

    gdds__write_u32le(dst + 76, 32u);
    gdds__write_u32le(dst + 80, pf_flags);
    gdds__write_u32le(dst + 84, pf_fourcc);
    gdds__write_u32le(dst + 88, pf_rgb_bits);
    gdds__write_u32le(dst + 92, pf_rmask);
    gdds__write_u32le(dst + 96, pf_gmask);
    gdds__write_u32le(dst + 100, pf_bmask);
    gdds__write_u32le(dst + 104, pf_amask);

    gdds__write_u32le(dst + 108, caps);
    gdds__write_u32le(dst + 112, 0u);
    gdds__write_u32le(dst + 116, 0u);
    gdds__write_u32le(dst + 120, 0u);
    gdds__write_u32le(dst + 124, 0u);
}

static void gdds__write_dx10_header(gdds_u8* dst,
                                    gdds_u32 width,
                                    gdds_u32 height,
                                    gdds_format fmt,
                                    gdds_u32 pitch_or_linear_size,
                                    gdds_u32 mip_count) {
    gdds_u32 flags = GDDS__DDSD_CAPS | GDDS__DDSD_HEIGHT | GDDS__DDSD_WIDTH | GDDS__DDSD_PIXELFORMAT;
    gdds_u32 caps = GDDS__DDSCAPS_TEXTURE;
    gdds_u32 dxgi_format = gdds__dxgi_from_format(fmt);
    gdds_format base_format = gdds__format_base_variant(fmt);

    memset(dst, 0, 148u);
    gdds__write_u32le(dst + 0, GDDS__DDS_MAGIC);
    gdds__write_u32le(dst + 4, 124u);

    if (base_format == GDDS_FORMAT_RGBA8 || base_format == GDDS_FORMAT_BGRA8) {
        flags |= GDDS__DDSD_PITCH;
    } else {
        flags |= GDDS__DDSD_LINEARSIZE;
    }

    if (mip_count > 1u) {
        flags |= GDDS__DDSD_MIPMAPCOUNT;
        caps |= GDDS__DDSCAPS_COMPLEX | GDDS__DDSCAPS_MIPMAP;
    }

    gdds__write_u32le(dst + 8, flags);
    gdds__write_u32le(dst + 12, height);
    gdds__write_u32le(dst + 16, width);
    gdds__write_u32le(dst + 20, pitch_or_linear_size);
    gdds__write_u32le(dst + 24, 0u);
    gdds__write_u32le(dst + 28, (mip_count > 1u) ? mip_count : 0u);

    gdds__write_u32le(dst + 76, 32u);
    gdds__write_u32le(dst + 80, GDDS__DDPF_FOURCC);
    gdds__write_u32le(dst + 84, GDDS__FOURCC_DX10);
    gdds__write_u32le(dst + 88, 0u);
    gdds__write_u32le(dst + 92, 0u);
    gdds__write_u32le(dst + 96, 0u);
    gdds__write_u32le(dst + 100, 0u);
    gdds__write_u32le(dst + 104, 0u);

    gdds__write_u32le(dst + 108, caps);
    gdds__write_u32le(dst + 112, 0u);
    gdds__write_u32le(dst + 116, 0u);
    gdds__write_u32le(dst + 120, 0u);
    gdds__write_u32le(dst + 124, 0u);

    gdds__write_u32le(dst + 128, dxgi_format);
    gdds__write_u32le(dst + 132, GDDS__DDS_DIMENSION_TEXTURE2D);
    gdds__write_u32le(dst + 136, 0u);
    gdds__write_u32le(dst + 140, 1u);
    gdds__write_u32le(dst + 144, 0u);
}

gdds_result gdds__calc_payload_size_for_format(gdds_format format,
                                               gdds_u32 width,
                                               gdds_u32 height,
                                               gdds_size* out_size,
                                               gdds_u32* out_pitch_or_linear_size) {
    gdds_size payload_size = 0;
    gdds_u32 pitch_or_linear_size = 0u;
    gdds_format base_format = gdds__format_base_variant(format);

    if (!out_size || !out_pitch_or_linear_size || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    switch (base_format) {
        case GDDS_FORMAT_RGBA8:
        case GDDS_FORMAT_BGRA8:
            if (!gdds__calc_pitch_uncompressed_size(width, 32u, &payload_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            if (!gdds__size_to_u32(payload_size, &pitch_or_linear_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            if (!gdds__mul_size(payload_size, (gdds_size)height, &payload_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            break;

        case GDDS_FORMAT_DXT1:
        case GDDS_FORMAT_BC4_UNORM:
        case GDDS_FORMAT_BC4_SNORM:
            if (!gdds__calc_linear_size_block_size(width, height, 8u, &payload_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            if (!gdds__size_to_u32(payload_size, &pitch_or_linear_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            break;

        case GDDS_FORMAT_DXT3:
        case GDDS_FORMAT_DXT5:
        case GDDS_FORMAT_BC5_UNORM:
        case GDDS_FORMAT_BC5_SNORM:
            if (!gdds__calc_linear_size_block_size(width, height, 16u, &payload_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            if (!gdds__size_to_u32(payload_size, &pitch_or_linear_size)) {
                return GDDS_RESULT_UNSUPPORTED;
            }
            break;

        default:
            return GDDS_RESULT_UNSUPPORTED;
    }

    *out_size = payload_size;
    *out_pitch_or_linear_size = pitch_or_linear_size;
    return GDDS_RESULT_OK;
}

gdds_result gdds__encode_surface_rgba8(const gdds_u8* rgba8,
                                       gdds_u32 width,
                                       gdds_u32 height,
                                       gdds_format output_format,
                                       gdds_u8* dst,
                                       gdds_size dst_size) {
    gdds_size expected_size;
    gdds_u32 throwaway_pitch;
    gdds_format base_format = gdds__format_base_variant(output_format);

    if (!rgba8 || !dst) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    if (gdds__calc_payload_size_for_format(base_format,
                                           width,
                                           height,
                                           &expected_size,
                                           &throwaway_pitch) != GDDS_RESULT_OK) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (dst_size < expected_size) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    if (base_format == GDDS_FORMAT_RGBA8 || base_format == GDDS_FORMAT_BGRA8) {
        gdds_size row_pitch;
        gdds_u32 y, x;

        if (!gdds__calc_pitch_uncompressed_size(width, 32u, &row_pitch)) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        for (y = 0; y < height; ++y) {
            gdds_u8* row = dst + (gdds_size)y * row_pitch;
            for (x = 0; x < width; ++x) {
                const gdds_u8* src_px = rgba8 + (((gdds_size)y * width + x) * 4u);
                gdds_u8* dst_px = row + (gdds_size)x * 4u;
                if (base_format == GDDS_FORMAT_RGBA8) {
                    dst_px[0] = src_px[0];
                    dst_px[1] = src_px[1];
                    dst_px[2] = src_px[2];
                    dst_px[3] = src_px[3];
                } else {
                    dst_px[0] = src_px[2];
                    dst_px[1] = src_px[1];
                    dst_px[2] = src_px[0];
                    dst_px[3] = src_px[3];
                }
            }
        }
    } else {
        gdds_u32 bw = gdds__max_u32(1u, (width + 3u) / 4u);
        gdds_u32 bh = gdds__max_u32(1u, (height + 3u) / 4u);
        gdds_u32 by, bx;
        gdds_u32 block_size = (base_format == GDDS_FORMAT_DXT1 || base_format == GDDS_FORMAT_BC4_UNORM || base_format == GDDS_FORMAT_BC4_SNORM) ? 8u : 16u;
        for (by = 0; by < bh; ++by) {
            for (bx = 0; bx < bw; ++bx) {
                gdds_u8 block_rgba[16][4];
                gdds_u8* out_block = dst + (((gdds_size)by * bw) + bx) * block_size;
                gdds__fetch_block_rgba(rgba8, width, height, bx, by, block_rgba);
                if (base_format == GDDS_FORMAT_DXT1) {
                    gdds__encode_bc1_color_block(&block_rgba[0][0], 1, out_block);
                } else if (base_format == GDDS_FORMAT_DXT3) {
                    gdds__encode_bc2_block(&block_rgba[0][0], out_block);
                } else if (base_format == GDDS_FORMAT_DXT5) {
                    gdds__encode_bc3_block(&block_rgba[0][0], out_block);
                } else if (base_format == GDDS_FORMAT_BC4_UNORM) {
                    gdds__encode_bc4_block_from_rgba(&block_rgba[0][0], 0u, out_block);
                } else if (base_format == GDDS_FORMAT_BC4_SNORM) {
                    gdds__encode_bc4s_block_from_rgba(&block_rgba[0][0], 0u, out_block);
                } else if (base_format == GDDS_FORMAT_BC5_UNORM) {
                    gdds__encode_bc5_block_from_rgba(&block_rgba[0][0], 0u, 1u, out_block);
                } else {
                    gdds__encode_bc5s_block_from_rgba(&block_rgba[0][0], 0u, 1u, out_block);
                }
            }
        }
    }

    return GDDS_RESULT_OK;
}

gdds_result gdds_encode_mipchain_memory_rgba8(const gdds_mip_level_rgba8* levels,
                                              gdds_u32 level_count,
                                              gdds_u32 base_width,
                                              gdds_u32 base_height,
                                              const gdds_encode_options* options,
                                              gdds_buffer* out_buffer) {
    gdds_format output_format;
    gdds_u32 max_level_count;
    gdds_u32 level_width;
    gdds_u32 level_height;
    gdds_size payload_total = 0;
    gdds_size level_size;
    gdds_size total_size;
    gdds_size header_size;
    gdds_u32 top_pitch_or_linear_size = 0u;
    gdds_u8* dst;
    gdds_u8* payload;
    gdds_u32 level;
    gdds_result rc;

    if (!levels || !options || !out_buffer || base_width == 0u || base_height == 0u || level_count == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    memset(out_buffer, 0, sizeof(*out_buffer));
    output_format = options->format;
    if (!gdds__format_supports_encode(output_format)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    max_level_count = gdds__max_mip_count_2d(base_width, base_height);
    if (level_count > max_level_count) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    level_width = base_width;
    level_height = base_height;
    for (level = 0u; level < level_count; ++level) {
        gdds_size expected_rgba_size;

        if (!levels[level].pixels) {
            return GDDS_RESULT_INVALID_ARGUMENT;
        }
        if (!gdds__mul_size((gdds_size)level_width, (gdds_size)level_height, &expected_rgba_size) ||
            !gdds__mul_size(expected_rgba_size, 4u, &expected_rgba_size)) {
            return GDDS_RESULT_UNSUPPORTED;
        }
        if (levels[level].size < expected_rgba_size) {
            return GDDS_RESULT_INVALID_ARGUMENT;
        }

        rc = gdds__calc_payload_size_for_format(output_format,
                                                level_width,
                                                level_height,
                                                &level_size,
                                                &top_pitch_or_linear_size);
        if (rc != GDDS_RESULT_OK) {
            return rc;
        }
        if (!gdds__add_size(payload_total, level_size, &payload_total)) {
            return GDDS_RESULT_UNSUPPORTED;
        }

        level_width = gdds__mip_dim(level_width);
        level_height = gdds__mip_dim(level_height);
    }

    rc = gdds__calc_payload_size_for_format(output_format,
                                            base_width,
                                            base_height,
                                            &level_size,
                                            &top_pitch_or_linear_size);
    if (rc != GDDS_RESULT_OK) {
        return rc;
    }

    header_size = gdds__format_requires_dx10_header(output_format) ? 148u : 128u;
    if (!gdds__add_size(header_size, payload_total, &total_size)) return GDDS_RESULT_UNSUPPORTED;
    if (total_size > GDDS_STATIC_DDS_CAPACITY) return GDDS_RESULT_ALLOC;
    dst = gdds__static_encode_dds;

    if (gdds__format_requires_dx10_header(output_format)) {
        gdds__write_dx10_header(dst,
                                base_width,
                                base_height,
                                output_format,
                                top_pitch_or_linear_size,
                                level_count);
    } else {
        gdds__write_legacy_header(dst,
                                  base_width,
                                  base_height,
                                  output_format,
                                  top_pitch_or_linear_size,
                                  level_count);
    }

    payload = dst + header_size;
    level_width = base_width;
    level_height = base_height;
    for (level = 0u; level < level_count; ++level) {
        gdds_u32 ignored_pitch = 0u;

        rc = gdds__calc_payload_size_for_format(output_format,
                                                level_width,
                                                level_height,
                                                &level_size,
                                                &ignored_pitch);
        if (rc != GDDS_RESULT_OK) {
            return rc;
        }

        rc = gdds__encode_surface_rgba8(levels[level].pixels,
                                        level_width,
                                        level_height,
                                        output_format,
                                        payload,
                                        level_size);
        if (rc != GDDS_RESULT_OK) {
            return rc;
        }

        payload += level_size;
        level_width = gdds__mip_dim(level_width);
        level_height = gdds__mip_dim(level_height);
    }

    out_buffer->data = dst;
    out_buffer->size = total_size;
    return GDDS_RESULT_OK;
}

gdds_result gdds_encode_memory_rgba8(const gdds_u8* rgba8,
                                     gdds_u32 width,
                                     gdds_u32 height,
                                     const gdds_encode_options* options,
                                     gdds_buffer* out_buffer) {
    gdds_mip_level_rgba8 level;
    gdds_size level_size;

    if (!rgba8 || !options || !out_buffer || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    if (!gdds__mul_size((gdds_size)width, (gdds_size)height, &level_size) ||
        !gdds__mul_size(level_size, 4u, &level_size)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    level.pixels = rgba8;
    level.size = level_size;
    return gdds_encode_mipchain_memory_rgba8(&level,
                                             1u,
                                             width,
                                             height,
                                             options,
                                             out_buffer);
}
