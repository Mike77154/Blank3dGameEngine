#include "zragf_bitio.h"

zragf_status
zragf_bw_init(zragf_bitwriter *bw,
              zragf_u8 *buf,
              zragf_size_t cap)
{
    if (!bw || !buf)
        return ZRAGF_ERR_NULL_POINTER;
    bw->buf = buf;
    bw->cap = cap;
    bw->pos = 0;
    bw->bitbuf = 0;
    bw->bitcount = 0;
    return ZRAGF_OK;
}

zragf_status
zragf_bw_put_bits(zragf_bitwriter *bw,
                  zragf_u32 code,
                  zragf_u8 bits)
{
    zragf_u32 i;
    if (!bw)
        return ZRAGF_ERR_NULL_POINTER;
    if (bits == 0)
        return ZRAGF_OK;
    if (bits > 24)
        return ZRAGF_ERR_INTERNAL;
    for (i = 0; i < bits; ++i) {
        zragf_u32 shift = (zragf_u32)(bits - 1u - i);
        zragf_u32 bit = (code >> shift) & 1u;
        bw->bitbuf = (zragf_u32)((bw->bitbuf << 1) | bit);
        bw->bitcount++;
        if (bw->bitcount == 8u) {
            if (bw->pos >= bw->cap)
                return ZRAGF_ERR_OUTPUT_TOO_SMALL;
            bw->buf[bw->pos++] = (zragf_u8)bw->bitbuf;
            bw->bitbuf = 0;
            bw->bitcount = 0;
        }
    }
    return ZRAGF_OK;
}

zragf_status
zragf_bw_flush(zragf_bitwriter *bw,
               zragf_size_t *out_size)
{
    if (!bw || !out_size)
        return ZRAGF_ERR_NULL_POINTER;
    if (bw->bitcount > 0) {
        zragf_u32 pad = (zragf_u32)(8u - bw->bitcount);
        bw->bitbuf = (zragf_u32)(bw->bitbuf << pad);
        if (bw->pos >= bw->cap)
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;
        bw->buf[bw->pos++] = (zragf_u8)bw->bitbuf;
        bw->bitbuf = 0;
        bw->bitcount = 0;
    }
    *out_size = bw->pos;
    return ZRAGF_OK;
}

zragf_status
zragf_br_init(zragf_bitreader *br,
              const zragf_u8 *buf,
              zragf_size_t size)
{
    if (!br || !buf)
        return ZRAGF_ERR_NULL_POINTER;
    br->buf = buf;
    br->size = size;
    br->pos = 0;
    br->bitbuf = 0;
    br->bitcount = 0;
    return ZRAGF_OK;
}

zragf_status
zragf_br_get_bit(zragf_bitreader *br,
                 zragf_u32 *out_bit)
{
    if (!br || !out_bit)
        return ZRAGF_ERR_NULL_POINTER;
    if (br->bitcount == 0) {
        if (br->pos >= br->size)
            return ZRAGF_ERR_CORRUPTED_DATA;
        br->bitbuf = br->buf[br->pos++];
        br->bitcount = 8u;
    }
    *out_bit = (br->bitbuf >> 7) & 1u;
    br->bitbuf <<= 1;
    br->bitcount--;
    return ZRAGF_OK;
}
