#include <stdio.h>
#include <string.h>
#include "zragflib.h"

static int phase3_dynamic_roundtrip(void)
{
    unsigned char payload[3000];
    unsigned char comp[64];
    unsigned char out[4096];
    zragf_stream zs;
    zragf_stream is;
    int rc;
    size_t i;

    for (i = 0u; i < 1000u; ++i) payload[i] = (unsigned char)'a';
    for (; i < 2000u; ++i) payload[i] = (unsigned char)'b';
    for (; i < 3000u; ++i) payload[i] = (unsigned char)'c';

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, 0);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateInit2 raw failed: %d\n", rc);
        return 1;
    }

    zs.next_in = payload;
    zs.avail_in = sizeof(payload);
    zs.next_out = comp;
    zs.avail_out = sizeof(comp);
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "deflate finish failed: %d\n", rc);
        zragf_deflateEndZ(&zs);
        return 1;
    }
    zragf_deflateEndZ(&zs);

    if ((comp[0] & 0x07u) != 0x05u) {
        fprintf(stderr, "expected BFINAL=1 BTYPE=10, got 0x%02x\n", (unsigned)(comp[0] & 0x07u));
        return 1;
    }

    memset(&is, 0, sizeof(is));
    rc = zragf_inflateInit2(&is, -15);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateInit2 raw failed: %d\n", rc);
        return 1;
    }
    is.next_in = comp;
    is.avail_in = sizeof(comp) - zs.avail_out;
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "inflate raw failed: %d\n", rc);
        return 1;
    }

    if ((sizeof(out) - is.avail_out) != sizeof(payload) ||
        memcmp(out, payload, sizeof(payload)) != 0) {
        fprintf(stderr, "phase3 dynamic payload mismatch\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (phase3_dynamic_roundtrip()) return 1;
    puts("phase3 dynamic encode smoke ok");
    return 0;
}
