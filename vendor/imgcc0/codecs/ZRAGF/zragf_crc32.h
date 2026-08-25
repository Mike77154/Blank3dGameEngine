#ifndef ZRAGF_CRC32_H_INCLUDED
#define ZRAGF_CRC32_H_INCLUDED

#include "zragflib_internal.h"

/* CRC32 estándar (polinomio 0xEDB88320, inicial 0xFFFFFFFF) */
static ZRAGF_MAYBE_UNUSED ZRAGF_INLINE zragf_u32 zragf_crc32(const zragf_u8 *buf, zragf_size_t size)
{
    zragf_u32 crc = 0xFFFFFFFFu;
    zragf_size_t i, j;

    if (size == 0)
        return 0u;
    if (!buf)
        return 0u;

    for (i = 0; i < size; ++i) {
        crc ^= (zragf_u32)buf[i];
        for (j = 0; j < 8u; ++j) {
            zragf_u32 mask = (zragf_u32)(0u - (crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return (zragf_u32)(~crc);
}

#endif /* ZRAGF_CRC32_H_INCLUDED */
