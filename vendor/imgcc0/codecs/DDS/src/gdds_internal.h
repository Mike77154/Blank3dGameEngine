#ifndef GDDS_INTERNAL_H
#define GDDS_INTERNAL_H

#include "giffany_dds/gdds.h"

#include <string.h>


extern gdds_u8 gdds__static_decode_rgba[GDDS_STATIC_RGBA_CAPACITY];
extern gdds_u8 gdds__static_encode_dds[GDDS_STATIC_DDS_CAPACITY];
extern gdds_u8 gdds__static_normal_rgba[GDDS_STATIC_RGBA_CAPACITY];
extern gdds_u8 gdds__static_mip_rgba[GDDS_STATIC_MIP_CAPACITY];
extern gdds_generated_mip_level gdds__static_mip_levels[GDDS_MAX_MIP_LEVELS];
extern gdds_mip_level_rgba8 gdds__static_encode_levels[GDDS_MAX_MIP_LEVELS];

#define GDDS__DDS_MAGIC 0x20534444u
#define GDDS__FOURCC_DXT1 0x31545844u
#define GDDS__FOURCC_DXT2 0x32545844u
#define GDDS__FOURCC_DXT3 0x33545844u
#define GDDS__FOURCC_DXT4 0x34545844u
#define GDDS__FOURCC_DXT5 0x35545844u
#define GDDS__FOURCC_DX10 0x30315844u
#define GDDS__FOURCC_ATI1 0x31495441u
#define GDDS__FOURCC_ATI2 0x32495441u

#define GDDS__DDSD_CAPS 0x00000001u
#define GDDS__DDSD_HEIGHT 0x00000002u
#define GDDS__DDSD_WIDTH 0x00000004u
#define GDDS__DDSD_PITCH 0x00000008u
#define GDDS__DDSD_PIXELFORMAT 0x00001000u
#define GDDS__DDSD_MIPMAPCOUNT 0x00020000u
#define GDDS__DDSD_LINEARSIZE 0x00080000u
#define GDDS__DDSD_DEPTH 0x00800000u

#define GDDS__DDSCAPS_COMPLEX 0x00000008u
#define GDDS__DDSCAPS_TEXTURE 0x00001000u
#define GDDS__DDSCAPS_MIPMAP 0x00400000u

#define GDDS__DDSCAPS2_CUBEMAP 0x00000200u
#define GDDS__DDSCAPS2_VOLUME 0x00200000u

#define GDDS__DDPF_ALPHAPIXELS 0x00000001u
#define GDDS__DDPF_ALPHA 0x00000002u
#define GDDS__DDPF_FOURCC 0x00000004u
#define GDDS__DDPF_RGB 0x00000040u

#define GDDS__DXGI_R8G8B8A8_UNORM 28u
#define GDDS__DXGI_R8G8B8A8_UNORM_SRGB 29u
#define GDDS__DXGI_BC1_UNORM 71u
#define GDDS__DXGI_BC1_UNORM_SRGB 72u
#define GDDS__DXGI_BC2_UNORM 74u
#define GDDS__DXGI_BC2_UNORM_SRGB 75u
#define GDDS__DXGI_BC3_UNORM 77u
#define GDDS__DXGI_BC3_UNORM_SRGB 78u
#define GDDS__DXGI_BC4_UNORM 80u
#define GDDS__DXGI_BC4_SNORM 81u
#define GDDS__DXGI_BC5_UNORM 83u
#define GDDS__DXGI_BC5_SNORM 84u
#define GDDS__DXGI_B8G8R8A8_UNORM 87u
#define GDDS__DXGI_B8G8R8A8_UNORM_SRGB 91u

#define GDDS__DDS_DIMENSION_TEXTURE1D 2u
#define GDDS__DDS_DIMENSION_TEXTURE2D 3u
#define GDDS__DDS_DIMENSION_TEXTURE3D 4u
#define GDDS__DDS_RESOURCE_MISC_TEXTURECUBE 0x4u
#define GDDS__DDS_ALPHA_MODE_MASK 0x7u
#define GDDS__DDS_ALPHA_MODE_CUSTOM 0x4u

