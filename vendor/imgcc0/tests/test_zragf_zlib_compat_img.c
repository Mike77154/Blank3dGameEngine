#include "zragflib.h"
#include <stdio.h>
#include <string.h>

static int test_inflate_exact_fill_empty_final(void)
{
    static const unsigned char stream[] = {
        0x78U,0x01U,
        0x00U,0x04U,0x00U,0xFBU,0xFFU,0x01U,0x02U,0x03U,0x04U,
        0x01U,0x00U,0x00U,0xFFU,0xFFU,
        0x00U,0x18U,0x00U,0x0BU
    };
    static const unsigned char expect[] = {1U,2U,3U,4U};
    unsigned char out[4];
    unsigned char arena[65536];
    zragf_stream z;
    zragf_workspace ws;
    int rc;

    memset(&z, 0, sizeof(z));
    zragf_workspace_init(&ws, arena, (zragf_size_t)sizeof(arena));
    zragf_stream_set_workspace(&z, &ws);
    z.next_in = (zragf_u8 *)stream;
    z.avail_in = (zragf_size_t)sizeof(stream);
    z.next_out = out;
    z.avail_out = (zragf_size_t)sizeof(out);
    rc = zragf_inflateInit(&z);
    if (rc != ZRAGF_OK) return 0;
    rc = zragf_inflateZ(&z, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_inflateEndZ(&z);
        return 0;
    }
    if (z.total_out != 4U || z.avail_out != 0U || memcmp(out, expect, 4U) != 0) {
        zragf_inflateEndZ(&z);
        return 0;
    }
    zragf_inflateEndZ(&z);
    return 1;
}

static int test_deflate_level0_zlib_exact(void)
{
    static const unsigned char input[] = {1U,2U,3U,4U};
    static const unsigned char expect[] = {
        0x78U,0x01U,0x01U,0x04U,0x00U,0xFBU,0xFFU,
        0x01U,0x02U,0x03U,0x04U,0x00U,0x18U,0x00U,0x0BU
    };
    unsigned char out[64];
    unsigned char arena[65536];
    zragf_stream z;
    zragf_workspace ws;
    int rc;

    memset(&z, 0, sizeof(z));
    zragf_workspace_init(&ws, arena, (zragf_size_t)sizeof(arena));
    zragf_stream_set_workspace(&z, &ws);
    z.next_in = (zragf_u8 *)input;
    z.avail_in = (zragf_size_t)sizeof(input);
    z.next_out = out;
    z.avail_out = (zragf_size_t)sizeof(out);
    rc = zragf_deflateInit2(&z, 0, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) return 0;
    rc = zragf_deflateZ(&z, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&z);
        return 0;
    }
    if (z.total_out != (zragf_u32)sizeof(expect) ||
        memcmp(out, expect, sizeof(expect)) != 0) {
        zragf_deflateEndZ(&z);
        return 0;
    }
    zragf_deflateEndZ(&z);
    return 1;
}

int main(void)
{
    if (!test_inflate_exact_fill_empty_final()) {
        puts("FAIL: inflate exact-fill empty-final");
        return 1;
    }
    if (!test_deflate_level0_zlib_exact()) {
        puts("FAIL: deflate level0 zlib byte-exact");
        return 1;
    }
    puts("PASS: ZRAGF zlib image compatibility regressions");
    return 0;
}
