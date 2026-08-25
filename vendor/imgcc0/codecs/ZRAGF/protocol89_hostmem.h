#ifndef ZRAGF_PROTOCOL89_HOSTMEM_H_INCLUDED
#define ZRAGF_PROTOCOL89_HOSTMEM_H_INCLUDED

#include <string.h>
#include "zragflib.h"

#ifndef ZRAGF_P89_HOST_SLOTS
#define ZRAGF_P89_HOST_SLOTS 12u
#endif
#ifndef ZRAGF_P89_HOST_SLOT_BYTES
#define ZRAGF_P89_HOST_SLOT_BYTES (32u * 1024u * 1024u)
#endif

static zragf_u8 zragf_p89_host_mem[ZRAGF_P89_HOST_SLOTS][ZRAGF_P89_HOST_SLOT_BYTES];
static zragf_size_t zragf_p89_host_sizes[ZRAGF_P89_HOST_SLOTS];
static zragf_u8 zragf_p89_host_busy[ZRAGF_P89_HOST_SLOTS];

static void *zragf_p89_host_take(zragf_size_t bytes)
{
    zragf_u32 i;
    if (bytes == 0u)
        bytes = 1u;
    if (bytes > (zragf_size_t)ZRAGF_P89_HOST_SLOT_BYTES)
        return NULL;
    for (i = 0u; i < (zragf_u32)ZRAGF_P89_HOST_SLOTS; ++i) {
        if (!zragf_p89_host_busy[i]) {
            zragf_p89_host_busy[i] = 1u;
            zragf_p89_host_sizes[i] = bytes;
            memset(zragf_p89_host_mem[i], 0, bytes);
            return zragf_p89_host_mem[i];
        }
    }
    return NULL;
}

static void *zragf_p89_host_take_zero(zragf_size_t items, zragf_size_t bytes_each)
{
    if (items == 0u || bytes_each == 0u)
        return zragf_p89_host_take(1u);
    if (items > ((zragf_size_t)-1) / bytes_each)
        return NULL;
    return zragf_p89_host_take(items * bytes_each);
}

static zragf_size_t zragf_p89_host_size_of(void *ptr)
{
    zragf_u32 i;
    for (i = 0u; i < (zragf_u32)ZRAGF_P89_HOST_SLOTS; ++i) {
        if (zragf_p89_host_busy[i] && ptr == (void *)zragf_p89_host_mem[i])
            return zragf_p89_host_sizes[i];
    }
    return 0u;
}

static void zragf_p89_host_release(void *ptr)
{
    zragf_u32 i;
    if (!ptr)
        return;
    for (i = 0u; i < (zragf_u32)ZRAGF_P89_HOST_SLOTS; ++i) {
        if (zragf_p89_host_busy[i] && ptr == (void *)zragf_p89_host_mem[i]) {
            zragf_p89_host_busy[i] = 0u;
            zragf_p89_host_sizes[i] = 0u;
            return;
        }
    }
}

static void *zragf_p89_host_resize(void *ptr, zragf_size_t new_bytes)
{
    void *next;
    zragf_size_t old_bytes;
    zragf_size_t copy_bytes;
    if (!ptr)
        return zragf_p89_host_take(new_bytes);
    if (new_bytes == 0u) {
        zragf_p89_host_release(ptr);
        return NULL;
    }
    old_bytes = zragf_p89_host_size_of(ptr);
    next = zragf_p89_host_take(new_bytes);
    if (!next)
        return NULL;
    copy_bytes = old_bytes < new_bytes ? old_bytes : new_bytes;
    if (copy_bytes > 0u)
        memcpy(next, ptr, copy_bytes);
    zragf_p89_host_release(ptr);
    return next;
}

#endif
