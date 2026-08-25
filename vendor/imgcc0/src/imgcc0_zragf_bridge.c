#include <string.h>
#include "imgcc0_zragf_bridge.h"
#include "zragflib.h"
#include "zragf_protocol89_mem.h"

int imgcc0_zragf_png_decompress(png_u8 *dest, png_u32 *dest_len,
                                const png_u8 *src, png_u32 src_len)
{
    zragf_stream strm;
    png_u32 out_cap;
    zragf_u8 sink;
    int rc;
    int end_rc;

    if (dest == 0 || dest_len == 0 || (src == 0 && src_len != 0U))
        return -1;

    out_cap = *dest_len;
    sink = 0U;
    memset(&strm, 0, sizeof(strm));
    zragf_p89_reset();

    rc = zragf_inflateInit(&strm);
    if (rc != ZRAGF_OK) {
        zragf_p89_reset();
        return -1;
    }

    strm.next_in = (zragf_u8 *)src;
    strm.avail_in = (zragf_size_t)src_len;
    strm.next_out = (zragf_u8 *)dest;
    strm.avail_out = (zragf_size_t)out_cap;

    rc = zragf_inflateZ(&strm, ZRAGF_FINISH);

    if (rc == ZRAGF_OK && strm.avail_out == 0U &&
        strm.total_out == (zragf_u32)out_cap) {
        strm.next_out = &sink;
        strm.avail_out = 1U;
        rc = zragf_inflateZ(&strm, ZRAGF_FINISH);
        if (strm.total_out != (zragf_u32)out_cap)
            rc = ZRAGF_BUF_ERROR;
    }

    *dest_len = (png_u32)strm.total_out;
    end_rc = zragf_inflateEndZ(&strm);
    zragf_p89_reset();

    if (end_rc != ZRAGF_OK)
        return -1;
    return (rc == ZRAGF_STREAM_END) ? 0 : -1;
}
