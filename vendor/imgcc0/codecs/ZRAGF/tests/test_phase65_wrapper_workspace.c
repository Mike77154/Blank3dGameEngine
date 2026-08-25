#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

static unsigned g_alloc_calls = 0u;
static unsigned g_realloc_calls = 0u;
static unsigned g_free_calls = 0u;

static void *count_alloc(void *user, zragf_size_t size)
{
    (void)user;
    ++g_alloc_calls;
    return zragf_p89_host_take(size);
}

static void *count_realloc(void *user, void *ptr, zragf_size_t old_size, zragf_size_t new_size)
{
    (void)user;
    (void)old_size;
    ++g_realloc_calls;
    return zragf_p89_host_resize(ptr, new_size);
}

static void count_free(void *user, void *ptr)
{
    (void)user;
    ++g_free_calls;
    zragf_p89_host_release(ptr);
}

static int check_roundtrip(int windowBits)
{
    static const unsigned char payload[] =
        "phase65 wrapper workspace payload :: raw/zlib/gzip :: "
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    unsigned char comp[4096];
    unsigned char out[4096];
    zragf_stream dstrm;
    zragf_stream istrm;
    zragf_workspace dws;
    zragf_workspace iws;
    zragf_size_t dneed;
    zragf_size_t ineed;
    void *dbuf;
    void *ibuf;
    unsigned long comp_len;
    int rc;

    g_alloc_calls = 0u;
    g_realloc_calls = 0u;
    g_free_calls = 0u;

    dneed = zragf_deflate_workspace_bound_z((unsigned long)(sizeof(payload) - 1u), windowBits, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (dneed == 0u) {
        fprintf(stderr, "deflate workspace bound invalid\n");
        return 1;
    }
    dbuf = zragf_p89_host_take(dneed);
    if (!dbuf) {
        fprintf(stderr, "static deflate workspace failed\n");
        return 1;
    }

    memset(&dstrm, 0, sizeof(dstrm));
    zragf_workspace_init(&dws, dbuf, dneed);
    zragf_stream_set_workspace(&dstrm, &dws);

    rc = zragf_deflateInit2(&dstrm, ZRAGF_LEVEL_DEFAULT, 8, windowBits, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateInit2 workspace failed: %s\n", zragf_strerror(rc));
        zragf_p89_host_release(dbuf);
        return 1;
    }

    dstrm.next_in = (zragf_u8 *)payload;
    dstrm.avail_in = sizeof(payload) - 1u;
    dstrm.next_out = comp;
    dstrm.avail_out = sizeof(comp);

    for (;;) {
        rc = zragf_deflateZ(&dstrm, ZRAGF_FINISH);
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc != ZRAGF_OK) {
            fprintf(stderr, "deflateZ failed: %s\n", zragf_strerror(rc));
            zragf_deflateEndZ(&dstrm);
            zragf_p89_host_release(dbuf);
            return 1;
        }
        if (dstrm.avail_out == 0u) {
            fprintf(stderr, "compressed buffer too small\n");
            zragf_deflateEndZ(&dstrm);
            zragf_p89_host_release(dbuf);
            return 1;
        }
    }
    comp_len = (unsigned long)dstrm.total_out;
    zragf_deflateEndZ(&dstrm);

    if (zragf_workspace_failed(&dws)) {
        fprintf(stderr, "workspace flagged failed on successful deflate\n");
        zragf_p89_host_release(dbuf);
        return 1;
    }
    if (zragf_workspace_used(&dws) == 0u) {
        fprintf(stderr, "workspace usage not tracked\n");
        zragf_p89_host_release(dbuf);
        return 1;
    }
    if (g_alloc_calls != 0u || g_realloc_calls != 0u || g_free_calls != 0u) {
        fprintf(stderr, "default allocator should not be used for wrapper deflate workspace\n");
        zragf_p89_host_release(dbuf);
        return 1;
    }

    ineed = zragf_inflate_workspace_bound_z(comp_len, windowBits);
    ibuf = zragf_p89_host_take(ineed ? ineed : 1u);
    if (!ibuf) {
        fprintf(stderr, "static inflate workspace failed\n");
        zragf_p89_host_release(dbuf);
        return 1;
    }

    memset(&istrm, 0, sizeof(istrm));
    zragf_workspace_init(&iws, ibuf, ineed);
    zragf_stream_set_workspace(&istrm, &iws);
    rc = zragf_inflateInit2(&istrm, windowBits);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateInit2 workspace failed: %s\n", zragf_strerror(rc));
        zragf_p89_host_release(ibuf);
        zragf_p89_host_release(dbuf);
        return 1;
    }

    istrm.next_in = comp;
    istrm.avail_in = (zragf_size_t)comp_len;
    istrm.next_out = out;
    istrm.avail_out = sizeof(out);

    for (;;) {
        rc = zragf_inflateZ(&istrm, ZRAGF_FINISH);
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc != ZRAGF_OK) {
            fprintf(stderr, "inflateZ failed: %s\n", zragf_strerror(rc));
            zragf_inflateEndZ(&istrm);
            zragf_p89_host_release(ibuf);
            zragf_p89_host_release(dbuf);
            return 1;
        }
        if (istrm.avail_out == 0u) {
            fprintf(stderr, "output buffer too small\n");
            zragf_inflateEndZ(&istrm);
            zragf_p89_host_release(ibuf);
            zragf_p89_host_release(dbuf);
            return 1;
        }
    }
    zragf_inflateEndZ(&istrm);

    if (zragf_workspace_failed(&iws)) {
        fprintf(stderr, "workspace flagged failed on successful inflate\n");
        zragf_p89_host_release(ibuf);
        zragf_p89_host_release(dbuf);
        return 1;
    }
    if (istrm.total_out != (zragf_u32)(sizeof(payload) - 1u) ||
        memcmp(out, payload, sizeof(payload) - 1u) != 0) {
        fprintf(stderr, "wrapper workspace roundtrip mismatch\n");
        zragf_p89_host_release(ibuf);
        zragf_p89_host_release(dbuf);
        return 1;
    }
    if (g_alloc_calls != 0u || g_realloc_calls != 0u || g_free_calls != 0u) {
        fprintf(stderr, "default allocator should not be used for wrapper inflate workspace\n");
        zragf_p89_host_release(ibuf);
        zragf_p89_host_release(dbuf);
        return 1;
    }

    zragf_p89_host_release(ibuf);
    zragf_p89_host_release(dbuf);
    return 0;
}

