#include "gdds_internal.h"

static void gdds__build_bc1_palette(gdds_u16 c0, gdds_u16 c1, gdds_u8 palette[4][4]) {
    gdds_u8 r0, g0, b0, r1, g1, b1;
    gdds__rgb565_to_rgb888(c0, &r0, &g0, &b0);
    gdds__rgb565_to_rgb888(c1, &r1, &g1, &b1);
    palette[0][0] = r0; palette[0][1] = g0; palette[0][2] = b0; palette[0][3] = 255u;
    palette[1][0] = r1; palette[1][1] = g1; palette[1][2] = b1; palette[1][3] = 255u;
    if (c0 > c1) {
        palette[2][0] = (gdds_u8)((2u * r0 + r1) / 3u);
        palette[2][1] = (gdds_u8)((2u * g0 + g1) / 3u);
        palette[2][2] = (gdds_u8)((2u * b0 + b1) / 3u); palette[2][3] = 255u;
        palette[3][0] = (gdds_u8)((r0 + 2u * r1) / 3u);
        palette[3][1] = (gdds_u8)((g0 + 2u * g1) / 3u);
        palette[3][2] = (gdds_u8)((b0 + 2u * b1) / 3u); palette[3][3] = 255u;
    } else {
        palette[2][0] = (gdds_u8)((r0 + r1) / 2u);
        palette[2][1] = (gdds_u8)((g0 + g1) / 2u);
        palette[2][2] = (gdds_u8)((b0 + b1) / 2u); palette[2][3] = 255u;
        palette[3][0] = 0u; palette[3][1] = 0u; palette[3][2] = 0u; palette[3][3] = 0u;
    }
}

static void gdds__build_bc_alpha_palette(gdds_u8 a0, gdds_u8 a1, gdds_u8 out[8]) {
    out[0] = a0; out[1] = a1;
    if (a0 > a1) {
        out[2] = (gdds_u8)((6u*a0 + a1)/7u); out[3] = (gdds_u8)((5u*a0 + 2u*a1)/7u);
        out[4] = (gdds_u8)((4u*a0 + 3u*a1)/7u); out[5] = (gdds_u8)((3u*a0 + 4u*a1)/7u);
        out[6] = (gdds_u8)((2u*a0 + 5u*a1)/7u); out[7] = (gdds_u8)((a0 + 6u*a1)/7u);
    } else {
        out[2] = (gdds_u8)((4u*a0 + a1)/5u); out[3] = (gdds_u8)((3u*a0 + 2u*a1)/5u);
        out[4] = (gdds_u8)((2u*a0 + 3u*a1)/5u); out[5] = (gdds_u8)((a0 + 4u*a1)/5u);
        out[6] = 0u; out[7] = 255u;
    }
}

static gdds_u32 gdds__read_3bit_index(const gdds_u8 bytes[6], gdds_u32 index) {
    gdds_u32 bit = index * 3u;
    gdds_u32 byte_index = bit >> 3u;
    gdds_u32 shift = bit & 7u;
    gdds_u32 word = bytes[byte_index];
    if (byte_index + 1u < 6u) word |= ((gdds_u32)bytes[byte_index + 1u]) << 8u;
    return (word >> shift) & 7u;
}

static void gdds__write_3bit_index(gdds_u8 bytes[6], gdds_u32 index, gdds_u32 value) {
    gdds_u32 bit = index * 3u;
    gdds_u32 byte_index = bit >> 3u;
    gdds_u32 shift = bit & 7u;
    gdds_u32 word = (value & 7u) << shift;
    bytes[byte_index] = (gdds_u8)(bytes[byte_index] | (gdds_u8)(word & 0xFFu));
    if (shift > 5u && byte_index + 1u < 6u) {
        bytes[byte_index + 1u] = (gdds_u8)(bytes[byte_index + 1u] | (gdds_u8)((word >> 8u) & 0xFFu));
    }
}

static void gdds__decode_bc_alpha_values(const gdds_u8* block, gdds_u8 out_values[16]) {
    gdds_u8 palette[8];
    gdds_u32 i;
    gdds__build_bc_alpha_palette(block[0], block[1], palette);
    for (i = 0u; i < 16u; ++i) out_values[i] = palette[gdds__read_3bit_index(block + 2, i)];
}

