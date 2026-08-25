#include <string.h>
#include "zragf_protocol89_mem.h"

static zragf_u8 zragf_p89_arena[ZRAGF_P89_STATIC_BYTES];
static zragf_size_t zragf_p89_top = 0u;
static zragf_u32 zragf_p89_live = 0u;

static zragf_size_t zragf_p89_align(zragf_size_t value)
{
    zragf_size_t rem;
    rem = value & 7u;
    if (rem == 0u)
        return value;
    if (value > ((zragf_size_t)-1) - (8u - rem))
        return (zragf_size_t)-1;
    return value + (8u - rem);
}

void *zragf_p89_take(zragf_size_t bytes)
{
    zragf_size_t pos;
    zragf_u8 *ptr;
    if (bytes == 0u)
        return NULL;
    pos = zragf_p89_align(zragf_p89_top);
    if (pos == (zragf_size_t)-1)
        return NULL;
    if (pos > (zragf_size_t)ZRAGF_P89_STATIC_BYTES)
        return NULL;
    if (bytes > ((zragf_size_t)ZRAGF_P89_STATIC_BYTES - pos))
        return NULL;
    ptr = zragf_p89_arena + pos;
    zragf_p89_top = pos + bytes;
    zragf_p89_live += 1u;
    memset(ptr, 0, bytes);
    return ptr;
}

void *zragf_p89_resize(void *ptr, zragf_size_t old_bytes, zragf_size_t new_bytes)
{
    void *next;
    zragf_size_t copy_bytes;
    if (new_bytes == 0u) {
        zragf_p89_release(ptr);
        return NULL;
    }
    if (!ptr)
        return zragf_p89_take(new_bytes);
    next = zragf_p89_take(new_bytes);
    if (!next)
        return NULL;
    copy_bytes = old_bytes < new_bytes ? old_bytes : new_bytes;
    if (copy_bytes > 0u)
        memcpy(next, ptr, copy_bytes);
    zragf_p89_release(ptr);
    return next;
}

void zragf_p89_release(void *ptr)
{
    zragf_u8 *p;
    if (!ptr)
        return;
    p = (zragf_u8 *)ptr;
    if (p < zragf_p89_arena || p >= zragf_p89_arena + ZRAGF_P89_STATIC_BYTES)
        return;
    if (zragf_p89_live > 0u)
        zragf_p89_live -= 1u;
    if (zragf_p89_live == 0u)
        zragf_p89_top = 0u;
}

void zragf_p89_reset(void)
{
    zragf_p89_top = 0u;
    zragf_p89_live = 0u;
}

zragf_size_t zragf_p89_used(void)
{
    return zragf_p89_top;
}

zragf_size_t zragf_p89_capacity(void)
{
    return (zragf_size_t)ZRAGF_P89_STATIC_BYTES;
}
