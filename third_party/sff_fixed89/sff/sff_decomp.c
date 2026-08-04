#include "sff_decomp.h"
#include "sff_endian.h"
#include <string.h>

int sff_decomp_rle8_sff(const sff_u8 *src, sff_u32 src_len,
                        sff_u16 w, sff_u16 h,
                        sff_u8 *dst, sff_u32 dst_len)
{
    sff_u32 i;
    sff_u32 j;
    sff_u32 rawsize;

    (void)w;
    (void)h;

    if (!dst) return 0;

    memset(dst, 0, (size_t)dst_len);
    if (!src || src_len == 0u) return 1;

    if (src_len >= 4u) {
        rawsize = sff_rd_le_u32(src);
        if (rawsize == dst_len) {
            src += 4u;
            src_len -= 4u;
        }
    }

    i = 0u;
    j = 0u;
    while (i < src_len && j < dst_len) {
        sff_u8 d;
        d = src[i++];
        if ((d & 0xC0u) == 0x40u) {
            sff_u32 count;
            sff_u32 k;
            sff_u8 val;

            count = (sff_u32)(d & 0x3Fu);
            if (count == 0u) continue;
            if (i >= src_len) break;
            val = src[i++];
            k = count;
            if (k > (dst_len - j)) k = dst_len - j;
            memset(dst + j, (int)val, (size_t)k);
            j += k;
        } else {
            dst[j++] = d;
        }
    }

    return 1;
}

int sff_decomp_rle5(const sff_u8 *src, sff_u32 src_len,
                    sff_u16 w, sff_u16 h,
                    sff_u8 *dst, sff_u32 dst_len)
{
    sff_u32 i;
    sff_u32 j;
    sff_u32 rawsize;

    (void)w;
    (void)h;

    if (!dst) return 0;
    memset(dst, 0, (size_t)dst_len);
    if (!src || src_len == 0u) return 1;

    if (src_len >= 4u) {
        rawsize = sff_rd_le_u32(src);
        if (rawsize == dst_len) {
            src += 4u;
            src_len -= 4u;
        }
    }

    i = 0u;
    j = 0u;
    while (j < dst_len && i < src_len) {
        sff_u8 rlen;
        sff_u8 second;
        int dlen;
        sff_u8 c;
        sff_u32 run;
        sff_u32 k;

        rlen = src[i++];
        if (i >= src_len) break;

        second = src[i];
        dlen = (int)(second & 0x7F);

        if ((second >> 7) != 0u) {
            if (i + 1u >= src_len) break;
            c = src[i + 1u];
            i += 2u;
        } else {
            c = 0u;
            i += 1u;
        }

        run = (sff_u32)rlen + 1u;
        k = run;
        if (k > (dst_len - j)) k = dst_len - j;
        memset(dst + j, (int)c, (size_t)k);
        j += k;

        while (dlen >= 0 && j < dst_len && i < src_len) {
            sff_u8 byte_;
            byte_ = src[i++];
            c = (sff_u8)(byte_ & 0x1Fu);
            rlen = (sff_u8)(byte_ >> 5);
            run = (sff_u32)rlen + 1u;
            k = run;
            if (k > (dst_len - j)) k = dst_len - j;
            memset(dst + j, (int)c, (size_t)k);
            j += k;
            --dlen;
        }
    }

    return 1;
}

int sff_decomp_lz5(const sff_u8 *src, sff_u32 src_len,
                   sff_u16 w, sff_u16 h,
                   sff_u8 *dst, sff_u32 dst_len)
{
    sff_u32 i;
    sff_u32 j;
    sff_u8 ct;
    sff_u32 s;
    sff_u32 rbc;
    sff_u32 rb;
    sff_u32 rawsize;

    (void)w;
    (void)h;

    if (!dst) return 0;
    memset(dst, 0, (size_t)dst_len);
    if (!src || src_len == 0u) return 1;

    if (src_len >= 4u) {
        rawsize = sff_rd_le_u32(src);
        if (rawsize == dst_len) {
            src += 4u;
            src_len -= 4u;
        }
    }

    if (src_len == 0u) return 1;

    i = 0u;
    j = 0u;
    s = 0u;
    rbc = 0u;
    rb = 0u;
    ct = src[i++];

    while (j < dst_len) {
        if ((ct & (1u << s)) != 0u) {
            sff_u32 d;
            sff_u32 size;
            sff_u32 run;
            sff_u32 t;

            if (i >= src_len) break;
            d = (sff_u32)src[i++];

            if ((d & 0x3Fu) == 0u) {
                if (i + 1u >= src_len) break;
                d = ((d << 2) | (sff_u32)src[i]);
                ++i;
                d += 1u;
                size = (sff_u32)src[i] + 2u;
                ++i;
            } else {
                rb |= (sff_u32)((d & 0xC0u) >> rbc);
                rbc += 2u;
                size = d & 0x3Fu;
                if (rbc < 8u) {
                    if (i >= src_len) break;
                    d = (sff_u32)src[i] + 1u;
                    ++i;
                } else {
                    d = rb + 1u;
                    rbc = 0u;
                    rb = 0u;
                }
            }

            if (d == 0u || d > j) break;

            run = size + 1u;
            for (t = 0u; t < run && j < dst_len; ++t) {
                dst[j] = dst[j - d];
                ++j;
            }
        } else {
            sff_u32 size;
            sff_u32 run;
            sff_u8 d;
            sff_u32 k;

            if (i >= src_len) break;
            d = src[i++];

            if ((d & 0xE0u) == 0u) {
                if (i >= src_len) break;
                size = (sff_u32)src[i] + 8u;
                ++i;
            } else {
                size = (sff_u32)(d >> 5);
                d = (sff_u8)(d & 0x1Fu);
            }

            /* LZ5 literal runs are encoded as an exact count, unlike the backref
               path which uses size+1. The old code treated literals as size+1,
               which over-expanded every literal token and visually smeared
               5-bit sprites. */
            run = size;
            k = run;
            if (k > (dst_len - j)) k = dst_len - j;
            memset(dst + j, (int)d, (size_t)k);
            j += k;
        }

        ++s;
        if (s >= 8u) {
            s = 0u;
            if (i >= src_len) break;
            ct = src[i++];
        }
    }

    return 1;
}