typedef enum gdds_storage_kind {
    GDDS__STORAGE_UNKNOWN = 0,
    GDDS__STORAGE_RGBA8,
    GDDS__STORAGE_BGRA8,
    GDDS__STORAGE_DXT1,
    GDDS__STORAGE_DXT3,
    GDDS__STORAGE_DXT5,
    GDDS__STORAGE_BC4,
    GDDS__STORAGE_BC5,
    GDDS__STORAGE_BC4_SNORM,
    GDDS__STORAGE_BC5_SNORM,
    GDDS__STORAGE_MASKED_UNCOMPRESSED
} gdds_storage_kind;

typedef struct gdds_parsed {
    gdds_u32 width;
    gdds_u32 height;
    gdds_u32 mip_count;
    gdds_u32 pitch_or_linear_size;
    gdds_size pixel_offset;
    gdds_u32 bits_per_pixel;
    gdds_u32 rmask;
    gdds_u32 gmask;
    gdds_u32 bmask;
    gdds_u32 amask;
    gdds_u32 warning_flags;
    gdds_size top_level_size;
    gdds_size full_chain_size;
    int has_dx10_header;
    int is_srgb;
    gdds_alpha_mode alpha_mode;
    gdds_storage_kind storage;
    gdds_format public_format;
} gdds_parsed;

static int gdds__format_is_srgb(gdds_format format) {
    switch (format) {
        case GDDS_FORMAT_RGBA8_SRGB:
        case GDDS_FORMAT_BGRA8_SRGB:
        case GDDS_FORMAT_DXT1_SRGB:
        case GDDS_FORMAT_DXT3_SRGB:
        case GDDS_FORMAT_DXT5_SRGB:
            return 1;
        default:
            return 0;
    }
}

static int gdds__format_is_snorm(gdds_format format) {
    switch (format) {
        case GDDS_FORMAT_BC4_SNORM:
        case GDDS_FORMAT_BC5_SNORM:
            return 1;
        default:
            return 0;
    }
}

static gdds_format gdds__format_base_variant(gdds_format format) {
    switch (format) {
        case GDDS_FORMAT_RGBA8_SRGB: return GDDS_FORMAT_RGBA8;
        case GDDS_FORMAT_BGRA8_SRGB: return GDDS_FORMAT_BGRA8;
        case GDDS_FORMAT_DXT1_SRGB: return GDDS_FORMAT_DXT1;
        case GDDS_FORMAT_DXT3_SRGB: return GDDS_FORMAT_DXT3;
        case GDDS_FORMAT_DXT5_SRGB: return GDDS_FORMAT_DXT5;
        default: return format;
    }
}

static int gdds__format_supports_encode(gdds_format format) {
    switch (gdds__format_base_variant(format)) {
        case GDDS_FORMAT_RGBA8:
        case GDDS_FORMAT_BGRA8:
        case GDDS_FORMAT_DXT1:
        case GDDS_FORMAT_DXT3:
        case GDDS_FORMAT_DXT5:
        case GDDS_FORMAT_BC4_UNORM:
        case GDDS_FORMAT_BC5_UNORM:
        case GDDS_FORMAT_BC4_SNORM:
        case GDDS_FORMAT_BC5_SNORM:
            return 1;
        default:
            return 0;
    }
}

static int gdds__format_requires_dx10_header(gdds_format format) {
    return gdds__format_is_srgb(format) ||
           format == GDDS_FORMAT_BC4_UNORM ||
           format == GDDS_FORMAT_BC5_UNORM ||
           format == GDDS_FORMAT_BC4_SNORM ||
           format == GDDS_FORMAT_BC5_SNORM;
}

