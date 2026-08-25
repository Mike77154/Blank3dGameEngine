#include <string.h>

#include "zragflib.h"
#include "zragflib_internal.h"

static zragf_size_t zragf_workspace_align(zragf_size_t value)
{
    zragf_size_t align;
    zragf_size_t rem;

    align = (zragf_size_t)sizeof(void *);
    if (align < 8u)
        align = 8u;
    if (align == 0u)
        return value;
    rem = value % align;
    if (rem == 0u)
        return value;
    if (value > ((zragf_size_t)-1) - (align - rem))
        return value;
    return value + (align - rem);
}

static int zragf_workspace_bits_from_window(int windowBits)
{
    int bits;

    bits = windowBits;
    if (bits < 0)
        bits = -bits;
    if (bits > 31)
        bits -= 32;
    if (bits > 15)
        bits -= 16;
    if (bits == 0)
        bits = 15;
    if (bits < 8)
        bits = 8;
    if (bits > 15)
        bits = 15;
    return bits;
}

static void *zragf_workspace_zalloc(void *opaque, unsigned int items, unsigned int size)
{
    zragf_workspace *workspace;
    zragf_size_t total;
    zragf_size_t pos;
    zragf_u8 *ptr;

    workspace = (zragf_workspace *)opaque;
    if (!workspace || !workspace->buffer)
        return NULL;
    if (items == 0u || size == 0u)
        return NULL;
    if ((zragf_size_t)items > (((zragf_size_t)-1) / (zragf_size_t)size)) {
        workspace->failed = 1;
        return NULL;
    }
    total = (zragf_size_t)items * (zragf_size_t)size;
    pos = zragf_workspace_align(workspace->used);
    if (pos > workspace->size || total > (workspace->size - pos)) {
        workspace->failed = 1;
        return NULL;
    }
    ptr = workspace->buffer + pos;
    workspace->used = pos + total;
    memset(ptr, 0, total);
    return ptr;
}

static void zragf_workspace_zfree(void *opaque, void *address)
{
    (void)opaque;
    (void)address;
}

void zragf_workspace_init(zragf_workspace *workspace, void *buffer, zragf_size_t size)
{
    if (!workspace)
        return;
    workspace->buffer = (zragf_u8 *)buffer;
    workspace->size = size;
    workspace->used = 0u;
    workspace->failed = 0;
    workspace->generation += 1u;
    if (workspace->generation == 0u)
        workspace->generation = 1u;
}

void zragf_workspace_reset(zragf_workspace *workspace)
{
    if (!workspace)
        return;
    workspace->used = 0u;
    workspace->failed = 0;
    workspace->generation += 1u;
    if (workspace->generation == 0u)
        workspace->generation = 1u;
}

zragf_size_t zragf_workspace_used(const zragf_workspace *workspace)
{
    if (!workspace)
        return 0u;
    return workspace->used;
}

int zragf_workspace_failed(const zragf_workspace *workspace)
{
    if (!workspace)
        return 1;
    return workspace->failed;
}

void zragf_stream_set_workspace(zragf_stream *strm, zragf_workspace *workspace)
{
    if (!strm)
        return;
    if (!workspace) {
        strm->zalloc = NULL;
        strm->zfree = NULL;
        strm->opaque = NULL;
        return;
    }
    strm->zalloc = zragf_workspace_zalloc;
    strm->zfree = zragf_workspace_zfree;
    strm->opaque = workspace;
}

zragf_size_t zragf_deflate_workspace_bound_z(unsigned long sourceLen,
                                             int windowBits,
                                             int memLevel,
                                             int strategy)
{
    zragf_size_t src_size;
    zragf_size_t blk_bound;
    zragf_size_t window_size;
    zragf_size_t total;
    int bits;

    (void)memLevel;
    (void)strategy;

    src_size = (zragf_size_t)sourceLen;
    bits = zragf_workspace_bits_from_window(windowBits);
    window_size = (zragf_size_t)1u << bits;
    blk_bound = zragf_deflate_rfc1951_stored_bound(src_size);
    if (blk_bound < src_size)
        blk_bound = src_size + 64u;

    total = (zragf_size_t)sizeof(zragf_deflate_state_z);
    total += window_size;
    total += (src_size * 2u) + 4096u;
    total += (blk_bound * 2u) + 4096u;
    total += (blk_bound * 2u) + (src_size * 2u) + 65536u;
    total += 262144u;
    return total;
}

zragf_size_t zragf_inflate_workspace_bound_z(unsigned long sourceLen, int windowBits)
{
    zragf_size_t src_size;
    zragf_size_t window_size;
    zragf_size_t total;
    int bits;

    src_size = (zragf_size_t)sourceLen;
    bits = zragf_workspace_bits_from_window(windowBits);
    window_size = (zragf_size_t)1u << bits;

    total = (zragf_size_t)sizeof(zragf_inflate_state_z);
    total += window_size;
    total += (src_size * 2u) + 4096u;
    total += 131072u;
    return total;
}
