#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "zragflib.h"

static int test_invalid_distance_too_far(void)
{
    /* Raw DEFLATE fixed block:
       BFINAL=1, BTYPE=01, length code 257 (len=3), dist code 0 (dist=1), EOB.
       No bytes were emitted before the match, so a compliant inflater must
       reject the stream as "distance too far back". */
    static const unsigned char raw[] = { 0x03u, 0x02u, 0x00u };
    zragf_stream is;
    unsigned char out[32];
    int rc;

    memset(&is, 0, sizeof(is));
    if (zragf_inflateInit2(&is, -15) != ZRAGF_OK)
        return 0;
    is.next_in = (zragf_u8 *)raw;
    is.avail_in = (unsigned int)sizeof(raw);
    is.next_out = out;
    is.avail_out = (unsigned int)sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    return rc == ZRAGF_DATA_ERROR;
}

static int test_bad_gzip_trailer_crc(void)
{
    const unsigned char payload[] = "phase17-gzip-trailer-check";
    z_stream zs;
    unsigned char comp[256];
    zragf_stream is;
    unsigned char out[128];
    int rc;

    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_BEST_SPEED, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return 0;
    zs.next_in = (Bytef *)payload;
    zs.avail_in = (uInt)(sizeof(payload) - 1u);
    zs.next_out = comp;
    zs.avail_out = (uInt)sizeof(comp);
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        return 0;
    }
    rc = deflateEnd(&zs);
    if (rc != Z_OK)
        return 0;

    /* Corrupt gzip trailer CRC32, keep body valid. */
    comp[zs.total_out - 8u] ^= 0x80u;

    memset(&is, 0, sizeof(is));
    if (zragf_inflateInit2(&is, 31) != ZRAGF_OK)
        return 0;
    is.next_in = comp;
    is.avail_in = (unsigned int)zs.total_out;
    is.next_out = out;
    is.avail_out = (unsigned int)sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    return rc == ZRAGF_DATA_ERROR;
}

static int test_good_gzip_trailer_split_incremental(void)
{
    const unsigned char payload[] = "split-gzip-trailer-ok";
    z_stream zs;
    unsigned char comp[256];
    zragf_stream is;
    unsigned char out[128];
    int rc;
    unsigned int produced = 0u;
    unsigned int split;

    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_BEST_SPEED, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return 0;
    zs.next_in = (Bytef *)payload;
    zs.avail_in = (uInt)(sizeof(payload) - 1u);
    zs.next_out = comp;
    zs.avail_out = (uInt)sizeof(comp);
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        return 0;
    }
    rc = deflateEnd(&zs);
    if (rc != Z_OK)
        return 0;

    memset(&is, 0, sizeof(is));
    if (zragf_inflateInit2(&is, 31) != ZRAGF_OK)
        return 0;
    memset(out, 0, sizeof(out));

    split = (unsigned int)zs.total_out - 3u; /* leave most of trailer for second call */
    is.next_in = comp;
    is.avail_in = split;
    is.next_out = out;
    is.avail_out = (unsigned int)sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_NO_FLUSH);
    if (rc != ZRAGF_OK) {
        zragf_inflateEndZ(&is);
        return 0;
    }
    produced = (unsigned int)(sizeof(out) - is.avail_out);
    if (produced != sizeof(payload) - 1u) {
        zragf_inflateEndZ(&is);
        return 0;
    }

    is.next_in = comp + split;
    is.avail_in = (unsigned int)zs.total_out - split;
    is.next_out = out + produced;
    is.avail_out = (unsigned int)(sizeof(out) - produced);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc != ZRAGF_STREAM_END)
        return 0;
    return memcmp(out, payload, sizeof(payload) - 1u) == 0;
}

int main(void)
{
    if (!test_invalid_distance_too_far()) {
        fprintf(stderr, "phase17: invalid back-reference was not rejected\n");
        return 1;
    }
    if (!test_bad_gzip_trailer_crc()) {
        fprintf(stderr, "phase17: bad gzip trailer CRC was not rejected\n");
        return 1;
    }
    if (!test_good_gzip_trailer_split_incremental()) {
        fprintf(stderr, "phase17: split gzip trailer incremental path failed\n");
        return 1;
    }

    printf("phase17 inflate hardening ok\n");
    return 0;
}
