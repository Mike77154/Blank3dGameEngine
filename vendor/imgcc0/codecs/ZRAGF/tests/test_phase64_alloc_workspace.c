#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

static unsigned g_alloc_calls = 0u;
static unsigned g_free_calls = 0u;
static unsigned g_realloc_calls = 0u;

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

static int check_equal(const unsigned char *a, const unsigned char *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

int main(void)
{
    static const unsigned char payload[] =
        "phase64 payload :: allocator + workspace + zlib-like helpers :: "
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    unsigned char native_comp[2048];
    unsigned char native_out[2048];
    unsigned char zbuf[2048];
    unsigned char zout[2048];
    zragf_info info;
    zragf_allocator alloc;
    zragf_size_t comp_size;
    zragf_size_t out_size;
    zragf_size_t workspace_need;
    zragf_size_t decomp_workspace_need;
    void *workspace;
    void *dworkspace;
    unsigned long zcap;
    unsigned long zcomp_len;
    unsigned long zout_len;
    zragf_format fmt;
    int rc;

    memset(&info, 0, sizeof(info));
    alloc.alloc_fn = count_alloc;
    alloc.realloc_fn = count_realloc;
    alloc.free_fn = count_free;
    alloc.user = NULL;
    zragf_set_default_allocator(&alloc);

    comp_size = sizeof(native_comp);
    rc = zragf_compress(payload, sizeof(payload) - 1u, native_comp, &comp_size, ZRAGF_LEVEL_DEFAULT);
    if (rc != ZRAGF_ST_OK) {
        fprintf(stderr, "zragf_compress failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    if (g_alloc_calls == 0u) {
        fprintf(stderr, "expected default allocator usage in native compress\n");
        return 1;
    }

    out_size = sizeof(native_out);
    rc = zragf_decompress(native_comp, comp_size, native_out, &out_size, &info);
    if (rc != ZRAGF_ST_OK) {
        fprintf(stderr, "zragf_decompress failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    if (!check_equal(payload, native_out, sizeof(payload) - 1u)) {
        fprintf(stderr, "native roundtrip mismatch\n");
        return 1;
    }

    g_alloc_calls = 0u;
    g_realloc_calls = 0u;
    g_free_calls = 0u;

    workspace_need = zragf_compress_workspace_bound(sizeof(payload) - 1u);
    if (workspace_need == 0u) {
        fprintf(stderr, "compress workspace bound invalid\n");
        return 1;
    }
    workspace = zragf_p89_host_take(workspace_need);
    if (!workspace) {
        fprintf(stderr, "static workspace failed\n");
        return 1;
    }
    comp_size = sizeof(native_comp);
    rc = zragf_compress_with_workspace(payload, sizeof(payload) - 1u,
                                       native_comp, &comp_size,
                                       workspace, workspace_need,
                                       ZRAGF_LEVEL_DEFAULT);
    zragf_p89_host_release(workspace);
    if (rc != ZRAGF_ST_OK) {
        fprintf(stderr, "zragf_compress_with_workspace failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    if (g_alloc_calls != 0u || g_realloc_calls != 0u || g_free_calls != 0u) {
        fprintf(stderr, "workspace compress should avoid default allocator\n");
        return 1;
    }

    rc = zragf_decompress_workspace_bound(native_comp, comp_size, &decomp_workspace_need);
    if (rc != ZRAGF_ST_OK) {
        fprintf(stderr, "decompress workspace bound failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    dworkspace = decomp_workspace_need ? zragf_p89_host_take(decomp_workspace_need) : NULL;
    if (decomp_workspace_need != 0u && !dworkspace) {
        fprintf(stderr, "static dworkspace failed\n");
        return 1;
    }
    out_size = sizeof(native_out);
    rc = zragf_decompress_with_workspace(native_comp, comp_size,
                                         native_out, &out_size,
                                         dworkspace, decomp_workspace_need,
                                         &info);
    zragf_p89_host_release(dworkspace);
    if (rc != ZRAGF_ST_OK) {
        fprintf(stderr, "zragf_decompress_with_workspace failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    if (!check_equal(payload, native_out, sizeof(payload) - 1u)) {
        fprintf(stderr, "workspace roundtrip mismatch\n");
        return 1;
    }
    if (g_alloc_calls != 0u || g_realloc_calls != 0u || g_free_calls != 0u) {
        fprintf(stderr, "workspace decompress should avoid default allocator\n");
        return 1;
    }

    zragf_set_default_allocator(NULL);

    if (zragf_build_config()[0] == '\0') {
        fprintf(stderr, "build config empty\n");
        return 1;
    }
    if (zragf_strerror(ZRAGF_ST_WORKSPACE_TOO_SMALL)[0] == '\0') {
        fprintf(stderr, "strerror empty\n");
        return 1;
    }

    zcap = zragf_compressBound((unsigned long)(sizeof(payload) - 1u));
    if (zcap > sizeof(zbuf)) {
        fprintf(stderr, "zbuf too small for test compressBound\n");
        return 1;
    }
    zcomp_len = zcap;
    rc = zragf_compress2(zbuf, &zcomp_len, payload, (unsigned long)(sizeof(payload) - 1u), ZRAGF_LEVEL_DEFAULT);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "zragf_compress2 failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    rc = zragf_inspect_wrapper(zbuf, (zragf_size_t)zcomp_len, &fmt);
    if (rc != ZRAGF_ST_OK || fmt != ZRAGF_FMT_ZLIB_WRAPPED) {
        fprintf(stderr, "inspect wrapper expected zlib\n");
        return 1;
    }
    zout_len = sizeof(zout);
    rc = zragf_uncompress(zout, &zout_len, zbuf, zcomp_len);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "zragf_uncompress failed: %s\n", zragf_strerror(rc));
        return 1;
    }
    if (zout_len != (unsigned long)(sizeof(payload) - 1u) || !check_equal(payload, zout, sizeof(payload) - 1u)) {
        fprintf(stderr, "zlib-like helper roundtrip mismatch\n");
        return 1;
    }

    printf("phase64 alloc/workspace/helpers ok native=%lu zlib=%lu\n",
           (unsigned long)comp_size, zcomp_len);
    return 0;
}
