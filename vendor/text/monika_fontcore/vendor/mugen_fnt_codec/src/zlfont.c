#include "mfont_internal.h"
#include "../third_party/puff/puff.h"

static mft_u32 zlf_crc_table[256];
static int zlf_crc_ready = 0;

static void zlf_crc_init(void)
{
    mft_u32 i;
    mft_u32 j;
    mft_u32 c;
    if (zlf_crc_ready) {
        return;
    }
    for (i = 0; i < 256UL; ++i) {
        c = i;
        for (j = 0; j < 8UL; ++j) {
            if (c & 1UL) {
                c = 0xEDB88320UL ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        zlf_crc_table[i] = c;
    }
    zlf_crc_ready = 1;
}

mft_u32 zlf_adler32(const mft_u8 *data, mft_u32 size)
{
    mft_u32 s1;
    mft_u32 s2;
    mft_u32 i;
    s1 = 1UL;
    s2 = 0UL;
    for (i = 0; i < size; ++i) {
        s1 = (s1 + data[i]) % 65521UL;
        s2 = (s2 + s1) % 65521UL;
    }
    return (s2 << 16) | s1;
}

mft_u32 zlf_crc32(const mft_u8 *data, mft_u32 size)
{
    mft_u32 c;
    mft_u32 i;
    zlf_crc_init();
    c = 0xFFFFFFFFUL;
    for (i = 0; i < size; ++i) {
        c = zlf_crc_table[(c ^ data[i]) & 0xFFUL] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFUL;
}

mft_u32 zlf_deflate_stored_bound(mft_u32 size)
{
    mft_u32 blocks;
    blocks = size / 65535UL;
    if ((size % 65535UL) != 0UL || size == 0UL) {
        blocks++;
    }
    return 2UL + (blocks * 5UL) + size + 4UL;
}

mft_status zlf_inflate_zlib(mft_u8 *dst,
                            mft_u32 *dst_size,
                            const mft_u8 *src,
                            mft_u32 src_size,
                            int verify_adler)
{
    mft_u8 cmf;
    mft_u8 flg;
    unsigned long out_len;
    unsigned long in_len;
    int ret;
    mft_u32 expected;
    mft_u32 actual;

    if (dst == 0 || dst_size == 0 || src == 0) {
        return MFT_ERR_ARGS;
    }
    if (src_size < 6UL) {
        return MFT_ERR_FORMAT;
    }
    cmf = src[0];
    flg = src[1];
    if ((cmf & 15U) != 8U) {
        return MFT_ERR_UNSUPPORTED;
    }
    if ((cmf >> 4) > 7U) {
        return MFT_ERR_UNSUPPORTED;
    }
    if ((((mft_u32)cmf << 8) + (mft_u32)flg) % 31UL != 0UL) {
        return MFT_ERR_FORMAT;
    }
    if ((flg & 0x20U) != 0U) {
        return MFT_ERR_UNSUPPORTED;
    }

    out_len = (unsigned long)(*dst_size);
    in_len = (unsigned long)(src_size - 6UL);
    ret = puff(dst, &out_len, src + 2, &in_len);
    if (ret == 1) {
        *dst_size = (mft_u32)out_len;
        return MFT_ERR_CAPACITY;
    }
    if (ret != 0) {
        return MFT_ERR_FORMAT;
    }
    if ((mft_u32)in_len != src_size - 6UL) {
        return MFT_ERR_FORMAT;
    }

    *dst_size = (mft_u32)out_len;
    if (verify_adler) {
        expected = mft_read_be32(src + src_size - 4UL);
        actual = zlf_adler32(dst, *dst_size);
        if (expected != actual) {
            return MFT_ERR_CHECKSUM;
        }
    }
    return MFT_OK;
}

mft_status zlf_deflate_stored_zlib(mft_u8 *dst,
                                   mft_u32 *dst_size,
                                   const mft_u8 *src,
                                   mft_u32 src_size)
{
    mft_u32 need;
    mft_u32 pos;
    mft_u32 left;
    mft_u32 block;
    mft_u32 adler;
    mft_u8 cmf;
    mft_u8 flg;
    mft_u32 check;

    if (dst_size == 0 || dst == 0 || src == 0) {
        return MFT_ERR_ARGS;
    }
    need = zlf_deflate_stored_bound(src_size);
    if (*dst_size < need) {
        *dst_size = need;
        return MFT_ERR_CAPACITY;
    }

    cmf = 0x78U;
    flg = 0U;
    check = (((mft_u32)cmf) << 8) | flg;
    flg = (mft_u8)((31UL - (check % 31UL)) % 31UL);

    pos = 0UL;
    dst[pos++] = cmf;
    dst[pos++] = flg;

    left = src_size;
    while (left > 0UL || src_size == 0UL) {
        int final_block;
        block = left;
        if (block > 65535UL) {
            block = 65535UL;
        }
        final_block = (left <= 65535UL) ? 1 : 0;
        if (src_size == 0UL) {
            final_block = 1;
            block = 0UL;
        }
        dst[pos++] = (mft_u8)(final_block ? 1U : 0U);
        mft_write_le16(dst + pos, (mft_u16)block);
        pos += 2UL;
        mft_write_le16(dst + pos, (mft_u16)(~block));
        pos += 2UL;
        if (block > 0UL) {
            memcpy(dst + pos, src + (src_size - left), (size_t)block);
            pos += block;
            left -= block;
        } else {
            break;
        }
    }
    adler = zlf_adler32(src, src_size);
    mft_write_be32(dst + pos, adler);
    pos += 4UL;
    *dst_size = pos;
    return MFT_OK;
}
