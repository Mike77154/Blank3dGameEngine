#include "psd89_internal.h"

#include <string.h>

int psd89_io_read(psd89_io *io, void *dst, psd89_u32 size)
{
    if (io == 0 || io->read == 0) {
        return 0;
    }
    return io->read(io->user, dst, size);
}

int psd89_io_write(psd89_io *io, const void *src, psd89_u32 size)
{
    if (io == 0 || io->write == 0) {
        return 0;
    }
    return io->write(io->user, src, size);
}

int psd89_io_seek(psd89_io *io, psd89_u32 offset)
{
    if (io == 0 || io->seek == 0) {
        return 0;
    }
    return io->seek(io->user, offset);
}

psd89_u32 psd89_io_tell(psd89_io *io)
{
    if (io == 0 || io->tell == 0) {
        return 0U;
    }
    return io->tell(io->user);
}

int psd89_rd_u8(psd89_io *io, psd89_u8 *v)
{
    return psd89_io_read(io, v, 1U);
}

int psd89_rd_be16(psd89_io *io, psd89_u16 *v)
{
    psd89_u8 b[2];
    if (!psd89_io_read(io, b, 2U)) {
        return 0;
    }
    *v = (psd89_u16)(((psd89_u16)b[0] << 8) | (psd89_u16)b[1]);
    return 1;
}

int psd89_rd_be16s(psd89_io *io, psd89_s16 *v)
{
    psd89_u16 u;
    if (!psd89_rd_be16(io, &u)) {
        return 0;
    }
    *v = (psd89_s16)u;
    return 1;
}

int psd89_rd_be32(psd89_io *io, psd89_u32 *v)
{
    psd89_u8 b[4];
    if (!psd89_io_read(io, b, 4U)) {
        return 0;
    }
    *v = ((psd89_u32)b[0] << 24) |
         ((psd89_u32)b[1] << 16) |
         ((psd89_u32)b[2] << 8) |
         (psd89_u32)b[3];
    return 1;
}

int psd89_rd_be32s(psd89_io *io, psd89_s32 *v)
{
    psd89_u32 u;
    if (!psd89_rd_be32(io, &u)) {
        return 0;
    }
    *v = (psd89_s32)u;
    return 1;
}

int psd89_wr_u8(psd89_io *io, psd89_u8 v)
{
    return psd89_io_write(io, &v, 1U);
}

int psd89_wr_be16(psd89_io *io, psd89_u16 v)
{
    psd89_u8 b[2];
    b[0] = (psd89_u8)((v >> 8) & 0xFFU);
    b[1] = (psd89_u8)(v & 0xFFU);
    return psd89_io_write(io, b, 2U);
}

int psd89_wr_be32(psd89_io *io, psd89_u32 v)
{
    psd89_u8 b[4];
    b[0] = (psd89_u8)((v >> 24) & 0xFFU);
    b[1] = (psd89_u8)((v >> 16) & 0xFFU);
    b[2] = (psd89_u8)((v >> 8) & 0xFFU);
    b[3] = (psd89_u8)(v & 0xFFU);
    return psd89_io_write(io, b, 4U);
}

int psd89_skip(psd89_io *io, psd89_u32 size)
{
    psd89_u32 pos;
    psd89_u8 buf[256];
    psd89_u32 chunk;
    if (size == 0U) {
        return 1;
    }
    if (io != 0 && io->seek != 0 && io->tell != 0) {
        pos = psd89_io_tell(io);
        return psd89_io_seek(io, pos + size);
    }
    while (size != 0U) {
        chunk = size > (psd89_u32)sizeof(buf) ? (psd89_u32)sizeof(buf) : size;
        if (!psd89_io_read(io, buf, chunk)) {
            return 0;
        }
        size -= chunk;
    }
    return 1;
}

int psd89_read_pascal_string(psd89_io *io, char *dst, psd89_u32 dst_size, psd89_u32 pad_multiple)
{
    psd89_u8 len;
    psd89_u32 total;
    psd89_u32 padded;
    psd89_u32 i;
    psd89_u8 buf[256];

    if (!psd89_rd_u8(io, &len)) {
        return 0;
    }
    total = 1U + (psd89_u32)len;
    if (len != 0U && !psd89_io_read(io, buf, (psd89_u32)len)) {
        return 0;
    }
    if (dst != 0 && dst_size != 0U) {
        psd89_u32 copy_n;
        copy_n = (psd89_u32)len;
        if (copy_n >= dst_size) {
            copy_n = dst_size - 1U;
        }
        for (i = 0U; i < copy_n; ++i) {
            dst[i] = (char)buf[i];
        }
        dst[copy_n] = '\0';
    }
    if (pad_multiple == 0U) {
        pad_multiple = 1U;
    }
    padded = total;
    while ((padded % pad_multiple) != 0U) {
        ++padded;
    }
    return psd89_skip(io, padded - total);
}