static gdds_u32 gdds__dxgi_from_format(gdds_format format) {
    switch (format) {
        case GDDS_FORMAT_RGBA8: return GDDS__DXGI_R8G8B8A8_UNORM;
        case GDDS_FORMAT_RGBA8_SRGB: return GDDS__DXGI_R8G8B8A8_UNORM_SRGB;
        case GDDS_FORMAT_BGRA8: return GDDS__DXGI_B8G8R8A8_UNORM;
        case GDDS_FORMAT_BGRA8_SRGB: return GDDS__DXGI_B8G8R8A8_UNORM_SRGB;
        case GDDS_FORMAT_DXT1: return GDDS__DXGI_BC1_UNORM;
        case GDDS_FORMAT_DXT1_SRGB: return GDDS__DXGI_BC1_UNORM_SRGB;
        case GDDS_FORMAT_DXT3: return GDDS__DXGI_BC2_UNORM;
        case GDDS_FORMAT_DXT3_SRGB: return GDDS__DXGI_BC2_UNORM_SRGB;
        case GDDS_FORMAT_DXT5: return GDDS__DXGI_BC3_UNORM;
        case GDDS_FORMAT_DXT5_SRGB: return GDDS__DXGI_BC3_UNORM_SRGB;
        case GDDS_FORMAT_BC4_UNORM: return GDDS__DXGI_BC4_UNORM;
        case GDDS_FORMAT_BC4_SNORM: return GDDS__DXGI_BC4_SNORM;
        case GDDS_FORMAT_BC5_UNORM: return GDDS__DXGI_BC5_UNORM;
        case GDDS_FORMAT_BC5_SNORM: return GDDS__DXGI_BC5_SNORM;
        default: return 0u;
    }
}

static gdds_u32 gdds__read_u32le(const gdds_u8* p) {
    return ((gdds_u32)p[0]) |
           ((gdds_u32)p[1] << 8) |
           ((gdds_u32)p[2] << 16) |
           ((gdds_u32)p[3] << 24);
}

static void gdds__write_u16le(gdds_u8* p, gdds_u16 v) {
    p[0] = (gdds_u8)(v & 0xFFu);
    p[1] = (gdds_u8)((v >> 8) & 0xFFu);
}

static void gdds__write_u32le(gdds_u8* p, gdds_u32 v) {
    p[0] = (gdds_u8)(v & 0xFFu);
    p[1] = (gdds_u8)((v >> 8) & 0xFFu);
    p[2] = (gdds_u8)((v >> 16) & 0xFFu);
    p[3] = (gdds_u8)((v >> 24) & 0xFFu);
}

static gdds_u32 gdds__max_u32(gdds_u32 a, gdds_u32 b) {
    return a > b ? a : b;
}

static int gdds__abs_i32(int v) {
    return v < 0 ? -v : v;
}

static int gdds__mul_size(gdds_size a, gdds_size b, gdds_size* out) {
    if (!out) return 0;
    if (a == 0 || b == 0) {
        *out = 0;
        return 1;
    }
    if (a > ((gdds_size)-1) / b) return 0;
    *out = a * b;
    return 1;
}

static int gdds__add_size(gdds_size a, gdds_size b, gdds_size* out) {
    if (!out) return 0;
    if (a > ((gdds_size)-1) - b) return 0;
    *out = a + b;
    return 1;
}

static int gdds__size_to_u32(gdds_size value, gdds_u32* out) {
    if (!out) return 0;
    if (value > 0xFFFFFFFFu) return 0;
    *out = (gdds_u32)value;
    return 1;
}

static int gdds__mask_shift(gdds_u32 mask) {
    int shift = 0;
    if (!mask) return 0;
    while ((mask & 1u) == 0u) {
        mask >>= 1u;
        ++shift;
    }
    return shift;
}

static int gdds__mask_bits(gdds_u32 mask) {
    int bits = 0;
    while (mask) {
        bits += (int)(mask & 1u);
        mask >>= 1u;
    }
    return bits;
}