static void gdds__encode_bc_alpha_values(const gdds_u8 values[16], gdds_u8 out_block[8]) {
    gdds_u32 i;
    gdds_u8 vmin = 255u, vmax = 0u, palette[8];
    for (i = 0u; i < 16u; ++i) { if (values[i] < vmin) vmin = values[i]; if (values[i] > vmax) vmax = values[i]; }
    out_block[0] = vmax; out_block[1] = vmin;
    for (i = 2u; i < 8u; ++i) out_block[i] = 0u;
    gdds__build_bc_alpha_palette(vmax, vmin, palette);
    for (i = 0u; i < 16u; ++i) {
        gdds_u32 j, best = 0u; int best_d = 1 << 30;
        for (j = 0u; j < 8u; ++j) {
            int d = gdds__abs_i32((int)values[i] - (int)palette[j]);
            if (d < best_d) { best_d = d; best = j; }
        }
        gdds__write_3bit_index(out_block + 2, i, best);
    }
}

static int gdds__snorm_byte_to_i32(gdds_u8 value) {
    int v = (int)((gdds_s8)value);
    if (v < -127) v = -127; if (v > 127) v = 127; return v;
}

static int gdds__round_div_signed(int n, int d) {
    if (n >= 0) return (n + d / 2) / d;
    return -(((-n) + d / 2) / d);
}

static gdds_u8 gdds__packed_to_snorm_byte(gdds_u8 value) {
    int p = (int)value * 2 - 255;
    int v = gdds__round_div_signed(p * 127, 255);
    if (v < -127) v = -127; if (v > 127) v = 127;
    return (gdds_u8)(gdds_s8)v;
}

static gdds_u8 gdds__rational_normal_to_packed(int num, int den) {
    int n;
    int d;
    if (num < -den) num = -den; if (num > den) num = den;
    n = (num + den) * 255;
    d = den * 2;
    return (gdds_u8)((n + d / 2) / d);
}

static void gdds__build_bc_snorm_palette(gdds_u8 a0, gdds_u8 a1, int nums[8], int dens[8]) {
    int i0 = gdds__snorm_byte_to_i32(a0);
    int i1 = gdds__snorm_byte_to_i32(a1);
    nums[0]=i0; dens[0]=127; nums[1]=i1; dens[1]=127;
    if (i0 > i1) {
        nums[2]=6*i0+i1; dens[2]=889; nums[3]=5*i0+2*i1; dens[3]=889;
        nums[4]=4*i0+3*i1; dens[4]=889; nums[5]=3*i0+4*i1; dens[5]=889;
        nums[6]=2*i0+5*i1; dens[6]=889; nums[7]=i0+6*i1; dens[7]=889;
    } else {
        nums[2]=4*i0+i1; dens[2]=635; nums[3]=3*i0+2*i1; dens[3]=635;
        nums[4]=2*i0+3*i1; dens[4]=635; nums[5]=i0+4*i1; dens[5]=635;
        nums[6]=-1; dens[6]=1; nums[7]=1; dens[7]=1;
    }
}

static void gdds__decode_bc_snorm_values(const gdds_u8* block, gdds_u8 out_values[16]) {
    int nums[8], dens[8]; gdds_u32 i;
    gdds__build_bc_snorm_palette(block[0], block[1], nums, dens);
    for (i=0u; i<16u; ++i) { gdds_u32 k=gdds__read_3bit_index(block+2,i); out_values[i]=gdds__rational_normal_to_packed(nums[k],dens[k]); }
}

static void gdds__encode_bc_snorm_values(const gdds_u8 values[16], gdds_u8 out_block[8]) {
    gdds_u32 i; gdds_u8 vmin=255u, vmax=0u; int nums[8], dens[8];
    for (i=0u;i<16u;++i) { if(values[i]<vmin)vmin=values[i]; if(values[i]>vmax)vmax=values[i]; }
    out_block[0]=gdds__packed_to_snorm_byte(vmax); out_block[1]=gdds__packed_to_snorm_byte(vmin);
    for(i=2u;i<8u;++i) out_block[i]=0u;
    gdds__build_bc_snorm_palette(out_block[0],out_block[1],nums,dens);
    for(i=0u;i<16u;++i) {
        int p=(int)values[i]*2-255; gdds_u32 j,best=0u; gdds_u32 best_num=0xFFFFFFFFu; int best_den=1;
        for(j=0u;j<8u;++j) {
            int d=p*dens[j]-nums[j]*255; gdds_u32 ad=(gdds_u32)(d<0?-d:d);
            if(best_num==0xFFFFFFFFu || ad*(gdds_u32)best_den < best_num*(gdds_u32)dens[j]) { best_num=ad; best_den=dens[j]; best=j; }
        }
        gdds__write_3bit_index(out_block+2,i,best);
    }
}

