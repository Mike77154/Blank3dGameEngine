#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zragflib.h"

static int inflate_known_raw_streams(void)
{
    static const unsigned char fixed_stream[] = {
        0xcb,0x48,0xcd,0xc9,0xc9,0x57,0xc8,0x40,0x22,0xcb,0xf3,
        0x8b,0x72,0x52,0x90,0xc9,0x0c,0xaa,0x28,0x01,0x00
    };
    static const unsigned char fixed_payload[] =
        "hello hello hello world world world"
        "hello hello hello world world world"
        "hello hello hello world world world";

    static const unsigned char dynamic_stream[] = {
        0xed,0xc1,0x01,0x0d,0x00,0x00,0x0c,0x02,0xa0,0xac,0xfe,0xfd,
        0x3b,0xd8,0xc3,0x01,0x09,0xb0,0xee,0x80,0x79,0x0f,0xcc,0x2b
    };
    unsigned char dynamic_payload[3000];
    unsigned char out[4096];
    zragf_stream is;
    int rc;
    size_t i;

    for (i = 0; i < 1000u; ++i) dynamic_payload[i] = 'a';
    for (; i < 2000u; ++i) dynamic_payload[i] = 'b';
    for (; i < 3000u; ++i) dynamic_payload[i] = 'c';

    memset(&is, 0, sizeof(is));
    rc = zragf_inflateInit2(&is, -15);
    if (rc != ZRAGF_OK) return 1;
    is.next_in = (zragf_u8 *)fixed_stream;
    is.avail_in = sizeof(fixed_stream);
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "inflate fixed stream failed: %d\n", rc);
        return 1;
    }
    if ((sizeof(out) - is.avail_out) != (sizeof(fixed_payload) - 1u) ||
        memcmp(out, fixed_payload, sizeof(fixed_payload) - 1u) != 0) {
        fprintf(stderr, "inflate fixed payload mismatch\n");
        return 1;
    }

    memset(&is, 0, sizeof(is));
    rc = zragf_inflateInit2(&is, -15);
    if (rc != ZRAGF_OK) return 1;
    is.next_in = (zragf_u8 *)dynamic_stream;
    is.avail_in = sizeof(dynamic_stream);
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "inflate dynamic stream failed: %d\n", rc);
        return 1;
    }
    if ((sizeof(out) - is.avail_out) != sizeof(dynamic_payload) ||
        memcmp(out, dynamic_payload, sizeof(dynamic_payload)) != 0) {
        fprintf(stderr, "inflate dynamic payload mismatch\n");
        return 1;
    }

    return 0;
}

static int raw_wrapper_uses_fixed_block(void)
{
    const unsigned char payload[] =
        "phase2 phase2 phase2 phase2 phase2 phase2 "
        "phase2 phase2 phase2 phase2 phase2 phase2 ";
    unsigned char comp[1024];
    zragf_stream zs;
    int rc;
    size_t comp_size;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, 0);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateInit2 raw failed: %d\n", rc);
        return 1;
    }

    zs.next_in = (zragf_u8 *)payload;
    zs.avail_in = sizeof(payload) - 1u;
    zs.next_out = comp;
    zs.avail_out = sizeof(comp);
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    comp_size = sizeof(comp) - zs.avail_out;
    zragf_deflateEndZ(&zs);

    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "raw deflate finish failed: %d\n", rc);
        return 1;
    }
    if (comp_size == 0u) {
        fprintf(stderr, "raw deflate produced empty stream\n");
        return 1;
    }
    if ((comp[0] & 0x07u) != 0x03u) {
        fprintf(stderr, "expected BFINAL=1 BTYPE=01, got 0x%02x\n", (unsigned)(comp[0] & 0x07u));
        return 1;
    }
    return 0;
}

int main(void)
{
    if (inflate_known_raw_streams()) return 1;
    if (raw_wrapper_uses_fixed_block()) return 1;
    puts("phase2 fixed/dynamic smoke ok");
    return 0;
}