static int gdds__mask_is_contiguous(gdds_u32 mask) {
    if (!mask) return 1;
    while ((mask & 1u) == 0u) {
        mask >>= 1u;
    }
    while (mask & 1u) {
        mask >>= 1u;
    }
    return mask == 0u;
}

static int gdds__mask_fits_bits(gdds_u32 mask, gdds_u32 bits_per_pixel) {
    if (bits_per_pixel >= 32u) return 1;
    return (mask >> bits_per_pixel) == 0u;
}

static gdds_u8 gdds__scale_to_u8(gdds_u32 value, int bits) {
    gdds_u32 maxv;
    if (bits <= 0) return 0;
    if (bits >= 8) {
        if (bits > 8) {
            maxv = (1u << bits) - 1u;
            return (gdds_u8)((value * 255u + (maxv / 2u)) / maxv);
        }
        return (gdds_u8)value;
    }
    maxv = (1u << bits) - 1u;
    return (gdds_u8)((value * 255u + (maxv / 2u)) / maxv);
}

static gdds_u8 gdds__extract_component(gdds_u32 px, gdds_u32 mask, int is_alpha) {
    int shift;
    int bits;
    gdds_u32 value;
    if (!mask) return (gdds_u8)(is_alpha ? 255 : 0);
    shift = gdds__mask_shift(mask);
    bits = gdds__mask_bits(mask);
    value = (px & mask) >> shift;
    return gdds__scale_to_u8(value, bits);
}

static int gdds__calc_pitch_uncompressed_size(gdds_u32 width,
                                                     gdds_u32 bits_per_pixel,
                                                     gdds_size* out_pitch) {
    gdds_u32 whole_bytes;
    gdds_u32 rem_bits;
    gdds_u32 pitch;
    if (!out_pitch || bits_per_pixel == 0u) return 0;
    whole_bytes = bits_per_pixel / 8u;
    rem_bits = bits_per_pixel & 7u;
    if (whole_bytes != 0u && width > 0xFFFFFFFFu / whole_bytes) return 0;
    pitch = width * whole_bytes;
    if (rem_bits != 0u) {
        gdds_u32 extra_bits;
        if (width > (0xFFFFFFFFu - 7u) / rem_bits) return 0;
        extra_bits = width * rem_bits;
        if (pitch > 0xFFFFFFFFu - ((extra_bits + 7u) / 8u)) return 0;
        pitch += (extra_bits + 7u) / 8u;
    }
    *out_pitch = pitch;
    return 1;
}

static gdds_u32 gdds__calc_pitch_uncompressed(gdds_u32 width, gdds_u32 bits_per_pixel) {
    gdds_size pitch = 0;
    if (!gdds__calc_pitch_uncompressed_size(width, bits_per_pixel, &pitch)) return 0u;
    return (pitch > 0xFFFFFFFFu) ? 0u : (gdds_u32)pitch;
}

static int gdds__calc_linear_size_block_size(gdds_u32 width,
                                                    gdds_u32 height,
                                                    gdds_u32 block_size,
                                                    gdds_size* out_size) {
    gdds_size bw;
    gdds_size bh;
    gdds_size blocks;
    if (!out_size || block_size == 0u) return 0;
    bw = (gdds_size)gdds__max_u32(1u, (width + 3u) / 4u);
    bh = (gdds_size)gdds__max_u32(1u, (height + 3u) / 4u);
    if (!gdds__mul_size(bw, bh, &blocks)) return 0;
    if (!gdds__mul_size(blocks, (gdds_size)block_size, out_size)) return 0;
    return 1;
}

static gdds_u32 gdds__calc_linear_size_block(gdds_u32 width, gdds_u32 height, gdds_u32 block_size) {
    gdds_size bytes = 0;
    if (!gdds__calc_linear_size_block_size(width, height, block_size, &bytes)) return 0u;
    return (bytes > 0xFFFFFFFFu) ? 0u : (gdds_u32)bytes;
}