int psd89_write_pascal_string(psd89_io *io, const char *src, psd89_u32 pad_multiple)
{
    psd89_u32 len;
    psd89_u32 total;
    if (src == 0) {
        src = "";
    }
    len = 0U;
    while (src[len] != '\0' && len < 255U) {
        ++len;
    }
    if (!psd89_wr_u8(io, (psd89_u8)len)) {
        return 0;
    }
    if (len != 0U && !psd89_io_write(io, src, len)) {
        return 0;
    }
    if (pad_multiple == 0U) {
        pad_multiple = 1U;
    }
    total = 1U + len;
    while ((total % pad_multiple) != 0U) {
        if (!psd89_wr_u8(io, 0U)) {
            return 0;
        }
        ++total;
    }
    return 1;
}

int psd89_copy_bytes(psd89_io *dst, psd89_io *src, psd89_u32 src_offset, psd89_u32 size)
{
    psd89_u8 buf[4096];
    psd89_u32 chunk;
    if (!psd89_io_seek(src, src_offset)) {
        return 0;
    }
    while (size != 0U) {
        chunk = size > (psd89_u32)sizeof(buf) ? (psd89_u32)sizeof(buf) : size;
        if (!psd89_io_read(src, buf, chunk) || !psd89_io_write(dst, buf, chunk)) {
            return 0;
        }
        size -= chunk;
    }
    return 1;
}

int psd89_patch_be32(psd89_io *io, psd89_u32 offset, psd89_u32 value)
{
    psd89_u32 pos;
    pos = psd89_io_tell(io);
    if (!psd89_io_seek(io, offset) || !psd89_wr_be32(io, value) || !psd89_io_seek(io, pos)) {
        return 0;
    }
    return 1;
}

int psd89_packbits_decode_row(psd89_io *io, psd89_u8 *dst, psd89_u32 row_bytes, psd89_u32 encoded_size)
{
    psd89_u32 out;
    psd89_u32 used;
    psd89_u8 ctrl;
    psd89_s8 sc;
    psd89_u32 count;
    psd89_u8 val;

    out = 0U;
    used = 0U;
    while (used < encoded_size && out < row_bytes) {
        if (!psd89_io_read(io, &ctrl, 1U)) {
            return 0;
        }
        ++used;
        sc = (psd89_s8)ctrl;
        if (sc >= 0) {
            count = (psd89_u32)sc + 1U;
            if (used + count > encoded_size || out + count > row_bytes) {
                return 0;
            }
            if (!psd89_io_read(io, dst + out, count)) {
                return 0;
            }
            used += count;
            out += count;
        } else if (sc >= -127) {
            count = (psd89_u32)(1 - sc);
            if (out + count > row_bytes) {
                return 0;
            }
            if (!psd89_io_read(io, &val, 1U)) {
                return 0;
            }
            ++used;
            while (count-- != 0U) {
                dst[out++] = val;
            }
        }
    }
    return used == encoded_size && out == row_bytes;
}

static psd89_u32 psd89_packbits_literal_len(psd89_u32 count)
{
    return 1U + count;
}

psd89_u32 psd89_packbits_encoded_len(const psd89_u8 *src, psd89_u32 row_bytes)
{
    psd89_u32 i;
    psd89_u32 len;
    psd89_u32 run;
    psd89_u32 lit;

    if (src == 0) {
        return ((row_bytes + 127U) / 128U) * 2U;
    }

    i = 0U;
    len = 0U;
    while (i < row_bytes) {
        run = 1U;
        while (i + run < row_bytes && run < 128U && src[i + run] == src[i]) {
            ++run;
        }
        if (run >= 3U) {
            len += 2U;
            i += run;
            continue;
        }
        lit = 0U;
        while (i + lit < row_bytes) {
            run = 1U;
            while (i + lit + run < row_bytes && run < 128U && src[i + lit + run] == src[i + lit]) {
                ++run;
            }
            if (run >= 3U || lit == 128U) {
                break;
            }
            ++lit;
        }
        len += psd89_packbits_literal_len(lit);
        i += lit;
    }
    return len;
}