void gdds__decode_bc1_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u16 c0 = (gdds_u16)(block[0] | ((gdds_u16)block[1] << 8));
    gdds_u16 c1 = (gdds_u16)(block[2] | ((gdds_u16)block[3] << 8));
    gdds_u32 bits = gdds__read_u32le(block + 4);
    gdds_u8 palette[4][4];
    gdds_u32 i;
    gdds__build_bc1_palette(c0, c1, palette);
    for (i = 0; i < 16u; ++i) {
        gdds_u32 idx = (bits >> (2u * i)) & 3u;
        out_rgba[i][0] = palette[idx][0];
        out_rgba[i][1] = palette[idx][1];
        out_rgba[i][2] = palette[idx][2];
        out_rgba[i][3] = palette[idx][3];
    }
}

void gdds__decode_bc2_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u32 i;
    gdds__decode_bc1_block(block + 8, out_rgba);
    for (i = 0u; i < 16u; ++i) {
        gdds_u8 packed = block[i >> 1u];
        gdds_u8 a4 = (gdds_u8)((i & 1u) ? (packed >> 4u) : (packed & 15u));
        out_rgba[i][3] = (gdds_u8)(a4 * 17u);
    }
}

void gdds__decode_bc3_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u8 values[16];
    gdds_u32 i;
    gdds__decode_bc_alpha_values(block, values);
    gdds__decode_bc1_block(block + 8, out_rgba);
    for (i = 0; i < 16u; ++i) {
        out_rgba[i][3] = values[i];
    }
}

void gdds__decode_bc4_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u8 values[16];
    gdds_u32 i;
    gdds__decode_bc_alpha_values(block, values);
    for (i = 0; i < 16u; ++i) {
        out_rgba[i][0] = values[i];
        out_rgba[i][1] = values[i];
        out_rgba[i][2] = values[i];
        out_rgba[i][3] = 255u;
    }
}

void gdds__decode_bc5_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u8 red_values[16];
    gdds_u8 green_values[16];
    gdds_u32 i;
    gdds__decode_bc_alpha_values(block, red_values);
    gdds__decode_bc_alpha_values(block + 8, green_values);
    for (i = 0; i < 16u; ++i) {
        out_rgba[i][0] = red_values[i];
        out_rgba[i][1] = green_values[i];
        out_rgba[i][2] = 0u;
        out_rgba[i][3] = 255u;
    }
}

void gdds__decode_bc4s_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u8 values[16];
    gdds_u32 i;
    gdds__decode_bc_snorm_values(block, values);
    for (i = 0; i < 16u; ++i) {
        out_rgba[i][0] = values[i];
        out_rgba[i][1] = values[i];
        out_rgba[i][2] = values[i];
        out_rgba[i][3] = 255u;
    }
}

void gdds__decode_bc5s_block(const gdds_u8* block, gdds_u8 out_rgba[16][4]) {
    gdds_u8 red_values[16];
    gdds_u8 green_values[16];
    gdds_u8 zero = 128u;
    gdds_u32 i;
    gdds__decode_bc_snorm_values(block, red_values);
    gdds__decode_bc_snorm_values(block + 8, green_values);
    for (i = 0; i < 16u; ++i) {
        out_rgba[i][0] = red_values[i];
        out_rgba[i][1] = green_values[i];
        out_rgba[i][2] = zero;
        out_rgba[i][3] = 255u;
    }
}