static gdds_u32 gdds__mip_dim(gdds_u32 v) {
    return v > 1u ? (v >> 1u) : 1u;
}

static gdds_u32 gdds__max_mip_count_2d(gdds_u32 width, gdds_u32 height) {
    gdds_u32 count = 1u;
    while (width > 1u || height > 1u) {
        width = gdds__mip_dim(width);
        height = gdds__mip_dim(height);
        ++count;
    }
    return count;
}

static int gdds__storage_is_block_compressed(gdds_storage_kind storage) {
    return storage == GDDS__STORAGE_DXT1 ||
           storage == GDDS__STORAGE_DXT3 ||
           storage == GDDS__STORAGE_DXT5 ||
           storage == GDDS__STORAGE_BC4 ||
           storage == GDDS__STORAGE_BC5 ||
           storage == GDDS__STORAGE_BC4_SNORM ||
           storage == GDDS__STORAGE_BC5_SNORM;
}

static gdds_u32 gdds__block_size_for_storage(gdds_storage_kind storage) {
    return (storage == GDDS__STORAGE_DXT1 || storage == GDDS__STORAGE_BC4 || storage == GDDS__STORAGE_BC4_SNORM) ? 8u : 16u;
}

static int gdds__calc_storage_level_size(gdds_storage_kind storage,
                                                gdds_u32 bits_per_pixel,
                                                gdds_u32 width,
                                                gdds_u32 height,
                                                gdds_size* out_bytes) {
    if (!out_bytes || width == 0u || height == 0u) {
        return 0;
    }

    if (gdds__storage_is_block_compressed(storage)) {
        return gdds__calc_linear_size_block_size(width,
                                                 height,
                                                 gdds__block_size_for_storage(storage),
                                                 out_bytes);
    }

    {
        gdds_size row_pitch;
        if (!gdds__calc_pitch_uncompressed_size(width, bits_per_pixel, &row_pitch)) {
            return 0;
        }
        return gdds__mul_size(row_pitch, (gdds_size)height, out_bytes);
    }
}

static int gdds__calc_top_level_span_for_decode(const gdds_parsed* p, gdds_size* out_bytes) {
    if (!p || !out_bytes) return 0;

    if (gdds__storage_is_block_compressed(p->storage)) {
        return gdds__calc_linear_size_block_size(p->width,
                                                 p->height,
                                                 gdds__block_size_for_storage(p->storage),
                                                 out_bytes);
    }

    {
        gdds_size row_pitch;
        if (!gdds__calc_pitch_uncompressed_size(p->width, p->bits_per_pixel, &row_pitch)) {
            return 0;
        }
        if ((gdds_size)p->pitch_or_linear_size > row_pitch) {
            row_pitch = (gdds_size)p->pitch_or_linear_size;
        }
        return gdds__mul_size(row_pitch, (gdds_size)p->height, out_bytes);
    }
}

static void gdds__mip_dimensions(gdds_u32 base_width,
                                        gdds_u32 base_height,
                                        gdds_u32 level_index,
                                        gdds_u32* out_width,
                                        gdds_u32* out_height) {
    while (level_index > 0u) {
        base_width = gdds__mip_dim(base_width);
        base_height = gdds__mip_dim(base_height);
        --level_index;
    }
    if (out_width) *out_width = base_width;
    if (out_height) *out_height = base_height;
}

static void gdds__rgb565_to_rgb888(gdds_u16 c, gdds_u8* r, gdds_u8* g, gdds_u8* b) {
    gdds_u32 rr = (gdds_u32)((c >> 11) & 31u);
    gdds_u32 gg = (gdds_u32)((c >> 5) & 63u);
    gdds_u32 bb = (gdds_u32)(c & 31u);
    *r = (gdds_u8)((rr * 255u + 15u) / 31u);
    *g = (gdds_u8)((gg * 255u + 31u) / 63u);
    *b = (gdds_u8)((bb * 255u + 15u) / 31u);
}

