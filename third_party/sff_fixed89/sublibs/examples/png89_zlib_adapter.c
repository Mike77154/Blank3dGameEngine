#include "../png89/png89.h"
#include <zlib.h>
#include <string.h>

int png89_example_inflate_zlib(void *user,
                               const png89_u8 *src, png89_u32 src_size,
                               png89_u8 *dst, png89_u32 dst_size,
                               png89_u32 *out_written)
{
    z_stream zs;
    int zrc;
    (void)user;
    memset(&zs, 0, sizeof(zs));
    zs.next_in = (Bytef*)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = (Bytef*)dst;
    zs.avail_out = (uInt)dst_size;
    zrc = inflateInit(&zs);
    if (zrc != Z_OK) return -1;
    zrc = inflate(&zs, Z_FINISH);
    if (zrc != Z_STREAM_END) {
        inflateEnd(&zs);
        return -2;
    }
    if (out_written) *out_written = (png89_u32)zs.total_out;
    inflateEnd(&zs);
    return 0;
}
