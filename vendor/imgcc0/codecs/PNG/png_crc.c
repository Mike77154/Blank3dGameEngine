/* png_crc.c - CRC32 for PNG chunks.
 *
 * PNG uses the standard IEEE CRC-32 polynomial.
 */

#include "png_decoder_internal.h"

static png_u32 png_crc_table[256];
static int png_crc_table_computed = 0;

static void png_make_crc_table(void)
{
    png_u32 c;
    int n, k;

    for (n = 0; n < 256; ++n)
    {
        c = (png_u32)n;
        for (k = 0; k < 8; ++k)
        {
            if (c & 1u)
                c = 0xEDB88320u ^ (c >> 1);
            else
                c = c >> 1;
        }
        png_crc_table[n] = c;
    }
    png_crc_table_computed = 1;
}

png_u32 png_crc32_start(void)
{
    if (!png_crc_table_computed)
        png_make_crc_table();
    return 0xFFFFFFFFu;
}

png_u32 png_crc32_update(png_u32 crc, const png_u8* buf, png_u32 len)
{
    png_u32 i;

    if (!png_crc_table_computed)
        png_make_crc_table();

    if (!buf || len == 0u)
        return crc;

    for (i = 0; i < len; ++i)
        crc = png_crc_table[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);

    return crc;
}

png_u32 png_crc32_finish(png_u32 crc)
{
    return crc ^ 0xFFFFFFFFu;
}

png_u32 png_crc32(const png_u8* buf, png_u32 len)
{
    png_u32 crc = png_crc32_start();
    crc = png_crc32_update(crc, buf, len);
    return png_crc32_finish(crc);
}
