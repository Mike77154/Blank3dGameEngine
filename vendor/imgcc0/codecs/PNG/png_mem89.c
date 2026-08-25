#include "png_mem89.h"

#define PNG_MEM89_HEADER_RAW_BYTES 8u
#define PNG_MEM89_USED 1u

typedef unsigned char png_mem89_u8;
typedef unsigned int png_mem89_u32;

/*
 * C89 has no _Alignas/max_align_t.  The arena therefore lives in a union
 * whose non-array members force alignment suitable for every object the
 * codec stores: data pointers, function pointers, and 32-bit integers.
 * The 8-byte block header is a multiple of the pointer alignment on the
 * supported Win32/ILP32 and common LP64 validation hosts.
 */
typedef union png_mem89_storage_u {
    void* data_ptr_align;
    void (*func_ptr_align)(void);
    unsigned int u32_align;
    png_mem89_u8 bytes[PNG_MEM89_ARENA_BYTES];
} png_mem89_storage;

static png_mem89_storage png_mem89_store;
#define png_mem89_arena (png_mem89_store.bytes)
static int png_mem89_ready = 0;
static png_mem89_u32 png_mem89_used_now = 0u;
static png_mem89_u32 png_mem89_peak_now = 0u;

static void png_mem89_write_u32(png_mem89_u32 off, png_mem89_u32 value)
{
    png_mem89_arena[off + 0u] = (png_mem89_u8)(value & 255u);
    png_mem89_arena[off + 1u] = (png_mem89_u8)((value >> 8) & 255u);
    png_mem89_arena[off + 2u] = (png_mem89_u8)((value >> 16) & 255u);
    png_mem89_arena[off + 3u] = (png_mem89_u8)((value >> 24) & 255u);
}

static png_mem89_u32 png_mem89_read_u32(png_mem89_u32 off)
{
    png_mem89_u32 v;
    v = (png_mem89_u32)png_mem89_arena[off + 0u];
    v |= (png_mem89_u32)png_mem89_arena[off + 1u] << 8;
    v |= (png_mem89_u32)png_mem89_arena[off + 2u] << 16;
    v |= (png_mem89_u32)png_mem89_arena[off + 3u] << 24;
    return v;
}

static png_mem89_u32 png_mem89_alignment(void)
{
    png_mem89_u32 a;
    png_mem89_u32 b;
    a = (png_mem89_u32)sizeof(void*);
    b = (png_mem89_u32)sizeof(void (*)(void));
    if (b > a)
        a = b;
    b = (png_mem89_u32)sizeof(unsigned int);
    if (b > a)
        a = b;
    if (a == 0u)
        a = 1u;
    return a;
}

static png_mem89_u32 png_mem89_align(png_mem89_u32 n)
{
    png_mem89_u32 a;
    png_mem89_u32 rem;
    a = png_mem89_alignment();
    rem = n % a;
    if (rem == 0u)
        return n;
    if (n > 0xFFFFFFFFu - (a - rem))
        return 0u;
    return n + (a - rem);
}

static png_mem89_u32 png_mem89_header_bytes(void)
{
    return png_mem89_align(PNG_MEM89_HEADER_RAW_BYTES);
}

#define PNG_MEM89_HEADER_BYTES (png_mem89_header_bytes())

static void png_mem89_boot(void)
{
    if (png_mem89_ready)
        return;
    png_mem89_reset();
}

void png_mem89_reset(void)
{
    png_mem89_u32 payload;
    png_mem89_used_now = 0u;
    png_mem89_peak_now = 0u;
    png_mem89_ready = 1;
    if (PNG_MEM89_ARENA_BYTES <= PNG_MEM89_HEADER_BYTES)
        return;
    payload = PNG_MEM89_ARENA_BYTES - PNG_MEM89_HEADER_BYTES;
    payload -= payload % png_mem89_alignment();
    png_mem89_write_u32(0u, payload);
    png_mem89_write_u32(4u, 0u);
}

static png_mem89_u32 png_mem89_find_off(void* ptr)
{
    png_mem89_u32 off;
    png_mem89_u8* p;
    if (!ptr)
        return 0xFFFFFFFFu;
    p = (png_mem89_u8*)ptr;
    if (p < png_mem89_arena + PNG_MEM89_HEADER_BYTES ||
        p >= png_mem89_arena + PNG_MEM89_ARENA_BYTES)
        return 0xFFFFFFFFu;
    off = (png_mem89_u32)(p - png_mem89_arena) - PNG_MEM89_HEADER_BYTES;
    if ((off % png_mem89_alignment()) != 0u)
        return 0xFFFFFFFFu;
    return off;
}

