#include "zragflib_internal.h"

static zragf_allocator zragf_g_default_allocator = { NULL, NULL, NULL, NULL };

void zragf_set_default_allocator(const zragf_allocator *allocator)
{
    if (!allocator) {
        zragf_g_default_allocator.alloc_fn = NULL;
        zragf_g_default_allocator.realloc_fn = NULL;
        zragf_g_default_allocator.free_fn = NULL;
        zragf_g_default_allocator.user = NULL;
        return;
    }
    zragf_g_default_allocator = *allocator;
}

void zragf_get_default_allocator(zragf_allocator *allocator)
{
    if (!allocator)
        return;
    *allocator = zragf_g_default_allocator;
}

void zragf_write_u32_le(zragf_u8 *p, zragf_u32 v)
{
    p[0] = (zragf_u8)(v & 0xFFu);
    p[1] = (zragf_u8)((v >> 8) & 0xFFu);
    p[2] = (zragf_u8)((v >> 16) & 0xFFu);
    p[3] = (zragf_u8)((v >> 24) & 0xFFu);
}

zragf_u32 zragf_read_u32_le(const zragf_u8 *p)
{
    return (zragf_u32)p[0]
        | ((zragf_u32)p[1] << 8)
        | ((zragf_u32)p[2] << 16)
        | ((zragf_u32)p[3] << 24);
}

zragf_internal_format zragf_select_format(int windowBits)
{
    if (windowBits > 15)
        return ZRAGF_INTERNAL_FMT_GZIP;
    if (windowBits > 0)
        return ZRAGF_INTERNAL_FMT_ZLIB;
    if (windowBits < 0)
        return ZRAGF_INTERNAL_FMT_DEFLATE_RAW;
    return ZRAGF_INTERNAL_FMT_ZRAGF;
}

void *zragf_alloc_default(void *opaque, zragf_size_t items, zragf_size_t size)
{
    zragf_size_t total;
    (void)opaque;
    if (items == 0 || size == 0)
        return NULL;
    if (items > (((zragf_size_t)-1) / size))
        return NULL;
    total = items * size;
    if (zragf_g_default_allocator.alloc_fn)
        return zragf_g_default_allocator.alloc_fn(zragf_g_default_allocator.user, total);
    return zragf_p89_take(total);
}

void *zragf_realloc_default(void *opaque, void *ptr, zragf_size_t old_size, zragf_size_t new_size)
{
    void *np;
    (void)opaque;
    if (new_size == 0u) {
        zragf_free_default(NULL, ptr);
        return NULL;
    }
    if (zragf_g_default_allocator.realloc_fn)
        return zragf_g_default_allocator.realloc_fn(zragf_g_default_allocator.user, ptr, old_size, new_size);
    if (!zragf_g_default_allocator.alloc_fn && !zragf_g_default_allocator.free_fn)
        return zragf_p89_resize(ptr, old_size, new_size);
    np = zragf_alloc_default(NULL, 1u, new_size);
    if (!np)
        return NULL;
    if (ptr && old_size > 0u)
        memcpy(np, ptr, (old_size < new_size) ? old_size : new_size);
    zragf_free_default(NULL, ptr);
    return np;
}

void zragf_free_default(void *opaque, void *addr)
{
    (void)opaque;
    if (!addr)
        return;
    if (zragf_g_default_allocator.free_fn) {
        zragf_g_default_allocator.free_fn(zragf_g_default_allocator.user, addr);
        return;
    }
    zragf_p89_release(addr);
}

void zragf_bw_init_state(zragf_deflate_state_z *st)
{
    st->bitbuf   = 0;
    st->bitcount = 0;
}

void zragf_bw_putbits(zragf_deflate_state_z *st, zragf_u32 code, int bits)
{
    while (bits-- > 0) {
        st->bitbuf |= (code & 1u) << st->bitcount;
        st->bitcount++;
        code >>= 1u;

        if (st->bitcount == 8) {
            if (st->blk_size < st->blk_cap)
                st->blk_out[st->blk_size++] = (zragf_u8)st->bitbuf;
            st->bitbuf = 0;
            st->bitcount = 0;
        }
    }
}

void zragf_bw_flushbits(zragf_deflate_state_z *st)
{
    if (st->bitcount > 0) {
        if (st->blk_size < st->blk_cap)
            st->blk_out[st->blk_size++] = (zragf_u8)st->bitbuf;
        st->bitbuf = 0;
        st->bitcount = 0;
    }
}

int zragf_br_getbit(zragf_inflate_state_z *st, int *out)
{
    if (st->bitcount == 0) {
        if (st->in_size == 0)
            return 0;
        st->bitbuf = st->in_buf[0];
        if (st->in_size > 1)
            memmove(st->in_buf, st->in_buf + 1, st->in_size - 1);
        st->in_size--;
        st->bitcount = 8;
    }

    *out = (int)(st->bitbuf & 1u);
    st->bitbuf >>= 1u;
    st->bitcount--;
    return 1;
}
