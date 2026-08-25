#include "gdds_internal.h"

static gdds_mipmap_options gdds__mipmap_options_or_default(const gdds_mipmap_options* options) {
    if (options) {
        return *options;
    }
    return gdds_mipmap_options_default();
}

static gdds_u8 gdds__div_round_u32(gdds_u32 value, gdds_u32 divisor) {
    if (divisor == 0u) return 0u;
    return (gdds_u8)((value + (divisor / 2u)) / divisor);
}

static gdds_u16 gdds__div_round_u32_to_u16(gdds_u32 value, gdds_u32 divisor) {
    if (divisor == 0u) return 0u;
    return (gdds_u16)((value + (divisor / 2u)) / divisor);
}

static gdds_mip_color_space gdds__resolve_generate_color_space(gdds_mip_color_space color_space) {
    if (color_space == GDDS_MIP_COLOR_SPACE_AUTO) {
        return GDDS_MIP_COLOR_SPACE_LINEAR;
    }
    return color_space;
}

static void gdds__downsample_box_rgba8(const gdds_u8* src,
                                       gdds_u32 src_w,
                                       gdds_u32 src_h,
                                       gdds_u8* dst,
                                       gdds_u32 dst_w,
                                       gdds_u32 dst_h,
                                       gdds_mip_alpha_filter alpha_filter,
                                       gdds_mip_color_space color_space) {
    gdds_u32 y, x;
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

            if (color_space == GDDS_MIP_COLOR_SPACE_SRGB) {
                gdds_u32 a_sum = (gdds_u32)p0[3] + (gdds_u32)p1[3] + (gdds_u32)p2[3] + (gdds_u32)p3[3];
                gdds_u16 l0r = gdds__srgb8_to_linear12(p0[0]);
                gdds_u16 l1r = gdds__srgb8_to_linear12(p1[0]);
                gdds_u16 l2r = gdds__srgb8_to_linear12(p2[0]);
                gdds_u16 l3r = gdds__srgb8_to_linear12(p3[0]);
                gdds_u16 l0g = gdds__srgb8_to_linear12(p0[1]);
                gdds_u16 l1g = gdds__srgb8_to_linear12(p1[1]);
                gdds_u16 l2g = gdds__srgb8_to_linear12(p2[1]);
                gdds_u16 l3g = gdds__srgb8_to_linear12(p3[1]);
                gdds_u16 l0b = gdds__srgb8_to_linear12(p0[2]);
                gdds_u16 l1b = gdds__srgb8_to_linear12(p1[2]);
                gdds_u16 l2b = gdds__srgb8_to_linear12(p2[2]);
                gdds_u16 l3b = gdds__srgb8_to_linear12(p3[2]);

                out[3] = gdds__div_round_u32(a_sum, 4u);
                if (alpha_filter == GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED) {
                    gdds_u32 r_sum = (gdds_u32)l0r * (gdds_u32)p0[3] +
                                     (gdds_u32)l1r * (gdds_u32)p1[3] +
                                     (gdds_u32)l2r * (gdds_u32)p2[3] +
                                     (gdds_u32)l3r * (gdds_u32)p3[3];
                    gdds_u32 g_sum = (gdds_u32)l0g * (gdds_u32)p0[3] +
                                     (gdds_u32)l1g * (gdds_u32)p1[3] +
                                     (gdds_u32)l2g * (gdds_u32)p2[3] +
                                     (gdds_u32)l3g * (gdds_u32)p3[3];
                    gdds_u32 b_sum = (gdds_u32)l0b * (gdds_u32)p0[3] +
                                     (gdds_u32)l1b * (gdds_u32)p1[3] +
                                     (gdds_u32)l2b * (gdds_u32)p2[3] +
                                     (gdds_u32)l3b * (gdds_u32)p3[3];
                    if (a_sum == 0u) {
                        out[0] = 0u;
                        out[1] = 0u;
                        out[2] = 0u;
                    } else {
                        out[0] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(r_sum, a_sum));
                        out[1] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(g_sum, a_sum));
                        out[2] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(b_sum, a_sum));
                    }
                } else {
                    gdds_u32 r_sum = (gdds_u32)l0r + (gdds_u32)l1r + (gdds_u32)l2r + (gdds_u32)l3r;
                    gdds_u32 g_sum = (gdds_u32)l0g + (gdds_u32)l1g + (gdds_u32)l2g + (gdds_u32)l3g;
                    gdds_u32 b_sum = (gdds_u32)l0b + (gdds_u32)l1b + (gdds_u32)l2b + (gdds_u32)l3b;
                    out[0] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(r_sum, 4u));
                    out[1] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(g_sum, 4u));
                    out[2] = gdds__linear12_to_srgb8(gdds__div_round_u32_to_u16(b_sum, 4u));
                }
            } else if (alpha_filter == GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED) {
                gdds_u32 a_sum = (gdds_u32)p0[3] + (gdds_u32)p1[3] + (gdds_u32)p2[3] + (gdds_u32)p3[3];
                gdds_u32 r_sum = (gdds_u32)p0[0] * (gdds_u32)p0[3] +
                                 (gdds_u32)p1[0] * (gdds_u32)p1[3] +
                                 (gdds_u32)p2[0] * (gdds_u32)p2[3] +
                                 (gdds_u32)p3[0] * (gdds_u32)p3[3];
                gdds_u32 g_sum = (gdds_u32)p0[1] * (gdds_u32)p0[3] +
                                 (gdds_u32)p1[1] * (gdds_u32)p1[3] +
                                 (gdds_u32)p2[1] * (gdds_u32)p2[3] +
                                 (gdds_u32)p3[1] * (gdds_u32)p3[3];
                gdds_u32 b_sum = (gdds_u32)p0[2] * (gdds_u32)p0[3] +
                                 (gdds_u32)p1[2] * (gdds_u32)p1[3] +
                                 (gdds_u32)p2[2] * (gdds_u32)p2[3] +
                                 (gdds_u32)p3[2] * (gdds_u32)p3[3];
                out[3] = gdds__div_round_u32(a_sum, 4u);
                if (a_sum == 0u) {
                    out[0] = 0u;
                    out[1] = 0u;
                    out[2] = 0u;
                } else {
                    out[0] = gdds__div_round_u32(r_sum, a_sum);
                    out[1] = gdds__div_round_u32(g_sum, a_sum);
                    out[2] = gdds__div_round_u32(b_sum, a_sum);
                }
            } else {
                out[0] = gdds__div_round_u32((gdds_u32)p0[0] + (gdds_u32)p1[0] + (gdds_u32)p2[0] + (gdds_u32)p3[0], 4u);
                out[1] = gdds__div_round_u32((gdds_u32)p0[1] + (gdds_u32)p1[1] + (gdds_u32)p2[1] + (gdds_u32)p3[1], 4u);
                out[2] = gdds__div_round_u32((gdds_u32)p0[2] + (gdds_u32)p1[2] + (gdds_u32)p2[2] + (gdds_u32)p3[2], 4u);
                out[3] = gdds__div_round_u32((gdds_u32)p0[3] + (gdds_u32)p1[3] + (gdds_u32)p2[3] + (gdds_u32)p3[3], 4u);
            }
        }
    }
}

