#include "gdds_internal.h"


static gdds_normal_map_decode_options gdds__normal_decode_options_or_default(const gdds_normal_map_decode_options* options) {
    if (options) return *options;
    return gdds_normal_map_decode_options_default();
}

static gdds_normal_map_encode_options gdds__normal_encode_options_or_default(const gdds_normal_map_encode_options* options) {
    if (options) return *options;
    return gdds_normal_map_encode_options_default();
}

static int gdds__normal_convention_valid(gdds_normal_map_convention convention) {
    return convention == GDDS_NORMAL_MAP_CONVENTION_DIRECTX ||
           convention == GDDS_NORMAL_MAP_CONVENTION_OPENGL ||
           convention == GDDS_NORMAL_MAP_CONVENTION_GLTF;
}

static int gdds__normal_decode_options_valid(const gdds_normal_map_decode_options* options) {
    if (!options) return 0;
    return gdds__normal_convention_valid(options->stored_convention) &&
           gdds__normal_convention_valid(options->output_convention);
}

static int gdds__normal_encode_options_valid(const gdds_normal_map_encode_options* options) {
    if (!options) return 0;
    if (!(options->input_layout == GDDS_NORMAL_MAP_LAYOUT_XY_RG ||
          options->input_layout == GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB)) {
        return 0;
    }
    if (!gdds__normal_convention_valid(options->input_convention) ||
        !gdds__normal_convention_valid(options->output_convention)) {
        return 0;
    }
    return options->output_format == GDDS_FORMAT_BC5_UNORM ||
           options->output_format == GDDS_FORMAT_BC5_SNORM;
}

static int gdds__normal_map_source_format_is_bc5(gdds_format format) {
    return format == GDDS_FORMAT_BC5_UNORM || format == GDDS_FORMAT_BC5_SNORM;
}

#define GDDS__NORM_SHIFT 13
#define GDDS__NORM_ONE 8192

static gdds_s32 gdds__round_div_s32(gdds_s32 n, gdds_s32 d) {
    if (n >= 0) return (n + d / 2) / d;
    return -(((-n) + d / 2) / d);
}

static gdds_u32 gdds__isqrt_u32(gdds_u32 value) {
    gdds_u32 op = value;
    gdds_u32 res = 0u;
    gdds_u32 one = 1u << 30u;
    while (one > op) one >>= 2u;
    while (one != 0u) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1u) + one;
        } else {
            res >>= 1u;
        }
        one >>= 2u;
    }
    return res;
}

static gdds_u8 gdds__normal_pack_ratio(gdds_s32 component_num, gdds_u32 sum_sq) {
    gdds_u32 scale = 1u;
    gdds_u32 scaled_sum;
    gdds_u32 root;
    gdds_s32 qnum;
    gdds_s32 numer;
    gdds_s32 denom;
    if (sum_sq == 0u) return 128u;
    while (scale < 32768u) {
        gdds_u32 next = scale << 1u;
        gdds_u32 next_sq = next * next;
        if (sum_sq > 0xFFFFFFFFu / next_sq) break;
        scale = next;
    }
    scaled_sum = sum_sq * scale * scale;
    root = gdds__isqrt_u32(scaled_sum);
    if (root < 65535u) {
        gdds_u32 rem = scaled_sum - root * root;
        if (rem > root) ++root;
    }
    if (root == 0u) return 128u;
    qnum = component_num * (gdds_s32)scale;
    numer = ((gdds_s32)root + qnum) * 255;
    denom = (gdds_s32)root * 2;
    if (numer <= 0) return 0u;
    if (numer >= denom * 255) return 255u;
    return (gdds_u8)((numer + denom / 2) / denom);
}

static gdds_s32 gdds__normal_unpack_u8(gdds_u8 value) {
    gdds_s32 n = (gdds_s32)value * 2 - 255;
    return gdds__round_div_s32(n * GDDS__NORM_ONE, 255);
}

static gdds_u8 gdds__normal_pack_u8(gdds_s32 value) {
    gdds_s32 n;
    gdds_s32 d = GDDS__NORM_ONE * 2;
    if (value < -GDDS__NORM_ONE) value = -GDDS__NORM_ONE;
    if (value > GDDS__NORM_ONE) value = GDDS__NORM_ONE;
    n = (value + GDDS__NORM_ONE) * 255;
    return (gdds_u8)((n + d / 2) / d);
}