static gdds_u16 gdds__rgb888_to_rgb565(gdds_u8 r, gdds_u8 g, gdds_u8 b) {
    gdds_u16 rr = (gdds_u16)((r * 31u + 127u) / 255u);
    gdds_u16 gg = (gdds_u16)((g * 63u + 127u) / 255u);
    gdds_u16 bb = (gdds_u16)((b * 31u + 127u) / 255u);
    return (gdds_u16)((rr << 11) | (gg << 5) | bb);
}

gdds_u16 gdds__srgb8_to_linear12(gdds_u8 value);
gdds_u8 gdds__linear12_to_srgb8(gdds_u16 value);

gdds_result gdds__parse_internal(const void* dds_data,
                                 gdds_size dds_size,
                                 gdds_parsed* out);

gdds_result gdds__parse_internal_ex(const void* dds_data,
                                    gdds_size dds_size,
                                    const gdds_parse_options* options,
                                    gdds_parsed* out);

gdds_result gdds__decode_uncompressed_rgba(const gdds_u8* data,
                                           gdds_size size,
                                           const gdds_parsed* p,
                                           gdds_u8* out_rgba);

gdds_result gdds__decode_block_compressed(const gdds_u8* data,
                                          gdds_size size,
                                          const gdds_parsed* p,
                                          gdds_u8* out_rgba);

gdds_result gdds__get_mip_layout_from_parsed(const gdds_parsed* parsed,
                                             gdds_u32 level_index,
                                             gdds_mip_info* out_mip_info);

void gdds__write_legacy_header(gdds_u8* dst,
                               gdds_u32 width,
                               gdds_u32 height,
                               gdds_format fmt,
                               gdds_u32 pitch_or_linear_size,
                               gdds_u32 mip_count);

gdds_result gdds__calc_payload_size_for_format(gdds_format format,
                                               gdds_u32 width,
                                               gdds_u32 height,
                                               gdds_size* out_size,
                                               gdds_u32* out_pitch_or_linear_size);

gdds_result gdds__encode_surface_rgba8(const gdds_u8* rgba8,
                                       gdds_u32 width,
                                       gdds_u32 height,
                                       gdds_format output_format,
                                       gdds_u8* dst,
                                       gdds_size dst_size);

void gdds__fetch_block_rgba(const gdds_u8* rgba,
                            gdds_u32 width,
                            gdds_u32 height,
                            gdds_u32 bx,
                            gdds_u32 by,
                            gdds_u8 block[16][4]);

void gdds__encode_bc1_color_block(const gdds_u8* block,
                                  int allow_1bit_alpha,
                                  gdds_u8 out_block[8]);

void gdds__encode_bc2_block(const gdds_u8* block, gdds_u8 out_block[16]);
void gdds__encode_bc3_block(const gdds_u8* block, gdds_u8 out_block[16]);
void gdds__encode_bc4_block_from_rgba(const gdds_u8* block, gdds_u32 channel_index, gdds_u8 out_block[8]);
void gdds__encode_bc5_block_from_rgba(const gdds_u8* block, gdds_u32 red_channel_index, gdds_u32 green_channel_index, gdds_u8 out_block[16]);
void gdds__encode_bc4s_block_from_rgba(const gdds_u8* block, gdds_u32 channel_index, gdds_u8 out_block[8]);
void gdds__encode_bc5s_block_from_rgba(const gdds_u8* block, gdds_u32 red_channel_index, gdds_u32 green_channel_index, gdds_u8 out_block[16]);

void gdds__decode_bc1_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc2_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc3_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc4_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc5_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc4s_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__decode_bc5s_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]);
void gdds__block_to_image(const gdds_u8* block_rgba,
                          gdds_u8* dst,
                          gdds_u32 width,
                          gdds_u32 height,
                          gdds_u32 bx,
                          gdds_u32 by);

#endif /* GDDS_INTERNAL_H */