static png_mem89_u32 png_mem89_next_off(png_mem89_u32 off)
{
    png_mem89_u32 sz;
    png_mem89_u32 next;
    if (off + PNG_MEM89_HEADER_BYTES > PNG_MEM89_ARENA_BYTES)
        return 0xFFFFFFFFu;
    sz = png_mem89_read_u32(off);
    if (sz > PNG_MEM89_ARENA_BYTES - off - PNG_MEM89_HEADER_BYTES)
        return 0xFFFFFFFFu;
    next = off + PNG_MEM89_HEADER_BYTES + sz;
    if (next >= PNG_MEM89_ARENA_BYTES)
        return 0xFFFFFFFFu;
    return next;
}

static void png_mem89_copy_bytes(png_mem89_u8* dst, const png_mem89_u8* src, png_mem89_u32 n)
{
    png_mem89_u32 i;
    if (dst == src || n == 0u)
        return;
    if (dst < src)
    {
        for (i = 0u; i < n; ++i)
            dst[i] = src[i];
    }
    else
    {
        i = n;
        while (i != 0u)
        {
            --i;
            dst[i] = src[i];
        }
    }
}

void* png_mem89_alloc(unsigned int bytes)
{
    png_mem89_u32 want;
    png_mem89_u32 off;
    png_mem89_u32 sz;
    png_mem89_u32 flags;
    png_mem89_u32 remain;
    png_mem89_boot();
    if (bytes == 0u)
        bytes = 1u;
    want = png_mem89_align((png_mem89_u32)bytes);
    if (want == 0u)
        return 0;
    off = 0u;
    while (off != 0xFFFFFFFFu && off + PNG_MEM89_HEADER_BYTES <= PNG_MEM89_ARENA_BYTES)
    {
        sz = png_mem89_read_u32(off);
        flags = png_mem89_read_u32(off + 4u);
        if ((flags & PNG_MEM89_USED) == 0u && sz >= want)
        {
            remain = sz - want;
            if (remain >= PNG_MEM89_HEADER_BYTES + png_mem89_alignment())
            {
                png_mem89_u32 split_off;
                png_mem89_u32 split_sz;
                split_off = off + PNG_MEM89_HEADER_BYTES + want;
                split_sz = remain - PNG_MEM89_HEADER_BYTES;
                png_mem89_write_u32(split_off, split_sz);
                png_mem89_write_u32(split_off + 4u, 0u);
                png_mem89_write_u32(off, want);
                sz = want;
            }
            png_mem89_write_u32(off + 4u, PNG_MEM89_USED);
            png_mem89_used_now += sz;
            if (png_mem89_used_now > png_mem89_peak_now)
                png_mem89_peak_now = png_mem89_used_now;
            return (void*)(png_mem89_arena + off + PNG_MEM89_HEADER_BYTES);
        }
        off = png_mem89_next_off(off);
    }
    return 0;
}

void* png_mem89_alloc_zero(unsigned int count, unsigned int bytes_each)
{
    png_mem89_u32 total;
    png_mem89_u32 i;
    png_mem89_u8* p;
    if (count != 0u && bytes_each > 0xFFFFFFFFu / count)
        return 0;
    total = count * bytes_each;
    p = (png_mem89_u8*)png_mem89_alloc(total);
    if (!p)
        return 0;
    for (i = 0u; i < total; ++i)
        p[i] = 0u;
    return p;
}

