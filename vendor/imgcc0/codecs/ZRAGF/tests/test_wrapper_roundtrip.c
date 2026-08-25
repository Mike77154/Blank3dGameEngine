#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zragflib.h"

static int roundtrip_wrapper(int windowBits)
{
    const unsigned char payload[] = "wrapper-stream-check: lorem ipsum lorem ipsum lorem ipsum";
    unsigned char comp[4096];
    unsigned char decomp[4096];
    zragf_stream zs;
    zragf_stream is;
    zragf_size_t comp_size;
    int rc;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 5, 8, windowBits, 8, 0);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateInit2 failed: %d\n", rc);
        return 1;
    }

    memset(comp, 0, sizeof(comp));
    zs.next_out = comp;
    zs.avail_out = sizeof(comp);

    zs.next_in = (zragf_u8 *)payload;
    zs.avail_in = 20u;
    rc = zragf_deflateZ(&zs, ZRAGF_SYNC_FLUSH);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "first deflate failed: %d\n", rc);
        return 1;
    }

    zs.next_in = (zragf_u8 *)(payload + 20u);
    zs.avail_in = sizeof(payload) - 1u - 20u;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "finish deflate failed: %d\n", rc);
        return 1;
    }
    comp_size = sizeof(comp) - zs.avail_out;
    zragf_deflateEndZ(&zs);

    memset(&is, 0, sizeof(is));
    rc = zragf_inflateInit2(&is, windowBits);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateInit2 failed: %d\n", rc);
        return 1;
    }

    memset(decomp, 0, sizeof(decomp));
    is.next_in = comp;
    is.avail_in = comp_size;
    is.next_out = decomp;
    is.avail_out = sizeof(decomp);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "inflate failed: %d\n", rc);
        return 1;
    }
    zragf_inflateEndZ(&is);

    if ((sizeof(payload) - 1u) != (sizeof(decomp) - is.avail_out)) {
        fprintf(stderr, "wrapper size mismatch\n");
        return 1;
    }
    if (memcmp(payload, decomp, sizeof(payload) - 1u) != 0) {
        fprintf(stderr, "wrapper payload mismatch\n");
        return 1;
    }
    return 0;
}

int main(void)
{
    if (roundtrip_wrapper(-15)) return 1; /* raw */
    if (roundtrip_wrapper(15)) return 1;  /* zlib */
    if (roundtrip_wrapper(31)) return 1;  /* gzip */
    puts("wrapper roundtrip ok");
    return 0;
}