void gdds__block_to_image(const gdds_u8* block_rgba,
                          gdds_u8* dst,
                          gdds_u32 width,
                          gdds_u32 height,
                          gdds_u32 bx,
                          gdds_u32 by) {
    gdds_u32 px, py;
    for (py = 0; py < 4u; ++py) {
        gdds_u32 y = by * 4u + py;
        if (y >= height) continue;
        for (px = 0; px < 4u; ++px) {
            gdds_u32 x = bx * 4u + px;
            gdds_u8* out_px;
            const gdds_u8* src_px;
            if (x >= width) continue;
            out_px = dst + (((gdds_size)y * width + x) * 4u);
            src_px = block_rgba + ((py * 4u + px) * 4u);
            out_px[0] = src_px[0];
            out_px[1] = src_px[1];
            out_px[2] = src_px[2];
            out_px[3] = src_px[3];
        }
    }
}

void gdds__fetch_block_rgba(const gdds_u8* rgba,
                            gdds_u32 width,
                            gdds_u32 height,
                            gdds_u32 bx,
                            gdds_u32 by,
                            gdds_u8 block[16][4]) {
    gdds_u32 px, py;
    gdds_u32 sx0 = bx * 4u;
    gdds_u32 sy0 = by * 4u;
    for (py = 0; py < 4u; ++py) {
        gdds_u32 y = sy0 + py;
        if (y >= height) y = height - 1u;
        for (px = 0; px < 4u; ++px) {
            gdds_u32 x = sx0 + px;
            const gdds_u8* src;
            if (x >= width) x = width - 1u;
            src = rgba + (((gdds_size)y * width + x) * 4u);
            block[py * 4u + px][0] = src[0];
            block[py * 4u + px][1] = src[1];
            block[py * 4u + px][2] = src[2];
            block[py * 4u + px][3] = src[3];
        }
    }
}

static gdds_u32 gdds__color_distance_sq(const gdds_u8* a, const gdds_u8* b) {
    int dr = (int)a[0] - (int)b[0];
    int dg = (int)a[1] - (int)b[1];
    int db = (int)a[2] - (int)b[2];
    return (gdds_u32)(dr * dr + dg * dg + db * db);
}

static void gdds__choose_color_endpoints(const gdds_u8* block,
                                         int ignore_transparent,
                                         gdds_u16* out_cmin,
                                         gdds_u16* out_cmax,
                                         int* out_all_transparent) {
    gdds_u32 i;
    int found = 0;
    gdds_u8 min_r = 255, min_g = 255, min_b = 255;
    gdds_u8 max_r = 0, max_g = 0, max_b = 0;
    if (out_all_transparent) *out_all_transparent = 0;
    for (i = 0; i < 16u; ++i) {
        const gdds_u8* p = block + i * 4u;
        if (ignore_transparent && p[3] < 128u) continue;
        if (!found) {
            min_r = max_r = p[0];
            min_g = max_g = p[1];
            min_b = max_b = p[2];
            found = 1;
        } else {
            if (p[0] < min_r) min_r = p[0];
            if (p[1] < min_g) min_g = p[1];
            if (p[2] < min_b) min_b = p[2];
            if (p[0] > max_r) max_r = p[0];
            if (p[1] > max_g) max_g = p[1];
            if (p[2] > max_b) max_b = p[2];
        }
    }
    if (!found) {
        *out_cmin = 0;
        *out_cmax = 0;
        if (out_all_transparent) *out_all_transparent = 1;
        return;
    }
    *out_cmin = gdds__rgb888_to_rgb565(min_r, min_g, min_b);
    *out_cmax = gdds__rgb888_to_rgb565(max_r, max_g, max_b);
}