gdds_result gdds_generate_mipchain_rgba8(const gdds_u8* rgba8,
                                         gdds_u32 width,
                                         gdds_u32 height,
                                         const gdds_mipmap_options* options,
                                         gdds_generated_mipchain* out_mipchain) {
    gdds_mipmap_options opt;
    gdds_generated_mipchain chain;
    gdds_u32 level_count;
    gdds_u32 max_level_count;
    gdds_u32 level;
    gdds_u32 cur_w;
    gdds_u32 cur_h;
    gdds_size level_bytes;
    gdds_size mip_offset = 0u;

    if (!rgba8 || !out_mipchain || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    memset(out_mipchain, 0, sizeof(*out_mipchain));
    memset(&chain, 0, sizeof(chain));

    opt = gdds__mipmap_options_or_default(options);
    if (opt.filter != GDDS_MIP_FILTER_BOX) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    if (opt.alpha_filter != GDDS_MIP_ALPHA_FILTER_STRAIGHT &&
        opt.alpha_filter != GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }
    opt.color_space = gdds__resolve_generate_color_space(opt.color_space);
    if (opt.color_space != GDDS_MIP_COLOR_SPACE_LINEAR &&
        opt.color_space != GDDS_MIP_COLOR_SPACE_SRGB) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    max_level_count = gdds__max_mip_count_2d(width, height);
    level_count = (opt.mip_count == 0u) ? max_level_count : opt.mip_count;
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
        mip_offset += level_bytes;
        dst_level->width = cur_w;
        dst_level->height = cur_h;
        dst_level->size = level_bytes;

        if (level == 0u) {
            memcpy(dst_level->pixels, rgba8, level_bytes);
        } else {
            const gdds_generated_mip_level* prev_level = &chain.levels[level - 1u];
            gdds__downsample_box_rgba8(prev_level->pixels,
                                       prev_level->width,
                                       prev_level->height,
                                       dst_level->pixels,
                                       cur_w,
                                       cur_h,
                                       opt.alpha_filter,
                                       opt.color_space);
        }

        cur_w = gdds__mip_dim(cur_w);
        cur_h = gdds__mip_dim(cur_h);
    }

    *out_mipchain = chain;
    return GDDS_RESULT_OK;
}

gdds_result gdds_encode_memory_rgba8_auto_mips(const gdds_u8* rgba8,
                                               gdds_u32 width,
                                               gdds_u32 height,
                                               const gdds_encode_options* encode_options,
                                               const gdds_mipmap_options* mip_options,
                                               gdds_buffer* out_buffer) {
    gdds_generated_mipchain chain;
    gdds_mip_level_rgba8* levels = NULL;
    gdds_mipmap_options effective_mips;
    gdds_size levels_bytes;
    gdds_u32 i;
    gdds_result rc;

    if (!rgba8 || !encode_options || !out_buffer || width == 0u || height == 0u) {
        return GDDS_RESULT_INVALID_ARGUMENT;
    }

    memset(&chain, 0, sizeof(chain));

    effective_mips = gdds__mipmap_options_or_default(mip_options);
    if (effective_mips.color_space == GDDS_MIP_COLOR_SPACE_AUTO) {
        effective_mips.color_space = gdds__format_is_srgb(encode_options->format)
            ? GDDS_MIP_COLOR_SPACE_SRGB
            : GDDS_MIP_COLOR_SPACE_LINEAR;
    }

    rc = gdds_generate_mipchain_rgba8(rgba8, width, height, &effective_mips, &chain);
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
                                           encode_options,
                                           out_buffer);

    gdds_generated_mipchain_release(&chain);
    return rc;
}
