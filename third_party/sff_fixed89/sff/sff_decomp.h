#ifndef SFF_DECOMP_H
#define SFF_DECOMP_H

#include "sff_types.h"

int sff_decomp_rle8_sff(const sff_u8 *src, sff_u32 src_len,
                        sff_u16 w, sff_u16 h,
                        sff_u8 *dst, sff_u32 dst_len);

int sff_decomp_rle5(const sff_u8 *src, sff_u32 src_len,
                    sff_u16 w, sff_u16 h,
                    sff_u8 *dst, sff_u32 dst_len);

int sff_decomp_lz5(const sff_u8 *src, sff_u32 src_len,
                   sff_u16 w, sff_u16 h,
                   sff_u8 *dst, sff_u32 dst_len);

#endif /* SFF_DECOMP_H */