void gdds__encode_bc1_color_block(const gdds_u8* block, int allow_1bit_alpha, gdds_u8 out_block[8]) {
    int use_transparency = 0;
    int all_transparent = 0;
    gdds_u16 c0, c1;
    gdds_u8 palette[4][4];
    gdds_u32 idx_bits = 0;
    gdds_u32 i;

    if (allow_1bit_alpha) {
        for (i = 0; i < 16u; ++i) {
            if (block[i * 4u + 3u] < 128u) {
                use_transparency = 1;
                break;
            }
        }
    }

    gdds__choose_color_endpoints(block, use_transparency, &c1, &c0, &all_transparent);
    if (all_transparent && use_transparency) {
        c0 = 0;
        c1 = 0;
        gdds__build_bc1_palette(c0, c1, palette);
        idx_bits = 0xFFFFFFFFu;
        gdds__write_u16le(out_block + 0, c0);
        gdds__write_u16le(out_block + 2, c1);
        gdds__write_u32le(out_block + 4, idx_bits);
        return;
    }

    if (use_transparency) {
        if (c0 > c1) {
            gdds_u16 t = c0; c0 = c1; c1 = t;
        }
    } else {
        if (c0 < c1) {
            gdds_u16 t = c0; c0 = c1; c1 = t;
        }
    }

    gdds__build_bc1_palette(c0, c1, palette);

    for (i = 0; i < 16u; ++i) {
        gdds_u32 best = 0;
        gdds_u32 best_d = 0xFFFFFFFFu;
        gdds_u32 max_index = (use_transparency ? 2u : 3u);
        gdds_u32 j;
        if (use_transparency && block[i * 4u + 3u] < 128u) {
            best = 3u;
        } else {
            for (j = 0; j <= max_index; ++j) {
                gdds_u32 d = gdds__color_distance_sq(block + i * 4u, palette[j]);
                if (d < best_d) {
                    best_d = d;
                    best = j;
                }
            }
        }
        idx_bits |= (best & 3u) << (2u * i);
    }

    gdds__write_u16le(out_block + 0, c0);
    gdds__write_u16le(out_block + 2, c1);
    gdds__write_u32le(out_block + 4, idx_bits);
}

void gdds__encode_bc2_block(const gdds_u8* block, gdds_u8 out_block[16]) {
    gdds_u32 i;
    for (i = 0u; i < 8u; ++i) out_block[i] = 0u;
    for (i = 0u; i < 16u; ++i) {
        gdds_u8 a4 = (gdds_u8)((block[i * 4u + 3u] + 8u) / 17u);
        gdds_u32 bi = i >> 1u;
        if (a4 > 15u) a4 = 15u;
        if (i & 1u) out_block[bi] = (gdds_u8)(out_block[bi] | (gdds_u8)(a4 << 4u));
        else out_block[bi] = (gdds_u8)(out_block[bi] | a4);
    }
    gdds__encode_bc1_color_block(block, 0, out_block + 8);
}

void gdds__encode_bc3_block(const gdds_u8* block, gdds_u8 out_block[16]) {
    gdds_u8 alpha_values[16];
    gdds_u32 i;

    for (i = 0; i < 16u; ++i) {
        alpha_values[i] = block[i * 4u + 3u];
    }
    gdds__encode_bc_alpha_values(alpha_values, out_block);
    gdds__encode_bc1_color_block(block, 0, out_block + 8);
}

void gdds__encode_bc4_block_from_rgba(const gdds_u8* block, gdds_u32 channel_index, gdds_u8 out_block[8]) {
    gdds_u8 channel_values[16];
    gdds_u32 i;

    if (channel_index > 3u) channel_index = 0u;
    for (i = 0; i < 16u; ++i) {
        channel_values[i] = block[i * 4u + channel_index];
    }
    gdds__encode_bc_alpha_values(channel_values, out_block);
}

void gdds__encode_bc5_block_from_rgba(const gdds_u8* block,
                                      gdds_u32 red_channel_index,
                                      gdds_u32 green_channel_index,
                                      gdds_u8 out_block[16]) {
    gdds__encode_bc4_block_from_rgba(block, red_channel_index, out_block);
    gdds__encode_bc4_block_from_rgba(block, green_channel_index, out_block + 8);
}

void gdds__encode_bc4s_block_from_rgba(const gdds_u8* block, gdds_u32 channel_index, gdds_u8 out_block[8]) {
    gdds_u8 channel_values[16];
    gdds_u32 i;

    if (channel_index > 3u) channel_index = 0u;
    for (i = 0; i < 16u; ++i) {
        channel_values[i] = block[i * 4u + channel_index];
    }
    gdds__encode_bc_snorm_values(channel_values, out_block);
}

void gdds__encode_bc5s_block_from_rgba(const gdds_u8* block,
                                       gdds_u32 red_channel_index,
                                       gdds_u32 green_channel_index,
                                       gdds_u8 out_block[16]) {
    gdds__encode_bc4s_block_from_rgba(block, red_channel_index, out_block);
    gdds__encode_bc4s_block_from_rgba(block, green_channel_index, out_block + 8);
}
