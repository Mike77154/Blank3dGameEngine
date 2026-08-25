#include <stdio.h>
#include <string.h>
#include "zragflib.h"

int main(void)
{
    const unsigned char payload[] = "corruption-test-payload";
    unsigned char comp[1024];
    unsigned char out[1024];
    zragf_size_t comp_size = sizeof(comp);
    zragf_size_t out_size = sizeof(out);
    zragf_info info;
    zragf_stream zs;
    zragf_stream is;
    int rc;

    if (zragf_compress(payload, sizeof(payload) - 1u, comp, &comp_size, ZRAGF_LEVEL_DEFAULT) != ZRAGF_ST_OK)
        return 1;
    comp[comp_size - 1u] ^= 0x5Au;
    if (zragf_decompress(comp, comp_size, out, &out_size, &info) == ZRAGF_ST_OK) {
        fprintf(stderr, "native corruption was not detected\n");
        return 1;
    }

    memset(&zs, 0, sizeof(zs));
    memset(&is, 0, sizeof(is));
    rc = zragf_deflateInit2(&zs, 5, 8, 31, 8, 0);
    if (rc != ZRAGF_OK)
        return 1;
    zs.next_in = (zragf_u8 *)payload;
    zs.avail_in = sizeof(payload) - 1u;
    zs.next_out = comp;
    zs.avail_out = sizeof(comp);
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    zragf_deflateEndZ(&zs);
    if (rc != ZRAGF_STREAM_END)
        return 1;
    comp_size = sizeof(comp) - zs.avail_out;
    comp[comp_size - 8u] ^= 0x01u; /* CRC32 trailer */

    rc = zragf_inflateInit2(&is, 31);
    if (rc != ZRAGF_OK)
        return 1;
    is.next_in = comp;
    is.avail_in = comp_size;
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc == ZRAGF_STREAM_END) {
        fprintf(stderr, "gzip corruption was not detected\n");
        return 1;
    }

    puts("corruption detection ok");
    return 0;
}
