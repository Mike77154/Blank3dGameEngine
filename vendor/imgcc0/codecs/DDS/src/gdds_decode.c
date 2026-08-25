#include "gdds_internal.h"

gdds_result gdds__decode_uncompressed_rgba(const gdds_u8* data,
                                           gdds_size size,
                                           const gdds_parsed* p,
                                           gdds_u8* out_rgba) {
    gdds_size row_pitch;
    gdds_size header_pitch = (gdds_size)p->pitch_or_linear_size;
    gdds_size bytes_per_pixel;
    gdds_size needed;
    gdds_u32 y, x;

    if (!gdds__calc_pitch_uncompressed_size(p->width, p->bits_per_pixel, &row_pitch)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    bytes_per_pixel = (gdds_size)((p->bits_per_pixel + 7u) / 8u);
    if (bytes_per_pixel == 0u) return GDDS_RESULT_UNSUPPORTED;
    if (header_pitch >= row_pitch) row_pitch = header_pitch;

    if (!gdds__mul_size(row_pitch, (gdds_size)p->height, &needed)) return GDDS_RESULT_UNSUPPORTED;
    if (!gdds__add_size(p->pixel_offset, needed, &needed)) return GDDS_RESULT_UNSUPPORTED;
    if (size < needed) return GDDS_RESULT_TRUNCATED;

    for (y = 0; y < p->height; ++y) {
        const gdds_u8* src_row = data + p->pixel_offset + row_pitch * (gdds_size)y;
        gdds_u8* dst_row = out_rgba + ((gdds_size)y * p->width * 4u);
        for (x = 0; x < p->width; ++x) {
            gdds_u32 raw = 0;
            const gdds_u8* src_px = src_row + (gdds_size)x * bytes_per_pixel;
            gdds_u8* dst_px = dst_row + (gdds_size)x * 4u;
            gdds_size b;
            for (b = 0; b < bytes_per_pixel; ++b) raw |= ((gdds_u32)src_px[b]) << (8u * b);
            dst_px[0] = gdds__extract_component(raw, p->rmask, 0);
            dst_px[1] = gdds__extract_component(raw, p->gmask, 0);
            dst_px[2] = gdds__extract_component(raw, p->bmask, 0);
            dst_px[3] = gdds__extract_component(raw, p->amask, 1);
        }
    }
    return GDDS_RESULT_OK;
}

gdds_result gdds__decode_block_compressed(const gdds_u8* data,
                                          gdds_size size,
                                          const gdds_parsed* p,
                                          gdds_u8* out_rgba) {
    gdds_u32 block_size = gdds__block_size_for_storage(p->storage);
    gdds_u32 bw = gdds__max_u32(1u, (p->width + 3u) / 4u);
    gdds_u32 bh = gdds__max_u32(1u, (p->height + 3u) / 4u);
    gdds_size needed;
    gdds_u32 by, bx;

    if (!gdds__calc_linear_size_block_size(p->width, p->height, block_size, &needed)) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (!gdds__add_size(p->pixel_offset, needed, &needed)) return GDDS_RESULT_UNSUPPORTED;
    if (size < needed) return GDDS_RESULT_TRUNCATED;

    for (by = 0; by < bh; ++by) {
        for (bx = 0; bx < bw; ++bx) {
            gdds_u8 block_rgba[16][4];
            const gdds_u8* block = data + p->pixel_offset + (((gdds_size)by * bw) + bx) * block_size;
            if (p->storage == GDDS__STORAGE_DXT1) {
                gdds__decode_bc1_block(block, block_rgba);
            } else if (p->storage == GDDS__STORAGE_DXT3) {
                gdds__decode_bc2_block(block, block_rgba);
            } else if (p->storage == GDDS__STORAGE_DXT5) {
                gdds__decode_bc3_block(block, block_rgba);
            } else if (p->storage == GDDS__STORAGE_BC4) {
                gdds__decode_bc4_block(block, block_rgba);
            } else if (p->storage == GDDS__STORAGE_BC5) {
                gdds__decode_bc5_block(block, block_rgba);
            } else if (p->storage == GDDS__STORAGE_BC4_SNORM) {
                gdds__decode_bc4s_block(block, block_rgba);
            } else {
                gdds__decode_bc5s_block(block, block_rgba);
            }
            gdds__block_to_image(&block_rgba[0][0], out_rgba, p->width, p->height, bx, by);
        }
    }

    return GDDS_RESULT_OK;
}

gdds_result gdds_decode_memory(const void* dds_data,
                               gdds_size dds_size,
                               gdds_image* out_image) {
    return gdds_decode_memory_ex(dds_data, dds_size, NULL, out_image);
}

gdds_result gdds_decode_memory_ex(const void* dds_data,
                                  gdds_size dds_size,
                                  const gdds_parse_options* options,
                                  gdds_image* out_image) {
    return gdds_decode_mip_memory_ex(dds_data, dds_size, 0u, options, out_image);
}