static gdds_u32 gdds__square_s32(gdds_s32 v) {
    gdds_u32 a = (gdds_u32)(v < 0 ? -v : v);
    return a * a;
}

static void gdds__normal_reconstruct_z_from_xy(gdds_s32* x, gdds_s32* y, gdds_s32* z) {
    gdds_u32 xy_len_sq;
    gdds_u32 one_sq = (gdds_u32)GDDS__NORM_ONE * (gdds_u32)GDDS__NORM_ONE;
    if (!x || !y || !z) return;
    xy_len_sq = gdds__square_s32(*x) + gdds__square_s32(*y);
    if (xy_len_sq > one_sq) {
        gdds_u32 len = gdds__isqrt_u32(xy_len_sq);
        if (len != 0u) {
            *x = gdds__round_div_s32(*x * GDDS__NORM_ONE, (gdds_s32)len);
            *y = gdds__round_div_s32(*y * GDDS__NORM_ONE, (gdds_s32)len);
        }
        *z = 0;
        return;
    }
    *z = (gdds_s32)gdds__isqrt_u32(one_sq - xy_len_sq);
}

static void gdds__normal_normalize_xyz(gdds_s32* x, gdds_s32* y, gdds_s32* z) {
    gdds_u32 len_sq;
    gdds_u32 len;
    if (!x || !y || !z) return;
    len_sq = gdds__square_s32(*x) + gdds__square_s32(*y) + gdds__square_s32(*z);
    if (len_sq == 0u) {
        *x = 0; *y = 0; *z = GDDS__NORM_ONE;
        return;
    }
    len = gdds__isqrt_u32(len_sq);
    if (len == 0u) {
        *x = 0; *y = 0; *z = GDDS__NORM_ONE;
        return;
    }
    *x = gdds__round_div_s32(*x * GDDS__NORM_ONE, (gdds_s32)len);
    *y = gdds__round_div_s32(*y * GDDS__NORM_ONE, (gdds_s32)len);
    *z = gdds__round_div_s32(*z * GDDS__NORM_ONE, (gdds_s32)len);
}

static void gdds__load_normal_from_rgb(const gdds_u8* rgba, gdds_s32* x, gdds_s32* y, gdds_s32* z) {
    *x = gdds__normal_unpack_u8(rgba[0]);
    *y = gdds__normal_unpack_u8(rgba[1]);
    *z = gdds__normal_unpack_u8(rgba[2]);
}

static void gdds__store_normal_to_rgb(gdds_u8* rgba, gdds_s32 x, gdds_s32 y, gdds_s32 z) {
    rgba[0] = gdds__normal_pack_u8(x);
    rgba[1] = gdds__normal_pack_u8(y);
    rgba[2] = gdds__normal_pack_u8(z);
}

static void gdds__prepare_normal_pixel(const gdds_u8* src_rgba,
                                       const gdds_normal_map_encode_options* options,
                                       gdds_u8* dst_rgba) {
    gdds_s32 x = gdds__normal_unpack_u8(src_rgba[0]);
    gdds_s32 y = gdds__normal_unpack_u8(src_rgba[1]);
    gdds_s32 z;
    if (options->input_layout == GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB) z = gdds__normal_unpack_u8(src_rgba[2]);
    else gdds__normal_reconstruct_z_from_xy(&x, &y, &z);
    if (options->normalize) gdds__normal_normalize_xyz(&x, &y, &z);
    if (gdds_normal_map_convention_needs_y_flip(options->input_convention, options->output_convention)) y = -y;
    if (options->invert_y) y = -y;
    gdds__store_normal_to_rgb(dst_rgba, x, y, z);
    dst_rgba[3] = src_rgba[3];
}

