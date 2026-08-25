#ifndef ZRAGF_BITIO_H_INCLUDED
#define ZRAGF_BITIO_H_INCLUDED

#include "zragflib_internal.h"

typedef struct zragf_bitwriter_s {
    zragf_u8 *buf;
    zragf_size_t cap;
    zragf_size_t pos;
    zragf_u32 bitbuf;
    zragf_u32 bitcount;
} zragf_bitwriter;

typedef struct zragf_bitreader_s {
    const zragf_u8 *buf;
    zragf_size_t size;
    zragf_size_t pos;
    zragf_u32 bitbuf;
    zragf_u32 bitcount;
} zragf_bitreader;

zragf_status
zragf_bw_init(zragf_bitwriter *bw,
              zragf_u8 *buf,
              zragf_size_t cap);

zragf_status
zragf_bw_put_bits(zragf_bitwriter *bw,
                  zragf_u32 code,
                  zragf_u8 bits);

zragf_status
zragf_bw_flush(zragf_bitwriter *bw,
               zragf_size_t *out_size);

zragf_status
zragf_br_init(zragf_bitreader *br,
              const zragf_u8 *buf,
              zragf_size_t size);

zragf_status
zragf_br_get_bit(zragf_bitreader *br,
                 zragf_u32 *out_bit);

#endif