int main(void)
{
    zragf_allocator alloc;
    unsigned char tiny_buf[64];
    zragf_workspace tiny_ws;
    zragf_stream tiny_strm;
    int rc;

    alloc.alloc_fn = count_alloc;
    alloc.realloc_fn = count_realloc;
    alloc.free_fn = count_free;
    alloc.user = NULL;
    zragf_set_default_allocator(&alloc);

    memset(&tiny_strm, 0, sizeof(tiny_strm));
    zragf_workspace_init(&tiny_ws, tiny_buf, sizeof(tiny_buf));
    zragf_stream_set_workspace(&tiny_strm, &tiny_ws);
    rc = zragf_deflateInit2(&tiny_strm, ZRAGF_LEVEL_DEFAULT, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc == ZRAGF_OK) {
        fprintf(stderr, "tiny wrapper workspace unexpectedly succeeded\n");
        zragf_deflateEndZ(&tiny_strm);
        return 1;
    }
    if (!zragf_workspace_failed(&tiny_ws)) {
        fprintf(stderr, "tiny workspace miss not tracked\n");
        return 1;
    }
    if (g_alloc_calls != 0u || g_realloc_calls != 0u || g_free_calls != 0u) {
        fprintf(stderr, "tiny workspace path should not hit default allocator\n");
        return 1;
    }

    if (check_roundtrip(-15) != 0)
        return 1;
    if (check_roundtrip(15) != 0)
        return 1;
    if (check_roundtrip(31) != 0)
        return 1;

    zragf_set_default_allocator(NULL);

    printf("phase65 wrapper workspace ok raw/zlib/gzip\n");
    return 0;
}
