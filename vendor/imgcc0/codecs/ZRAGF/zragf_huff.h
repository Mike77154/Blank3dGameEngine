#ifndef ZRAGF_HUFF_H_INCLUDED
#define ZRAGF_HUFF_H_INCLUDED

#include "zragflib_internal.h"

typedef struct zragf_huff_tables_s {
    zragf_u16 code[ZRAGF_HUFF_SYMS];
    zragf_u8  bits[ZRAGF_HUFF_SYMS];
} zragf_huff_tables;

zragf_status
zragf_huff_build_from_freq(const zragf_u32 *freq,
                           zragf_huff_tables *ht,
                           zragf_u8 *out_max_bits);

zragf_status
zragf_huff_build_from_lengths(const zragf_u8 *lens,
                              zragf_u8 max_bits,
                              zragf_huff_tables *ht);

#endif
