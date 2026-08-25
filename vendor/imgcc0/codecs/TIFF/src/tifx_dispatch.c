/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

static unsigned short tifx_dispatch_read_u16(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return (unsigned short)(((unsigned short)p[0] << 8) | (unsigned short)p[1]);
    }
    return (unsigned short)(((unsigned short)p[1] << 8) | (unsigned short)p[0]);
}

static unsigned short tifx_effective_container(const tifx_write_params *params)
{
    if (params == 0 || params->container_format == TIFX_CONTAINER_AUTO) {
        return TIFX_CONTAINER_CLASSIC;
    }
    return params->container_format;
}

int tifx_parse_memory(tifx_image_info *info, const void *data, unsigned long size)
{
    const unsigned char *bytes;
    int is_big_endian;
    unsigned short version;

    if (info == 0 || data == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (size < 4UL) {
        return TIFX_ERR_TRUNCATED;
    }

    bytes = (const unsigned char *)data;
    if (bytes[0] == 'I' && bytes[1] == 'I') {
        is_big_endian = 0;
    } else if (bytes[0] == 'M' && bytes[1] == 'M') {
        is_big_endian = 1;
    } else {
        return TIFX_ERR_BAD_FORMAT;
    }

    version = tifx_dispatch_read_u16(bytes + 2, is_big_endian);
    if (version == 42U) {
        return tifx_parse_classic_memory(info, data, size);
    }
    if (version == 43U) {
        return tifx_parse_bigtiff_memory(info, data, size);
    }
    return TIFX_ERR_BAD_FORMAT;
}

unsigned long tifx_write_buffer_size(const tifx_write_params *params)
{
    if (tifx_effective_container(params) == TIFX_CONTAINER_BIGTIFF) {
        return tifx_write_bigtiff_buffer_size(params);
    }
    return tifx_write_classic_buffer_size(params);
}

int tifx_write_memory(void *dst,
                      unsigned long dst_size,
                      const tifx_write_params *params,
                      unsigned long *written_size)
{
    if (tifx_effective_container(params) == TIFX_CONTAINER_BIGTIFF) {
        return tifx_write_bigtiff_memory(dst, dst_size, params, written_size);
    }
    return tifx_write_classic_memory(dst, dst_size, params, written_size);
}