void png_mem89_release(void* ptr)
{
    png_mem89_u32 off;
    png_mem89_u32 sz;
    png_mem89_u32 next;
    png_mem89_u32 prev;
    png_mem89_u32 scan;
    png_mem89_boot();
    if (!ptr)
        return;
    off = png_mem89_find_off(ptr);
    if (off == 0xFFFFFFFFu)
        return;
    if ((png_mem89_read_u32(off + 4u) & PNG_MEM89_USED) == 0u)
        return;
    sz = png_mem89_read_u32(off);
    if (png_mem89_used_now >= sz)
        png_mem89_used_now -= sz;
    else
        png_mem89_used_now = 0u;
    png_mem89_write_u32(off + 4u, 0u);

    next = png_mem89_next_off(off);
    if (next != 0xFFFFFFFFu && (png_mem89_read_u32(next + 4u) & PNG_MEM89_USED) == 0u)
    {
        png_mem89_u32 nsz;
        nsz = png_mem89_read_u32(next);
        if (sz <= 0xFFFFFFFFu - PNG_MEM89_HEADER_BYTES - nsz)
        {
            sz += PNG_MEM89_HEADER_BYTES + nsz;
            png_mem89_write_u32(off, sz);
        }
    }

    prev = 0xFFFFFFFFu;
    scan = 0u;
    while (scan != 0xFFFFFFFFu && scan < off)
    {
        png_mem89_u32 n;
        n = png_mem89_next_off(scan);
        if (n == off)
        {
            prev = scan;
            break;
        }
        if (n == 0xFFFFFFFFu || n <= scan)
            break;
        scan = n;
    }
    if (prev != 0xFFFFFFFFu && (png_mem89_read_u32(prev + 4u) & PNG_MEM89_USED) == 0u)
    {
        png_mem89_u32 psz;
        psz = png_mem89_read_u32(prev);
        sz = png_mem89_read_u32(off);
        if (psz <= 0xFFFFFFFFu - PNG_MEM89_HEADER_BYTES - sz)
            png_mem89_write_u32(prev, psz + PNG_MEM89_HEADER_BYTES + sz);
    }
}

void* png_mem89_resize(void* ptr, unsigned int new_bytes)
{
    png_mem89_u32 off;
    png_mem89_u32 old_sz;
    png_mem89_u32 want;
    png_mem89_u32 next;
    void* out;
    png_mem89_u32 copy_n;
    if (!ptr)
        return png_mem89_alloc(new_bytes);
    if (new_bytes == 0u)
    {
        png_mem89_release(ptr);
        return 0;
    }
    png_mem89_boot();
    off = png_mem89_find_off(ptr);
    if (off == 0xFFFFFFFFu)
        return 0;
    old_sz = png_mem89_read_u32(off);
    want = png_mem89_align((png_mem89_u32)new_bytes);
    if (want == 0u)
        return 0;
    if (want <= old_sz)
        return ptr;

    next = png_mem89_next_off(off);
    if (next != 0xFFFFFFFFu && (png_mem89_read_u32(next + 4u) & PNG_MEM89_USED) == 0u)
    {
        png_mem89_u32 nsz;
        png_mem89_u32 combined;
        nsz = png_mem89_read_u32(next);
        if (old_sz <= 0xFFFFFFFFu - PNG_MEM89_HEADER_BYTES - nsz)
        {
            combined = old_sz + PNG_MEM89_HEADER_BYTES + nsz;
            if (combined >= want)
            {
                png_mem89_u32 assigned;
                png_mem89_u32 extra;
                png_mem89_u32 remain;
                assigned = combined;
                remain = combined - want;
                if (remain >= PNG_MEM89_HEADER_BYTES + png_mem89_alignment())
                {
                    png_mem89_u32 split_off;
                    png_mem89_u32 split_sz;
                    assigned = want;
                    split_off = off + PNG_MEM89_HEADER_BYTES + assigned;
                    split_sz = remain - PNG_MEM89_HEADER_BYTES;
                    png_mem89_write_u32(split_off, split_sz);
                    png_mem89_write_u32(split_off + 4u, 0u);
                }
                png_mem89_write_u32(off, assigned);
                extra = assigned - old_sz;
                if (png_mem89_used_now <= 0xFFFFFFFFu - extra)
                    png_mem89_used_now += extra;
                if (png_mem89_used_now > png_mem89_peak_now)
                    png_mem89_peak_now = png_mem89_used_now;
                return ptr;
            }
        }
    }

    out = png_mem89_alloc(want);
    if (!out)
        return 0;
    copy_n = old_sz < want ? old_sz : want;
    png_mem89_copy_bytes((png_mem89_u8*)out, (const png_mem89_u8*)ptr, copy_n);
    png_mem89_release(ptr);
    return out;
}

unsigned int png_mem89_capacity(void)
{
    return (unsigned int)PNG_MEM89_ARENA_BYTES;
}

unsigned int png_mem89_bytes_used(void)
{
    return (unsigned int)png_mem89_used_now;
}

unsigned int png_mem89_peak_used(void)
{
    return (unsigned int)png_mem89_peak_now;
}
