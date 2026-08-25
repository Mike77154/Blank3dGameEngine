/* zragf_frame.c - implementación del manejo de frame */

#include "zragf_frame.h"

zragf_status
zragf_frame_write_header(zragf_u8 *out,
                         zragf_size_t out_cap,
                         zragf_u32 uncompressed_size,
                         zragf_u8 flags,
                         zragf_size_t *out_pos)
{
    if (!out || !out_pos)
        return ZRAGF_ERR_NULL_POINTER;

    if (out_cap < 12u)
        return ZRAGF_ERR_OUTPUT_TOO_SMALL;

    out[0] = (zragf_u8)ZRAGF_MAGIC0;
    out[1] = (zragf_u8)ZRAGF_MAGIC1;
    out[2] = (zragf_u8)ZRAGF_MAGIC2;
    out[3] = (zragf_u8)ZRAGF_MAGIC3;
    out[4] = (zragf_u8)ZRAGF_VERSION;
    out[5] = flags;
    out[6] = 0;
    out[7] = 0;
    zragf_write_u32_le(out + 8, uncompressed_size);

    *out_pos = 12u;
    return ZRAGF_OK;
}

zragf_status
zragf_frame_read_header(const zragf_u8 *in,
                        zragf_size_t in_size,
                        zragf_u32 *out_uncompressed_size,
                        zragf_u8 *out_flags,
                        zragf_size_t *in_pos)
{
    if (!in || !out_uncompressed_size || !in_pos || !out_flags)
        return ZRAGF_ERR_NULL_POINTER;

    if (in_size < 12u)
        return ZRAGF_ERR_CORRUPTED_DATA;

    if (in[0] != (zragf_u8)ZRAGF_MAGIC0 ||
        in[1] != (zragf_u8)ZRAGF_MAGIC1 ||
        in[2] != (zragf_u8)ZRAGF_MAGIC2 ||
        in[3] != (zragf_u8)ZRAGF_MAGIC3)
    {
        return ZRAGF_ERR_CORRUPTED_DATA;
    }

    if (in[4] != (zragf_u8)ZRAGF_VERSION)
        return ZRAGF_ERR_CORRUPTED_DATA;

    *out_flags = in[5];
    *out_uncompressed_size = zragf_read_u32_le(in + 8);
    *in_pos = 12u;

    return ZRAGF_OK;
}