static gdds_result gdds__alloc_rgba8_image(gdds_u32 width,
                                           gdds_u32 height,
                                           gdds_u8** out_pixels,
                                           gdds_size* out_size) {
    gdds_size pixel_count;
    gdds_size byte_count;
    gdds_u8* pixels;

    if (!out_pixels || !out_size || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds__mul_size((gdds_size)width, (gdds_size)height, &pixel_count) ||
        !gdds__mul_size(pixel_count, 4u, &byte_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }
    if (byte_count > GDDS_STATIC_RGBA_CAPACITY) {
        return GDDS_RESULT_ALLOC;
    }
    pixels = gdds__static_normal_rgba;
    *out_pixels = pixels;
    *out_size = byte_count;
    return GDDS_RESULT_OK;
}

static gdds_result gdds__prepare_normal_map_rgba8(const gdds_u8* rgba8,
                                                  gdds_u32 width,
                                                  gdds_u32 height,
                                                  const gdds_normal_map_encode_options* options,
                                                  gdds_u8** out_pixels,
                                                  gdds_size* out_size) {
    gdds_normal_map_encode_options opt;
    gdds_u8* dst;
    gdds_size byte_count;
    gdds_size pixel_count;
    gdds_size i;
    gdds_result rc;

    if (!rgba8 || !out_pixels || !out_size || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    opt = gdds__normal_encode_options_or_default(options);
    if (!gdds__normal_encode_options_valid(&opt)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    rc = gdds__alloc_rgba8_image(width, height, &dst, &byte_count);
    if (rc != GDDS_RESULT_OK) {
        return rc;
    }

    pixel_count = byte_count / 4u;
    for (i = 0u; i < pixel_count; ++i) {
        gdds__prepare_normal_pixel(rgba8 + i * 4u, &opt, dst + i * 4u);
    }

    *out_pixels = dst;
    *out_size = byte_count;
    return GDDS_RESULT_OK;
}

static gdds_u8 gdds__avg4_u8(gdds_u8 a, gdds_u8 b, gdds_u8 c, gdds_u8 d) {
    return (gdds_u8)(((gdds_u32)a + (gdds_u32)b + (gdds_u32)c + (gdds_u32)d + 2u) / 4u);
}

static void gdds__downsample_box_normal_map_rgba8(const gdds_u8* src,
                                                  gdds_u32 src_w,
                                                  gdds_u32 src_h,
                                                  gdds_u8* dst,
                                                  gdds_u32 dst_w,
                                                  gdds_u32 dst_h) {
    gdds_u32 y;
    gdds_u32 x;
    for (y = 0u; y < dst_h; ++y) {
        for (x = 0u; x < dst_w; ++x) {
            gdds_u32 sx0 = x * 2u;
            gdds_u32 sy0 = y * 2u;
            gdds_u32 sx1 = (sx0 + 1u < src_w) ? (sx0 + 1u) : (src_w - 1u);
            gdds_u32 sy1 = (sy0 + 1u < src_h) ? (sy0 + 1u) : (src_h - 1u);
            const gdds_u8* p0 = src + (((gdds_size)sy0 * src_w + sx0) * 4u);
            const gdds_u8* p1 = src + (((gdds_size)sy0 * src_w + sx1) * 4u);
            const gdds_u8* p2 = src + (((gdds_size)sy1 * src_w + sx0) * 4u);
            const gdds_u8* p3 = src + (((gdds_size)sy1 * src_w + sx1) * 4u);
            gdds_u8* out = dst + (((gdds_size)y * dst_w + x) * 4u);
            gdds_s32 nx0, ny0, nz0;
            gdds_s32 nx1, ny1, nz1;
            gdds_s32 nx2, ny2, nz2;
            gdds_s32 nx3, ny3, nz3;
            gdds_s32 xsum, ysum, zsum;

            gdds__load_normal_from_rgb(p0, &nx0, &ny0, &nz0);
            gdds__load_normal_from_rgb(p1, &nx1, &ny1, &nz1);
            gdds__load_normal_from_rgb(p2, &nx2, &ny2, &nz2);
            gdds__load_normal_from_rgb(p3, &nx3, &ny3, &nz3);

            xsum = nx0 + nx1 + nx2 + nx3;
            ysum = ny0 + ny1 + ny2 + ny3;
            zsum = nz0 + nz1 + nz2 + nz3;
            gdds__normal_normalize_xyz(&xsum, &ysum, &zsum);
            gdds__store_normal_to_rgb(out, xsum, ysum, zsum);
            out[3] = gdds__avg4_u8(p0[3], p1[3], p2[3], p3[3]);
        }
    }
}

gdds_result gdds_normal_map_reconstruct_z_rgba8(gdds_u8* rgba8,
                                               gdds_u32 width,
                                               gdds_u32 height) {
    gdds_size pixel_count;
    gdds_size i;

    if (!rgba8 || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds__mul_size((gdds_size)width, (gdds_size)height, &pixel_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    for (i = 0u; i < pixel_count; ++i) {
        gdds_u8* px = rgba8 + i * 4u;
        gdds_s32 nx = (gdds_s32)px[0] * 2 - 255;
        gdds_s32 ny = (gdds_s32)px[1] * 2 - 255;
        gdds_u32 sum = (gdds_u32)(nx * nx + ny * ny);
        if (sum <= 65025u) {
            gdds_u32 zroot = gdds__isqrt_u32(65025u - sum);
            px[2] = (gdds_u8)((256u + zroot) / 2u);
        } else {
            px[0] = gdds__normal_pack_ratio(nx, sum);
            px[1] = gdds__normal_pack_ratio(ny, sum);
            px[2] = 128u;
        }
    }
    return GDDS_RESULT_OK;
}

gdds_result gdds_normal_map_normalize_rgba8(gdds_u8* rgba8,
                                             gdds_u32 width,
                                             gdds_u32 height) {
    gdds_size pixel_count;
    gdds_size i;

    if (!rgba8 || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds__mul_size((gdds_size)width, (gdds_size)height, &pixel_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    for (i = 0u; i < pixel_count; ++i) {
        gdds_u8* px = rgba8 + i * 4u;
        gdds_s32 nx = (gdds_s32)px[0] * 2 - 255;
        gdds_s32 ny = (gdds_s32)px[1] * 2 - 255;
        gdds_s32 nz = (gdds_s32)px[2] * 2 - 255;
        gdds_u32 sum = (gdds_u32)(nx * nx + ny * ny + nz * nz);
        px[0] = gdds__normal_pack_ratio(nx, sum);
        px[1] = gdds__normal_pack_ratio(ny, sum);
        px[2] = gdds__normal_pack_ratio(nz, sum);
    }
    return GDDS_RESULT_OK;
}

gdds_result gdds_normal_map_invert_y_rgba8(gdds_u8* rgba8,
                                            gdds_u32 width,
                                            gdds_u32 height) {
    gdds_size pixel_count;
    gdds_size i;

    if (!rgba8 || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds__mul_size((gdds_size)width, (gdds_size)height, &pixel_count)) {
        return GDDS_RESULT_UNSUPPORTED;
    }

    for (i = 0u; i < pixel_count; ++i) {
        gdds_u8* px = rgba8 + i * 4u;
        px[1] = (gdds_u8)(255u - px[1]);
    }
    return GDDS_RESULT_OK;
}

gdds_result gdds_normal_map_convert_convention_rgba8(gdds_u8* rgba8,
                                                      gdds_u32 width,
                                                      gdds_u32 height,
                                                      gdds_normal_map_layout layout,
                                                      gdds_normal_map_convention source_convention,
                                                      gdds_normal_map_convention target_convention) {
    if (!rgba8 || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!(layout == GDDS_NORMAL_MAP_LAYOUT_XY_RG || layout == GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds__normal_convention_valid(source_convention) ||
        !gdds__normal_convention_valid(target_convention)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (!gdds_normal_map_convention_needs_y_flip(source_convention, target_convention)) {
        return GDDS_RESULT_OK;
    }
    return gdds_normal_map_invert_y_rgba8(rgba8, width, height);
}

gdds_result gdds_decode_bc5_normal_map_memory(const void* dds_data,
                                               gdds_size dds_size,
                                               const gdds_normal_map_decode_options* options,
                                               gdds_image* out_image) {
    return gdds_decode_bc5_normal_map_mip_memory(dds_data,
                                                 dds_size,
                                                 0u,
                                                 options,
                                                 out_image);
}

gdds_result gdds_decode_bc5_normal_map_mip_memory(const void* dds_data,
                                                   gdds_size dds_size,
                                                   gdds_u32 level_index,
                                                   const gdds_normal_map_decode_options* options,
                                                   gdds_image* out_image) {
    gdds_normal_map_decode_options opt;
    gdds_result rc;

    if (!dds_data || !out_image) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    rc = gdds_decode_mip_memory(dds_data, dds_size, level_index, out_image);
    if (rc != GDDS_RESULT_OK) {
        return rc;
    }
    if (!gdds__normal_map_source_format_is_bc5(out_image->source_format)) {
        gdds_image_release(out_image);
        return GDDS_RESULT_UNSUPPORTED;
    }

    opt = gdds__normal_decode_options_or_default(options);
    if (!gdds__normal_decode_options_valid(&opt)) {
        gdds_image_release(out_image);
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    rc = gdds_normal_map_convert_convention_rgba8(out_image->pixels,
                                                  out_image->width,
                                                  out_image->height,
                                                  GDDS_NORMAL_MAP_LAYOUT_XY_RG,
                                                  opt.stored_convention,
                                                  opt.output_convention);
    if (rc != GDDS_RESULT_OK) {
        gdds_image_release(out_image);
        return rc;
    }

    if (opt.invert_y) {
        rc = gdds_normal_map_invert_y_rgba8(out_image->pixels, out_image->width, out_image->height);
        if (rc != GDDS_RESULT_OK) {
            gdds_image_release(out_image);
            return rc;
        }
    }
    if (opt.reconstruct_z || opt.normalize) {
        rc = gdds_normal_map_reconstruct_z_rgba8(out_image->pixels, out_image->width, out_image->height);
        if (rc != GDDS_RESULT_OK) {
            gdds_image_release(out_image);
            return rc;
        }
    }
    if (opt.normalize) {
        rc = gdds_normal_map_normalize_rgba8(out_image->pixels, out_image->width, out_image->height);
        if (rc != GDDS_RESULT_OK) {
            gdds_image_release(out_image);
            return rc;
        }
    }

    return GDDS_RESULT_OK;
}

gdds_result gdds_generate_normal_map_mipchain_rgba8(const gdds_u8* rgba8,
                                                     gdds_u32 width,
                                                     gdds_u32 height,
                                                     const gdds_normal_map_encode_options* normal_options,
                                                     const gdds_mipmap_options* mip_options,
                                                     gdds_generated_mipchain* out_mipchain) {
    gdds_normal_map_encode_options normal_opt;
    gdds_mipmap_options mip_opt;
    gdds_generated_mipchain chain;
    gdds_u32 max_level_count;
    gdds_u32 level_count;
    gdds_u32 cur_w;
    gdds_u32 cur_h;
    gdds_u32 level;
    gdds_size levels_bytes;
    gdds_size mip_offset = 0u;

    if (!rgba8 || !out_mipchain || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    memset(out_mipchain, 0, sizeof(*out_mipchain));
    memset(&chain, 0, sizeof(chain));

    normal_opt = gdds__normal_encode_options_or_default(normal_options);
    if (!gdds__normal_encode_options_valid(&normal_opt)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    mip_opt = mip_options ? *mip_options : gdds_mipmap_options_default();
    if (mip_opt.filter != GDDS_MIP_FILTER_BOX) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (mip_opt.color_space == GDDS_MIP_COLOR_SPACE_SRGB) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    max_level_count = gdds__max_mip_count_2d(width, height);
    level_count = (mip_opt.mip_count == 0u) ? max_level_count : mip_opt.mip_count;
    if (level_count == 0u || level_count > max_level_count) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    if (level_count > GDDS_MAX_MIP_LEVELS) {
        return GDDS_RESULT_ALLOC;
    }
    chain.levels = gdds__static_mip_levels;
    memset(chain.levels, 0, sizeof(gdds__static_mip_levels));
    chain.level_count = level_count;

    cur_w = width;
    cur_h = height;
    for (level = 0u; level < level_count; ++level) {
        gdds_generated_mip_level* dst_level = &chain.levels[level];
        gdds_size level_bytes;
        if (!gdds__mul_size((gdds_size)cur_w, (gdds_size)cur_h, &level_bytes) ||
            !gdds__mul_size(level_bytes, 4u, &level_bytes)) {
            gdds_generated_mipchain_release(&chain);
            return GDDS_RESULT_UNSUPPORTED;
        }
        if (mip_offset > GDDS_STATIC_MIP_CAPACITY ||
            level_bytes > GDDS_STATIC_MIP_CAPACITY - mip_offset) {
            gdds_generated_mipchain_release(&chain);
            return GDDS_RESULT_ALLOC;
        }
        dst_level->pixels = gdds__static_mip_rgba + mip_offset;
        dst_level->size = level_bytes;
        mip_offset += level_bytes;
        dst_level->width = cur_w;
        dst_level->height = cur_h;

        if (level == 0u) {
            gdds_size i;
            gdds_size pixel_count = dst_level->size / 4u;
            for (i = 0u; i < pixel_count; ++i) {
                gdds__prepare_normal_pixel(rgba8 + i * 4u,
                                           &normal_opt,
                                           dst_level->pixels + i * 4u);
            }
        } else {
            const gdds_generated_mip_level* prev = &chain.levels[level - 1u];
            gdds__downsample_box_normal_map_rgba8(prev->pixels,
                                                  prev->width,
                                                  prev->height,
                                                  dst_level->pixels,
                                                  cur_w,
                                                  cur_h);
        }

        cur_w = gdds__mip_dim(cur_w);
        cur_h = gdds__mip_dim(cur_h);
    }

    *out_mipchain = chain;
    return GDDS_RESULT_OK;
}

gdds_result gdds_encode_bc5_normal_map_rgba8(const gdds_u8* rgba8,
                                              gdds_u32 width,
                                              gdds_u32 height,
                                              const gdds_normal_map_encode_options* normal_options,
                                              gdds_buffer* out_buffer) {
    gdds_normal_map_encode_options opt;
    gdds_encode_options encode_options;
    gdds_u8* prepared = NULL;
    gdds_size prepared_size = 0u;
    gdds_result rc;

    if (!rgba8 || !out_buffer || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    opt = gdds__normal_encode_options_or_default(normal_options);
    if (!gdds__normal_encode_options_valid(&opt)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    encode_options = gdds_encode_options_default(opt.output_format);

    rc = gdds__prepare_normal_map_rgba8(rgba8,
                                        width,
                                        height,
                                        &opt,
                                        &prepared,
                                        &prepared_size);
    if (rc != GDDS_RESULT_OK) {
        return rc;
    }

    rc = gdds_encode_memory_rgba8(prepared,
                                  width,
                                  height,
                                  &encode_options,
                                  out_buffer);
    (void)prepared_size;
    return rc;
}

gdds_result gdds_encode_bc5_normal_map_rgba8_auto_mips(const gdds_u8* rgba8,
                                                        gdds_u32 width,
                                                        gdds_u32 height,
                                                        const gdds_normal_map_encode_options* normal_options,
                                                        const gdds_mipmap_options* mip_options,
                                                        gdds_buffer* out_buffer) {
    gdds_normal_map_encode_options opt;
    gdds_generated_mipchain chain;
    gdds_mip_level_rgba8* levels = NULL;
    gdds_encode_options encode_options;
    gdds_size levels_bytes;
    gdds_u32 i;
    gdds_result rc;

    if (!rgba8 || !out_buffer || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    opt = gdds__normal_encode_options_or_default(normal_options);
    if (!gdds__normal_encode_options_valid(&opt)) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    encode_options = gdds_encode_options_default(opt.output_format);

    memset(&chain, 0, sizeof(chain));

    rc = gdds_generate_normal_map_mipchain_rgba8(rgba8,
                                                 width,
                                                 height,
                                                 &opt,
                                                 mip_options,
                                                 &chain);
    if (rc != GDDS_RESULT_OK) {
        return rc;
    }

    if (!gdds__mul_size((gdds_size)chain.level_count, sizeof(*levels), &levels_bytes)) {
        gdds_generated_mipchain_release(&chain);
        return GDDS_RESULT_UNSUPPORTED;
    }

    if (chain.level_count > GDDS_MAX_MIP_LEVELS) {
        gdds_generated_mipchain_release(&chain);
        return GDDS_RESULT_ALLOC;
    }
    levels = gdds__static_encode_levels;

    for (i = 0u; i < chain.level_count; ++i) {
        levels[i].pixels = chain.levels[i].pixels;
        levels[i].size = chain.levels[i].size;
    }

    rc = gdds_encode_mipchain_memory_rgba8(levels,
                                           chain.level_count,
                                           width,
                                           height,
                                           &encode_options,
                                           out_buffer);

    gdds_generated_mipchain_release(&chain);
    return rc;
}
