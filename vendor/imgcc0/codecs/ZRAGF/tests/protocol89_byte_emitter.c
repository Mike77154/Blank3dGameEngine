#include <stdio.h>
#include <string.h>
#include "zragflib.h"

#define BC_SRC_MAX 8192u
#define BC_OUT_MAX 65536u
#define BC_WORK_MAX (4u * 1024u * 1024u)

static zragf_u8 bc_src[BC_SRC_MAX];
static zragf_u8 bc_out[BC_OUT_MAX];
static zragf_u8 bc_work[BC_WORK_MAX];

static void bc_put_u32(zragf_u32 v)
{
    zragf_u8 b[4];
    b[0] = (zragf_u8)(v & 255u);
    b[1] = (zragf_u8)((v >> 8) & 255u);
    b[2] = (zragf_u8)((v >> 16) & 255u);
    b[3] = (zragf_u8)((v >> 24) & 255u);
    fwrite(b, 1u, 4u, stdout);
}

static int bc_emit(const char *tag, const zragf_u8 *buf, zragf_size_t n)
{
    zragf_size_t tag_n;
    tag_n = (zragf_size_t)strlen(tag);
    bc_put_u32((zragf_u32)tag_n);
    fwrite(tag, 1u, tag_n, stdout);
    bc_put_u32((zragf_u32)n);
    if (n > 0u)
        fwrite(buf, 1u, n, stdout);
    return 1;
}

static void bc_fill(int mode, zragf_size_t *n)
{
    zragf_size_t i;
    zragf_u32 x;
    if (mode == 0) {
        static const char text[] = "ZRAGF protocol89 byte exact byte exact byte exact; abcabcabcabc; 000111222333;";
        *n = 4096u;
        for (i = 0u; i < *n; ++i)
            bc_src[i] = (zragf_u8)text[i % (sizeof(text) - 1u)];
    } else if (mode == 1) {
        *n = 8192u;
        x = 0x12345678u;
        for (i = 0u; i < *n; ++i) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            bc_src[i] = (zragf_u8)(x & 255u);
        }
    } else {
        *n = 6144u;
        for (i = 0u; i < *n; ++i) {
            if ((i % 97u) < 70u)
                bc_src[i] = 0u;
            else
                bc_src[i] = (zragf_u8)((i * 29u + (i >> 2)) & 255u);
        }
    }
}

static int bc_native(int mode, int level)
{
    zragf_size_t n;
    zragf_size_t out_n;
    char tag[32];
    zragf_status rc;
    bc_fill(mode, &n);
    out_n = BC_OUT_MAX;
    rc = zragf_compress(bc_src, n, bc_out, &out_n, (zragf_level)level);
    if (rc != ZRAGF_ST_OK)
        return 10 + mode;
    sprintf(tag, "native-m%d-l%d", mode, level);
    bc_emit(tag, bc_out, out_n);
    return 0;
}

static int bc_stream(int mode, int level, int window_bits, const char *kind)
{
    zragf_size_t n;
    zragf_stream zs;
    zragf_workspace ws;
    zragf_size_t out_n;
    char tag[40];
    int rc;
    bc_fill(mode, &n);
    memset(&zs, 0, sizeof(zs));
    memset(&ws, 0, sizeof(ws));
    zragf_workspace_init(&ws, bc_work, BC_WORK_MAX);
    zragf_stream_set_workspace(&zs, &ws);
    rc = zragf_deflateInit2(&zs, level, 8, window_bits, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK)
        return 30 + mode;
    zs.next_in = bc_src;
    zs.avail_in = n;
    zs.next_out = bc_out;
    zs.avail_out = BC_OUT_MAX;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&zs);
        return 40 + mode;
    }
    out_n = BC_OUT_MAX - zs.avail_out;
    sprintf(tag, "%s-m%d-l%d", kind, mode, level);
    bc_emit(tag, bc_out, out_n);
    zragf_deflateEndZ(&zs);
    return 0;
}

int main(void)
{
    int mode;
    int rc;
    static const int levels[3] = {1, 5, 9};
    int li;
    for (mode = 0; mode < 3; ++mode) {
        for (li = 0; li < 3; ++li) {
            rc = bc_native(mode, levels[li]);
            if (rc) return rc;
            rc = bc_stream(mode, levels[li], -15, "raw");
            if (rc) return rc;
            rc = bc_stream(mode, levels[li], 15, "zlib");
            if (rc) return rc;
            rc = bc_stream(mode, levels[li], 31, "gzip");
            if (rc) return rc;
        }
    }
    return 0;
}
