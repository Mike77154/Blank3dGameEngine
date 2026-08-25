#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"

static unsigned int phase3_xorshift32(unsigned int *state)
{
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static int phase3_make_payload(const char *mode,
                               unsigned char **out_buf,
                               size_t *out_size)
{
    unsigned char *buf = NULL;
    size_t size = 0u;
    size_t i;

    if (!mode || !out_buf || !out_size)
        return 0;

    if (strcmp(mode, "dynamic") == 0) {
        size = 3000u;
        buf = (unsigned char *)zragf_p89_host_take(size);
        if (!buf) return 0;
        for (i = 0u; i < 1000u; ++i) buf[i] = (unsigned char)'a';
        for (; i < 2000u; ++i) buf[i] = (unsigned char)'b';
        for (; i < 3000u; ++i) buf[i] = (unsigned char)'c';
    } else if (strcmp(mode, "fixed") == 0) {
        size = 1024u;
        buf = (unsigned char *)zragf_p89_host_take(size);
        if (!buf) return 0;
        for (i = 0u; i < size; ++i)
            buf[i] = (unsigned char)(i & 0xFFu);
    } else if (strcmp(mode, "stored") == 0) {
        unsigned int state = 0x12345678u;
        size = 8192u;
        buf = (unsigned char *)zragf_p89_host_take(size);
        if (!buf) return 0;
        for (i = 0u; i < size; ++i)
            buf[i] = (unsigned char)(phase3_xorshift32(&state) & 0xFFu);
    } else {
        return 0;
    }

    *out_buf = buf;
    *out_size = size;
    return 1;
}

int main(int argc, char **argv)
{
    unsigned char *payload = NULL;
    unsigned char *comp = NULL;
    size_t payload_size = 0u;
    size_t comp_cap;
    zragf_stream zs;
    int rc;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <dynamic|fixed|stored>\n", argv[0]);
        return 2;
    }
    if (!phase3_make_payload(argv[1], &payload, &payload_size)) {
        fprintf(stderr, "unknown or failed payload case\n");
        return 3;
    }

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, 0);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(payload);
        fprintf(stderr, "deflateInit2 failed: %d\n", rc);
        return 4;
    }

    comp_cap = payload_size * 2u + 1024u;
    comp = (unsigned char *)zragf_p89_host_take(comp_cap);
    if (!comp) {
        zragf_deflateEndZ(&zs);
        zragf_p89_host_release(payload);
        return 5;
    }

    zs.next_in = payload;
    zs.avail_in = payload_size;
    zs.next_out = comp;
    zs.avail_out = comp_cap;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "deflate failed: %d\n", rc);
        zragf_deflateEndZ(&zs);
        zragf_p89_host_release(payload);
        zragf_p89_host_release(comp);
        return 6;
    }

    fwrite(comp, 1u, comp_cap - zs.avail_out, stdout);
    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(payload);
    zragf_p89_host_release(comp);
    return 0;
}