int psd89_packbits_write_row(psd89_io *io, const psd89_u8 *src, psd89_u32 row_bytes)
{
    psd89_u32 i;
    psd89_u32 run;
    psd89_u32 lit;
    psd89_u8 hdr;

    if (src == 0) {
        return psd89_packbits_write_zero_row(io, row_bytes);
    }

    i = 0U;
    while (i < row_bytes) {
        run = 1U;
        while (i + run < row_bytes && run < 128U && src[i + run] == src[i]) {
            ++run;
        }
        if (run >= 3U) {
            hdr = (psd89_u8)(257U - run);
            if (!psd89_io_write(io, &hdr, 1U) || !psd89_io_write(io, src + i, 1U)) {
                return 0;
            }
            i += run;
            continue;
        }
        lit = 0U;
        while (i + lit < row_bytes) {
            run = 1U;
            while (i + lit + run < row_bytes && run < 128U && src[i + lit + run] == src[i + lit]) {
                ++run;
            }
            if (run >= 3U || lit == 128U) {
                break;
            }
            ++lit;
        }
        hdr = (psd89_u8)(lit - 1U);
        if (!psd89_io_write(io, &hdr, 1U) || !psd89_io_write(io, src + i, lit)) {
            return 0;
        }
        i += lit;
    }
    return 1;
}

int psd89_packbits_write_zero_row(psd89_io *io, psd89_u32 row_bytes)
{
    psd89_u8 hdr;
    psd89_u8 zero;
    psd89_u32 run;
    zero = 0U;
    while (row_bytes != 0U) {
        run = row_bytes > 128U ? 128U : row_bytes;
        hdr = (psd89_u8)(257U - run);
        if (!psd89_io_write(io, &hdr, 1U) || !psd89_io_write(io, &zero, 1U)) {
            return 0;
        }
        row_bytes -= run;
    }
    return 1;
}

static psd89_u32 psd89_be32_from_bytes(const psd89_u8 *p)
{
    return ((psd89_u32)p[0] << 24) |
           ((psd89_u32)p[1] << 16) |
           ((psd89_u32)p[2] << 8) |
           (psd89_u32)p[3];
}

static void psd89_be32_to_bytes(psd89_u8 *p, psd89_u32 v)
{
    p[0] = (psd89_u8)(v >> 24);
    p[1] = (psd89_u8)(v >> 16);
    p[2] = (psd89_u8)(v >> 8);
    p[3] = (psd89_u8)v;
}

static psd89_u32 psd89_ieee_q16_round_mantissa(psd89_u32 mh,
                                                psd89_u32 ml,
                                                unsigned int shift)
{
    psd89_u32 q;
    psd89_u32 mask;
    psd89_u32 remh;
    psd89_u32 halfh;

    if (shift < 32U) {
        q = (mh << (32U - shift)) | (ml >> shift);
        mask = ((psd89_u32)1U << shift) - 1U;
        if ((ml & mask) >= ((psd89_u32)1U << (shift - 1U))) {
            q += 1U;
        }
        return q;
    }
    if (shift == 32U) {
        q = mh;
        if (ml >= 0x80000000U) {
            q += 1U;
        }
        return q;
    }

    q = mh >> (shift - 32U);
    if (shift - 32U >= 32U) {
        remh = mh;
    } else {
        mask = ((psd89_u32)1U << (shift - 32U)) - 1U;
        remh = mh & mask;
    }
    halfh = (psd89_u32)1U << (shift - 33U);
    if (remh >= halfh) {
        q += 1U;
    }
    return q;
}

int psd89_rd_ieee8_q16(psd89_io *io, psd89_fx16 *out)
{
    psd89_u8 be[8];
    psd89_u32 hi;
    psd89_u32 lo;
    psd89_u32 mh;
    psd89_u32 mag;
    unsigned int exp;
    int e;
    unsigned int shift;
    int negative;

    if (out != 0) {
        *out = 0;
    }
    if (!psd89_io_read(io, be, 8U)) {
        return 0;
    }
    hi = psd89_be32_from_bytes(be);
    lo = psd89_be32_from_bytes(be + 4);
    negative = (hi & 0x80000000U) != 0U;
    exp = (unsigned int)((hi >> 20) & 0x7FFU);

    if (exp == 0x7FFU) {
        if ((hi & 0x000FFFFFU) != 0U || lo != 0U) {
            return 1;
        }
        if (out != 0) {
            *out = negative ? (psd89_fx16)(-2147483647 - 1) : (psd89_fx16)2147483647;
        }
        return 1;
    }
    if (exp == 0U) {
        return 1;
    }

    e = (int)exp - 1023;
    if (e < -17) {
        return 1;
    }
    if (e >= 15) {
        if (out != 0) {
            *out = negative ? (psd89_fx16)(-2147483647 - 1) : (psd89_fx16)2147483647;
        }
        return 1;
    }

    mh = (hi & 0x000FFFFFU) | 0x00100000U;
    shift = (unsigned int)(36 - e);
    mag = psd89_ieee_q16_round_mantissa(mh, lo, shift);

    if (!negative) {
        if (mag > 0x7FFFFFFFU) {
            mag = 0x7FFFFFFFU;
        }
        if (out != 0) {
            *out = (psd89_fx16)(psd89_s32)mag;
        }
    } else {
        if (mag >= 0x80000000U) {
            if (out != 0) {
                *out = (psd89_fx16)(-2147483647 - 1);
            }
        } else if (out != 0) {
            *out = (psd89_fx16)(psd89_s32)(0U - mag);
        }
    }
    return 1;
}

int psd89_wr_ieee8_from_q16(psd89_io *io, psd89_fx16 value)
{
    psd89_u8 be[8];
    psd89_u32 mag;
    psd89_u32 hi_mant;
    psd89_u32 lo_mant;
    psd89_u32 hi;
    unsigned int top;
    unsigned int shift;
    unsigned int exp;
    int negative;

    memset(be, 0, sizeof(be));
    if (value == 0) {
        return psd89_io_write(io, be, 8U);
    }

    negative = value < 0;
    mag = (psd89_u32)value;
    if (negative) {
        mag = (psd89_u32)(0U - mag);
    }

    top = 31U;
    while (top != 0U && ((mag >> top) & 1U) == 0U) {
        --top;
    }
    exp = top + 1007U;
    shift = 52U - top;
    if (shift >= 32U) {
        hi_mant = mag << (shift - 32U);
        lo_mant = 0U;
    } else {
        hi_mant = mag >> (32U - shift);
        lo_mant = mag << shift;
    }
    hi = ((psd89_u32)exp << 20) | (hi_mant & 0x000FFFFFU);
    if (negative) {
        hi |= 0x80000000U;
    }
    psd89_be32_to_bytes(be, hi);
    psd89_be32_to_bytes(be + 4, lo_mant);
    return psd89_io_write(io, be, 8U);
}

static void psd89_resolve_mask_rect(const psd89_layer *layer,
                                    psd89_u8 flags,
                                    psd89_s32 *top,
                                    psd89_s32 *left,
                                    psd89_s32 *bottom,
                                    psd89_s32 *right)
{
    if (layer == 0 || (flags & 0x01U) == 0U) {
        return;
    }
    if (top != 0) {
        *top += layer->top;
    }
    if (left != 0) {
        *left += layer->left;
    }
    if (bottom != 0) {
        *bottom += layer->top;
    }
    if (right != 0) {
        *right += layer->left;
    }
}

int psd89_layer_mask_info_for_channel(const psd89_layer *layer,
                                      psd89_s16 channel_id,
                                      psd89_s32 *top,
                                      psd89_s32 *left,
                                      psd89_s32 *bottom,
                                      psd89_s32 *right,
                                      psd89_u8 *default_color,
                                      psd89_u8 *flags)
{
    psd89_u8 mask_flags;

    if (layer == 0) {
        return 0;
    }
    if (channel_id == PSD89_CH_REAL_LAYER_MASK && layer->user_mask.real_present) {
        if (top != 0) *top = layer->user_mask.real_top;
        if (left != 0) *left = layer->user_mask.real_left;
        if (bottom != 0) *bottom = layer->user_mask.real_bottom;
        if (right != 0) *right = layer->user_mask.real_right;
        if (default_color != 0) *default_color = layer->user_mask.real_background;
        mask_flags = layer->user_mask.real_flags;
        psd89_resolve_mask_rect(layer, mask_flags, top, left, bottom, right);
        if (flags != 0) *flags = mask_flags;
        return 1;
    }
    if (channel_id == PSD89_CH_LAYER_MASK && layer->user_mask.present) {
        if (top != 0) *top = layer->user_mask.top;
        if (left != 0) *left = layer->user_mask.left;
        if (bottom != 0) *bottom = layer->user_mask.bottom;
        if (right != 0) *right = layer->user_mask.right;
        if (default_color != 0) *default_color = layer->user_mask.default_color;
        mask_flags = layer->user_mask.flags;
        psd89_resolve_mask_rect(layer, mask_flags, top, left, bottom, right);
        if (flags != 0) *flags = mask_flags;
        return 1;
    }
    return 0;
}

int psd89_layer_channel_dims(const psd89_layer *layer,
                             psd89_s16 channel_id,
                             psd89_u32 *rows,
                             psd89_u32 *cols)
{
    psd89_s32 top;
    psd89_s32 left;
    psd89_s32 bottom;
    psd89_s32 right;
    if (layer == 0 || rows == 0 || cols == 0) {
        return 0;
    }
    if (psd89_layer_mask_info_for_channel(layer, channel_id, &top, &left, &bottom, &right, 0, 0)) {
        if (right > left && bottom > top) {
            *rows = (psd89_u32)(bottom - top);
            *cols = (psd89_u32)(right - left);
            return 1;
        }
    }
    if (layer->right <= layer->left || layer->bottom <= layer->top) {
        return 0;
    }
    *rows = (psd89_u32)(layer->bottom - layer->top);
    *cols = (psd89_u32)(layer->right - layer->left);
    return 1;
}
