#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "zragflib.h"
#include "zragflib_internal.h"
#include "inflate_stream.h"
#include "inflate_huffman.h"
#include "inflate_shared.h"
#include "zragf_crc32.h"

/* -------------------------------------------------------------
   Estados nativos (buffered) con tag explícito
   ------------------------------------------------------------- */
typedef struct {
    zragf_state_kind kind;
    zragf_u8    *in_buf;
    zragf_size_t in_size;
    zragf_size_t in_cap;
    zragf_level  level;
} zragf_native_deflate_state;

typedef struct {
    zragf_state_kind kind;
    zragf_u8    *in_buf;
    zragf_size_t in_size;
    zragf_size_t in_cap;
} zragf_native_inflate_state;

typedef enum {
    ZRAGF_INF_RUN_STEP_OK = 0,
    ZRAGF_INF_RUN_STEP_NEED_INPUT,
    ZRAGF_INF_RUN_STEP_NEED_OUTPUT,
    ZRAGF_INF_RUN_STEP_NEED_DICT,
    ZRAGF_INF_RUN_STEP_STREAM_END,
    ZRAGF_INF_RUN_STEP_ERROR
} zragf_inf_run_step;

typedef enum {
    ZRAGF_INF_RUN_STATE_HEADER = 0,
    ZRAGF_INF_RUN_STATE_BLOCK_HEADER,
    ZRAGF_INF_RUN_STATE_STORED_COPY,
    ZRAGF_INF_RUN_STATE_COMPRESSED,
    ZRAGF_INF_RUN_STATE_MATCH_COPY,
    ZRAGF_INF_RUN_STATE_TRAILER,
    ZRAGF_INF_RUN_STATE_DONE,
    ZRAGF_INF_RUN_STATE_ERROR
} zragf_inf_run_state;

typedef struct {
    zragf_size_t in_pos;
    zragf_u32 bitbuf;
    int bitcount;
    zragf_u32 total_in_consumed;
} zragf_inf_run_mark;

typedef struct {
    zragf_inflate_state_z base;
    zragf_inf_run_state run_state;

    zragf_size_t in_pos;
    zragf_u32 total_in_consumed;

    zragf_u32 running_adler;
    zragf_u32 running_crc;
    zragf_u32 output_isize;

    int block_final;
    int copy_distance;
    int copy_remaining;
    int dist_table_dummy;

    zragf_inflate_huff huff;
} zragf_inflate_state_run;


static void *zragf_default_zalloc(void *opaque, unsigned int items, unsigned int size)
{
    size_t total;
    (void)opaque;
    if (items == 0u || size == 0u)
        return NULL;
    total = (size_t)items * (size_t)size;
    if (items != 0u && total / (size_t)items != (size_t)size)
        return NULL;
    return zragf_alloc_default(opaque, (zragf_size_t)items, (zragf_size_t)size);
}

static void zragf_default_zfree(void *opaque, void *address)
{
    (void)opaque;
    zragf_free_default(opaque, address);
}

static void zragf_stream_prepare_allocators(zragf_stream *strm)
{
    if (!strm)
        return;
    if (!strm->zalloc)
        strm->zalloc = zragf_default_zalloc;
    if (!strm->zfree)
        strm->zfree = zragf_default_zfree;
}

static void *zragf_stream_bridge_alloc(void *user, zragf_size_t size)
{
    zragf_stream *strm;

    strm = (zragf_stream *)user;
    if (!strm || !strm->zalloc)
        return NULL;
    if (size > (zragf_size_t)UINT_MAX)
        return NULL;
    return strm->zalloc(strm->opaque, 1u, (unsigned int)size);
}

static void *zragf_stream_bridge_realloc(void *user, void *ptr, zragf_size_t old_size, zragf_size_t new_size)
{
    void *np;
    zragf_stream *strm;

    strm = (zragf_stream *)user;
    if (new_size == 0u) {
        if (strm && strm->zfree && ptr)
            strm->zfree(strm->opaque, ptr);
        return NULL;
    }
    np = zragf_stream_bridge_alloc(user, new_size);
    if (!np)
        return NULL;
    if (ptr && old_size > 0u)
        memcpy(np, ptr, (old_size < new_size) ? old_size : new_size);
    if (strm && strm->zfree && ptr)
        strm->zfree(strm->opaque, ptr);
    return np;
}

static void zragf_stream_bridge_free(void *user, void *ptr)
{
    zragf_stream *strm;

    strm = (zragf_stream *)user;
    if (strm && strm->zfree && ptr)
        strm->zfree(strm->opaque, ptr);
}

static void zragf_stream_allocator_scope_begin(zragf_stream *strm, zragf_allocator *saved, int *active)
{
    zragf_allocator tmp;

    if (active)
        *active = 0;
    if (!strm || !saved || !active)
        return;
    if (!strm->zalloc)
        return;
    if (strm->zalloc == zragf_default_zalloc && strm->zfree == zragf_default_zfree)
        return;

    zragf_get_default_allocator(saved);
    tmp.alloc_fn = zragf_stream_bridge_alloc;
    tmp.realloc_fn = zragf_stream_bridge_realloc;
    tmp.free_fn = zragf_stream_bridge_free;
    tmp.user = strm;
    zragf_set_default_allocator(&tmp);
    *active = 1;
}

static void zragf_stream_allocator_scope_end(const zragf_allocator *saved, int active)
{
    if (!saved || !active)
        return;
    zragf_set_default_allocator(saved);
}

static int zragf_stream_inflate_build_table(zragf_stream *strm,
                                            const int *lengths,
                                            int nlen,
                                            int table_bits,
                                            zragf_idec_table *out_tab)
{
    zragf_allocator saved;
    int active;
    int ok;

    zragf_stream_allocator_scope_begin(strm, &saved, &active);
    ok = zragf_inflate_build_table(lengths, nlen, table_bits, out_tab);
    zragf_stream_allocator_scope_end(&saved, active);
    return ok;
}

static int zragf_stream_inflate_init_fixed(zragf_stream *strm, zragf_inflate_huff *h)
{
    zragf_allocator saved;
    int active;
    int ok;

    zragf_stream_allocator_scope_begin(strm, &saved, &active);
    ok = zragf_inflate_init_fixed(h);
    zragf_stream_allocator_scope_end(&saved, active);
    return ok;
}

static void zragf_stream_inflate_free_huff(zragf_stream *strm, zragf_inflate_huff *h)
{
    zragf_allocator saved;
    int active;

    zragf_stream_allocator_scope_begin(strm, &saved, &active);
    zragf_inflate_free(h);
    zragf_stream_allocator_scope_end(&saved, active);
}

static void zragf_stream_reset_public_fields(zragf_stream *strm)
{
    if (!strm)
        return;
    strm->total_in = 0u;
    strm->total_out = 0u;
    strm->msg = NULL;
    strm->data_type = ZRAGF_Z_UNKNOWN;
    strm->adler = 1u;
    strm->reserved = 0u;
}

static void zragf_stream_set_checksum(zragf_stream *strm,
                                      zragf_internal_format fmt,
                                      zragf_u32 running_adler,
                                      zragf_u32 running_crc)
{
    if (!strm)
        return;
    strm->adler = (fmt == ZRAGF_INTERNAL_FMT_GZIP) ? running_crc : running_adler;
}

static int zragf_level_valid(int level)
{
    return level == -1 || (level >= 0 && level <= 9);
}

static int zragf_memlevel_valid(int memLevel)
{
    return memLevel >= 1 && memLevel <= 9;
}

static int zragf_strategy_valid(int strategy)
{
    switch (strategy) {
        case ZRAGF_Z_DEFAULT_STRATEGY:
        case ZRAGF_Z_FILTERED:
        case ZRAGF_Z_HUFFMAN_ONLY:
        case ZRAGF_Z_RLE:
        case ZRAGF_Z_FIXED:
            return 1;
        default:
            return 0;
    }
}

static int zragf_deflate_window_bits_valid(int windowBits)
{
    return ((windowBits >= 8 && windowBits <= 15)
         || (windowBits <= -8 && windowBits >= -15)
         || (windowBits >= 24 && windowBits <= 31));
}

static int zragf_inflate_window_bits_parse(int windowBits,
                                           zragf_internal_format *fmt,
                                           int *max_window_bits)
{
    if (!fmt || !max_window_bits)
        return 0;

    if (windowBits == 0) {
        *fmt = ZRAGF_INTERNAL_FMT_ZLIB;
        *max_window_bits = 0;
        return 1;
    }
    if (windowBits <= -8 && windowBits >= -15) {
        *fmt = ZRAGF_INTERNAL_FMT_DEFLATE_RAW;
        *max_window_bits = -windowBits;
        return 1;
    }
    if (windowBits >= 8 && windowBits <= 15) {
        *fmt = ZRAGF_INTERNAL_FMT_ZLIB;
        *max_window_bits = windowBits;
        return 1;
    }
    if (windowBits >= 24 && windowBits <= 31) {
        *fmt = ZRAGF_INTERNAL_FMT_GZIP;
        *max_window_bits = windowBits - 16;
        return 1;
    }
    if (windowBits >= 40 && windowBits <= 47) {
        *fmt = ZRAGF_INTERNAL_FMT_AUTO;
        *max_window_bits = windowBits - 32;
        return 1;
    }
    return 0;
}

/* -------------------------------------------------------------
   Alloc helpers con soporte para callbacks estilo zlib
   ------------------------------------------------------------- */
static void *zragf_stream_alloc_bytes(zragf_stream *strm, zragf_size_t size)
{
    if (size == 0u)
        return NULL;

    if (strm && strm->zalloc) {
        if (size > (zragf_size_t)UINT_MAX)
            return NULL;
        return strm->zalloc(strm->opaque, 1u, (unsigned int)size);
    }
    return zragf_alloc_default(NULL, 1u, size);
}

static void zragf_stream_free_bytes(zragf_stream *strm, void *ptr)
{
    if (!ptr)
        return;

    if (strm && strm->zfree) {
        strm->zfree(strm->opaque, ptr);
        return;
    }
    zragf_free_default(NULL, ptr);
}

static void *zragf_stream_realloc_bytes(zragf_stream *strm,
                                        void *ptr,
                                        zragf_size_t old_size,
                                        zragf_size_t new_size)
{
    void *np;

    if (new_size == 0u) {
        zragf_stream_free_bytes(strm, ptr);
        return NULL;
    }

    if ((!strm || !strm->zalloc) && (!strm || !strm->zfree))
        return zragf_realloc_default(NULL, ptr, old_size, new_size);

    np = zragf_stream_alloc_bytes(strm, new_size);
    if (!np)
        return NULL;

    if (ptr && old_size > 0u)
        memcpy(np, ptr, (old_size < new_size) ? old_size : new_size);

    zragf_stream_free_bytes(strm, ptr);
    return np;
}

static int zragf_stream_clone_buffer(zragf_stream *strm,
                                     zragf_u8 **dst,
                                     const zragf_u8 *src,
                                     zragf_size_t alloc_size,
                                     zragf_size_t copy_size)
{
    zragf_u8 *buf;

    if (!dst)
        return 0;
    *dst = NULL;
    if (alloc_size == 0u)
        return 1;
    buf = (zragf_u8 *)zragf_stream_alloc_bytes(strm, alloc_size);
    if (!buf)
        return 0;
    if (src && copy_size > 0u)
        memcpy(buf, src, copy_size);
    *dst = buf;
    return 1;
}

static int zragf_stream_clone_str0(zragf_stream *strm,
                                   zragf_u8 **dst,
                                   const zragf_u8 *src,
                                   zragf_size_t len)
{
    zragf_u8 *buf;

    if (!dst)
        return 0;
    *dst = NULL;
    if (!src)
        return 1;
    buf = (zragf_u8 *)zragf_stream_alloc_bytes(strm, len + 1u);
    if (!buf)
        return 0;
    if (len > 0u)
        memcpy(buf, src, len);
    buf[len] = 0u;
    *dst = buf;
    return 1;
}

static int zragf_clone_idec_table(zragf_stream *strm,
                                  zragf_idec_table *dst,
                                  const zragf_idec_table *src)
{
    if (!dst || !src)
        return 0;
    memset(dst, 0, sizeof(*dst));
    dst->table_bits = src->table_bits;
    dst->count = src->count;
    dst->max_bits = src->max_bits;
    if (!src->table || src->count <= 0)
        return 1;
    dst->table = (zragf_idec_entry *)zragf_stream_alloc_bytes(strm,
                                                              (zragf_size_t)src->count * sizeof(zragf_idec_entry));
    if (!dst->table)
        return 0;
    memcpy(dst->table, src->table, (size_t)src->count * sizeof(zragf_idec_entry));
    return 1;
}

static zragf_status zragf_append_input(zragf_stream *strm,
                                       zragf_u8 **buf,
                                       zragf_size_t *size,
                                       zragf_size_t *cap,
                                       const zragf_u8 *src,
                                       zragf_size_t len)
{
    zragf_size_t need;
    zragf_size_t newcap;
    zragf_u8 *nbuf;

    if (!buf || !size || !cap)
        return ZRAGF_ST_NULL_POINTER;
    if (len == 0u)
        return ZRAGF_ST_OK;
    if (!src)
        return ZRAGF_ST_NULL_POINTER;
    if (*size > (((zragf_size_t)-1) - len))
        return ZRAGF_ST_INTERNAL;

    need = *size + len;
    if (need > *cap) {
        newcap = (*cap == 0u) ? 4096u : *cap;
        while (newcap < need) {
            if (newcap > (((zragf_size_t)-1) / 2u)) {
                newcap = need;
                break;
            }
            newcap *= 2u;
        }

        nbuf = (zragf_u8 *)zragf_stream_realloc_bytes(strm, *buf, *cap, newcap);
        if (!nbuf)
            return ZRAGF_ST_INTERNAL;

        *buf = nbuf;
        *cap = newcap;
    }

    memcpy(*buf + *size, src, len);
    *size += len;
    return ZRAGF_ST_OK;
}

/* -------------------------------------------------------------
   Checksums incrementales
   ------------------------------------------------------------- */
static zragf_u32 zragf_adler32_update(zragf_u32 adler,
                                      const zragf_u8 *buf,
                                      zragf_size_t size)
{
    const zragf_u32 mod_adler = 65521u;
    zragf_u32 a = adler & 0xFFFFu;
    zragf_u32 b = (adler >> 16) & 0xFFFFu;
    zragf_size_t i;

    if (!buf || size == 0u)
        return adler;

    for (i = 0u; i < size; ++i) {
        a += (zragf_u32)buf[i];
        if (a >= mod_adler)
            a -= mod_adler;
        b += a;
        if (b >= mod_adler)
            b %= mod_adler;
    }
    return (b << 16) | a;
}

static zragf_u32 zragf_crc32_update_running(zragf_u32 crc,
                                            const zragf_u8 *buf,
                                            zragf_size_t size)
{
    zragf_size_t i;
    zragf_size_t j;

    crc = ~crc;
    if (!buf || size == 0u)
        return ~crc;

    for (i = 0u; i < size; ++i) {
        crc ^= (zragf_u32)buf[i];
        for (j = 0u; j < 8u; ++j) {
            zragf_u32 mask = (zragf_u32)(0u - (crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static zragf_u32 zragf_read_u32_be(const zragf_u8 *p)
{
    return ((zragf_u32)p[0] << 24)
         | ((zragf_u32)p[1] << 16)
         | ((zragf_u32)p[2] << 8)
         |  (zragf_u32)p[3];
}

static void zragf_write_u32_be(zragf_u8 *p, zragf_u32 v)
{
    p[0] = (zragf_u8)((v >> 24) & 0xFFu);
    p[1] = (zragf_u8)((v >> 16) & 0xFFu);
    p[2] = (zragf_u8)((v >> 8) & 0xFFu);
    p[3] = (zragf_u8)(v & 0xFFu);
}

static zragf_size_t zragf_cstr_len0(const zragf_u8 *s)
{
    zragf_size_t n = 0u;
    if (!s)
        return 0u;
    while (s[n] != 0u)
        n++;
    return n;
}

static int zragf_gzip_xflags_for_level(int level)
{
    if (level <= 2)
        return 4;
    if (level >= 8)
        return 2;
    return 0;
}

static zragf_size_t zragf_gzip_header_wire_size(const zragf_deflate_state_z *st)
{
    zragf_size_t size = 10u;

    if (!st)
        return 0u;
    if (st->gzip_extra && st->gzip_extra_len > 0u)
        size += 2u + st->gzip_extra_len;
    if (st->gzip_name)
        size += st->gzip_name_len + 1u;
    if (st->gzip_comment)
        size += st->gzip_comment_len + 1u;
    if (st->gzip_hcrc)
        size += 2u;
    return size;
}

static void zragf_deflate_free_metadata(zragf_stream *strm, zragf_deflate_state_z *st)
{
    if (!st)
        return;
    zragf_stream_free_bytes(strm, st->dict_buf);
    zragf_stream_free_bytes(strm, st->gzip_extra);
    zragf_stream_free_bytes(strm, st->gzip_name);
    zragf_stream_free_bytes(strm, st->gzip_comment);
    st->dict_buf = NULL;
    st->dict_len = 0u;
    st->dict_adler = 1u;
    st->dict_set = 0;
    st->gzip_extra = NULL;
    st->gzip_extra_len = 0u;
    st->gzip_name = NULL;
    st->gzip_name_len = 0u;
    st->gzip_comment = NULL;
    st->gzip_comment_len = 0u;
    st->gzip_mtime = 0u;
    st->gzip_text = 0;
    st->gzip_os = 255;
    st->gzip_hcrc = 0;
    st->gzip_header_set = 0;
}

static void zragf_inflate_reset_metadata(zragf_stream *strm, zragf_inflate_state_run *st)
{
    if (!st)
        return;
    zragf_stream_free_bytes(strm, st->base.dict_buf);
    st->base.dict_buf = NULL;
    st->base.dict_len = 0u;
    st->base.expected_dict_adler = 1u;
    st->base.need_dict = 0;
    st->base.saw_header = 0;
    if (st->base.gzip_header_out)
        st->base.gzip_header_out->done = 0;
    st->base.gzip_header_out = NULL;
}

static void zragf_window_load_dictionary(zragf_u8 *window,
                                         int window_size,
                                         int *window_pos,
                                         const zragf_u8 *dictionary,
                                         zragf_size_t dict_len)
{
    zragf_size_t use_len;
    if (!window || !window_pos || window_size <= 0)
        return;
    memset(window, 0, (size_t)window_size);
    if (!dictionary || dict_len == 0u) {
        *window_pos = 0;
        return;
    }
    use_len = dict_len;
    if (use_len > (zragf_size_t)window_size)
        use_len = (zragf_size_t)window_size;
    memcpy(window, dictionary + (dict_len - use_len), use_len);
    *window_pos = (int)(use_len % (zragf_size_t)window_size);
}

static int zragf_deflate_effective_window_bits(int windowBits)
{
    if (windowBits < 0)
        return -windowBits;
    if (windowBits > 15)
        return windowBits - 16;
    return windowBits;
}

static void zragf_window_append_bytes(zragf_u8 *window,
                                      int window_size,
                                      int *window_pos,
                                      zragf_size_t *window_filled,
                                      const zragf_u8 *src,
                                      zragf_size_t src_size)
{
    zragf_size_t i;

    if (!window || !window_pos || !window_filled || window_size <= 0 || !src)
        return;

    for (i = 0u; i < src_size; ++i) {
        window[*window_pos] = src[i];
        *window_pos = (*window_pos + 1) % window_size;
        if (*window_filled < (zragf_size_t)window_size)
            (*window_filled)++;
    }
}

static zragf_size_t zragf_window_copy_tail(const zragf_u8 *window,
                                           int window_size,
                                           int window_pos,
                                           zragf_size_t window_filled,
                                           zragf_u8 *dst)
{
    zragf_size_t used;
    zragf_size_t first;

    if (!window || !dst || window_size <= 0 || window_filled == 0u)
        return 0u;

    used = window_filled;
    if (used > (zragf_size_t)window_size)
        used = (zragf_size_t)window_size;

    if (used < (zragf_size_t)window_size) {
        memcpy(dst, window, used);
        return used;
    }

    first = (zragf_size_t)window_size - (zragf_size_t)window_pos;
    memcpy(dst, window + window_pos, first);
    if (window_pos > 0)
        memcpy(dst + first, window, (size_t)window_pos);
    return used;
}

static int zragf_copy_gzip_header_to_state(zragf_stream *strm,
                                           zragf_deflate_state_z *st,
                                           const zragf_gz_header *head)
{
    zragf_u8 *new_extra = NULL;
    zragf_u8 *new_name = NULL;
    zragf_u8 *new_comment = NULL;
    zragf_size_t name_len = 0u;
    zragf_size_t comment_len = 0u;

    if (!strm || !st || !head)
        return 0;
    if (head->extra_len > 65535u)
        return 0;

    if (head->extra && head->extra_len > 0u) {
        new_extra = (zragf_u8 *)zragf_stream_alloc_bytes(strm, (zragf_size_t)head->extra_len);
        if (!new_extra)
            return 0;
        memcpy(new_extra, head->extra, (size_t)head->extra_len);
    }

    if (head->name) {
        name_len = zragf_cstr_len0(head->name);
        new_name = (zragf_u8 *)zragf_stream_alloc_bytes(strm, name_len ? name_len : 1u);
        if (!new_name) {
            zragf_stream_free_bytes(strm, new_extra);
            return 0;
        }
        if (name_len > 0u)
            memcpy(new_name, head->name, name_len);
    }

    if (head->comment) {
        comment_len = zragf_cstr_len0(head->comment);
        new_comment = (zragf_u8 *)zragf_stream_alloc_bytes(strm, comment_len ? comment_len : 1u);
        if (!new_comment) {
            zragf_stream_free_bytes(strm, new_extra);
            zragf_stream_free_bytes(strm, new_name);
            return 0;
        }
        if (comment_len > 0u)
            memcpy(new_comment, head->comment, comment_len);
    }

    zragf_stream_free_bytes(strm, st->gzip_extra);
    zragf_stream_free_bytes(strm, st->gzip_name);
    zragf_stream_free_bytes(strm, st->gzip_comment);

    st->gzip_extra = new_extra;
    st->gzip_extra_len = head->extra ? (zragf_size_t)head->extra_len : 0u;
    st->gzip_name = new_name;
    st->gzip_name_len = name_len;
    st->gzip_comment = new_comment;
    st->gzip_comment_len = comment_len;
    st->gzip_text = head->text ? 1 : 0;
    st->gzip_mtime = head->time;
    st->gzip_os = head->os;
    st->gzip_hcrc = head->hcrc ? 1 : 0;
    st->gzip_header_set = 1;
    return 1;
}

static void zragf_fill_gzip_string_field(zragf_u8 **dst_ptr,
                                         unsigned int max,
                                         const zragf_u8 *src,
                                         zragf_size_t len)
{
    zragf_size_t copy_len;

    if (!dst_ptr)
        return;
    if (!src) {
        *dst_ptr = NULL;
        return;
    }
    if (!*dst_ptr || max == 0u)
        return;

    copy_len = len + 1u;
    if (copy_len > (zragf_size_t)max)
        copy_len = (zragf_size_t)max;
    if (copy_len > 1u)
        memcpy(*dst_ptr, src, copy_len - 1u);
    if (copy_len > len)
        (*dst_ptr)[copy_len - 1u] = 0u;
}

static int zragf_write_wrapper_header(zragf_deflate_state_z *st,
                                      zragf_u8 *dst,
                                      zragf_size_t dst_cap,
                                      zragf_size_t *written)
{
    if (!st || !dst || !written)
        return 0;

    *written = 0u;

    if (st->format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW)
        return 1;

    if (st->format == ZRAGF_INTERNAL_FMT_ZLIB) {
        int wbits = st->windowBits;
        zragf_u8 cmf;
        zragf_u8 flg = 0u;
        zragf_u32 cmf_flg;

        if (wbits < 8 || wbits > 15)
            wbits = 15;
        if (dst_cap < (st->dict_set ? 6u : 2u))
            return 0;

        cmf = (zragf_u8)((unsigned)(wbits - 8) << 4);
        cmf |= 8u;

        if (st->level <= 2)
            flg = (zragf_u8)(0u << 6);
        else if (st->level <= 5)
            flg = (zragf_u8)(1u << 6);
        else if (st->level <= 7)
            flg = (zragf_u8)(2u << 6);
        else
            flg = (zragf_u8)(3u << 6);
        if (st->dict_set)
            flg |= 0x20u;

        cmf_flg = ((zragf_u32)cmf << 8) | flg;
        flg = (zragf_u8)(flg + (31u - (cmf_flg % 31u)) % 31u);

        dst[0] = cmf;
        dst[1] = flg;
        *written = 2u;
        if (st->dict_set) {
            zragf_write_u32_be(dst + 2, st->dict_adler);
            *written += 4u;
        }
        return 1;
    }

    if (st->format == ZRAGF_INTERNAL_FMT_GZIP) {
        zragf_size_t pos = 0u;
        zragf_u8 flg = 0u;
        zragf_u32 hdr_crc;

        if (st->gzip_extra && st->gzip_extra_len > 0u)
            flg |= 0x04u;
        if (st->gzip_name)
            flg |= 0x08u;
        if (st->gzip_comment)
            flg |= 0x10u;
        if (st->gzip_hcrc)
            flg |= 0x02u;
        if (st->gzip_text)
            flg |= 0x01u;

        if (dst_cap < zragf_gzip_header_wire_size(st))
            return 0;

        dst[pos++] = 0x1Fu;
        dst[pos++] = 0x8Bu;
        dst[pos++] = 8u;
        dst[pos++] = flg;
        zragf_write_u32_le(dst + pos, st->gzip_mtime);
        pos += 4u;
        dst[pos++] = (zragf_u8)zragf_gzip_xflags_for_level(st->level);
        dst[pos++] = (zragf_u8)((st->gzip_os >= 0 && st->gzip_os <= 255) ? st->gzip_os : 255);

        if (st->gzip_extra && st->gzip_extra_len > 0u) {
            dst[pos++] = (zragf_u8)(st->gzip_extra_len & 0xFFu);
            dst[pos++] = (zragf_u8)((st->gzip_extra_len >> 8) & 0xFFu);
            memcpy(dst + pos, st->gzip_extra, st->gzip_extra_len);
            pos += st->gzip_extra_len;
        }
        if (st->gzip_name) {
            if (st->gzip_name_len > 0u)
                memcpy(dst + pos, st->gzip_name, st->gzip_name_len);
            pos += st->gzip_name_len;
            dst[pos++] = 0u;
        }
        if (st->gzip_comment) {
            if (st->gzip_comment_len > 0u)
                memcpy(dst + pos, st->gzip_comment, st->gzip_comment_len);
            pos += st->gzip_comment_len;
            dst[pos++] = 0u;
        }
        if (st->gzip_hcrc) {
            hdr_crc = zragf_crc32_update_running(0u, dst, pos);
            dst[pos++] = (zragf_u8)(hdr_crc & 0xFFu);
            dst[pos++] = (zragf_u8)((hdr_crc >> 8) & 0xFFu);
        }
        *written = pos;
        return 1;
    }

    return 0;
}

static int zragf_write_wrapper_trailer(zragf_internal_format format,
                                       zragf_u32 running_adler,
                                       zragf_u32 running_crc,
                                       zragf_u32 input_isize,
                                       zragf_u8 *dst,
                                       zragf_size_t dst_cap,
                                       zragf_size_t *written)
{
    if (!dst || !written)
        return 0;

    *written = 0u;

    if (format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW)
        return 1;

    if (format == ZRAGF_INTERNAL_FMT_ZLIB) {
        if (dst_cap < 4u)
            return 0;
        zragf_write_u32_be(dst, running_adler);
        *written = 4u;
        return 1;
    }

    if (format == ZRAGF_INTERNAL_FMT_GZIP) {
        if (dst_cap < 8u)
            return 0;
        zragf_write_u32_le(dst, running_crc);
        zragf_write_u32_le(dst + 4, input_isize);
        *written = 8u;
        return 1;
    }

    return 0;
}

static int zragf_pending_reserve(zragf_stream *strm,
                                zragf_deflate_state_z *st,
                                zragf_size_t extra)
{
    zragf_size_t remaining;
    zragf_size_t need;
    zragf_size_t newcap;
    zragf_u8 *nbuf;

    if (!st)
        return 0;
    if (extra == 0u)
        return 1;

    remaining = st->pending_size - st->pending_pos;
    if (st->pending_pos > 0u && remaining > 0u)
        memmove(st->pending_out, st->pending_out + st->pending_pos, remaining);
    st->pending_pos = 0u;
    st->pending_size = remaining;

    if (extra > ((zragf_size_t)-1) - st->pending_size)
        return 0;

    need = st->pending_size + extra;
    if (need <= st->pending_cap)
        return 1;

    newcap = (st->pending_cap == 0u) ? 4096u : st->pending_cap;
    while (newcap < need) {
        if (newcap > (((zragf_size_t)-1) / 2u)) {
            newcap = need;
            break;
        }
        newcap *= 2u;
    }

    nbuf = (zragf_u8 *)zragf_stream_realloc_bytes(strm,
                                                  st->pending_out,
                                                  st->pending_cap,
                                                  newcap);
    if (!nbuf)
        return 0;
    st->pending_out = nbuf;
    st->pending_cap = newcap;
    return 1;
}

static int zragf_pending_drain(zragf_stream *strm,
                               zragf_deflate_state_z *st,
                               zragf_size_t *copied)
{
    zragf_size_t remaining;
    zragf_size_t chunk;

    if (copied)
        *copied = 0u;
    if (!strm || !st)
        return 0;

    remaining = st->pending_size - st->pending_pos;
    if (remaining == 0u)
        return 1;
    if (!strm->next_out || strm->avail_out == 0u)
        return 1;

    chunk = (remaining < strm->avail_out) ? remaining : strm->avail_out;
    memcpy(strm->next_out, st->pending_out + st->pending_pos, chunk);
    st->pending_pos += chunk;
    strm->next_out += chunk;
    strm->avail_out -= chunk;
    strm->total_out += (zragf_u32)chunk;

    if (copied)
        *copied = chunk;

    if (st->pending_pos == st->pending_size) {
        st->pending_pos = 0u;
        st->pending_size = 0u;
    }
    return 1;
}

static void zragf_consume_prefix(zragf_u8 *buf,
                                 zragf_size_t *size,
                                 zragf_size_t count)
{
    if (!size || !buf || count == 0u)
        return;
    if (count >= *size) {
        *size = 0u;
        return;
    }
    memmove(buf, buf + count, *size - count);
    *size -= count;
}

static zragf_size_t zragf_stream_chunk_target(const zragf_deflate_state_z *st)
{
    zragf_size_t target = 65536u;

    if (!st)
        return 65536u;

    if (st->strategy == ZRAGF_Z_HUFFMAN_ONLY || st->strategy == ZRAGF_Z_RLE) {
        target = 32768u;
    } else if (st->strategy == ZRAGF_Z_FIXED) {
        target = (st->level >= 6) ? 131072u : 65536u;
    } else if (st->level >= 6) {
        target = (st->window_filled >= 32768u) ? 1048576u : 131072u;
    } else if (st->level >= 4) {
        target = 262144u;
    } else if (st->level <= 3) {
        target = 32768u;
    }

    if (st->memLevel > 0 && st->memLevel <= 5)
        target /= 2u;
    if (target < 16384u)
        target = 16384u;
    return target;
}

static int zragf_deflate_emit_header(zragf_stream *strm,
                                     zragf_deflate_state_z *st)
{
    zragf_size_t written = 0u;
    zragf_size_t reserve = 0u;

    if (!st || st->wrote_header)
        return 1;
    if (st->format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW) {
        st->wrote_header = 1;
        return 1;
    }

    if (st->format == ZRAGF_INTERNAL_FMT_ZLIB)
        reserve = st->dict_set ? 6u : 2u;
    else if (st->format == ZRAGF_INTERNAL_FMT_GZIP)
        reserve = zragf_gzip_header_wire_size(st);

    if (!zragf_pending_reserve(strm, st, reserve))
        return 0;
    if (!zragf_write_wrapper_header(st,
                                    st->pending_out + st->pending_size,
                                    st->pending_cap - st->pending_size,
                                    &written))
        return 0;
    st->pending_size += written;
    st->wrote_header = 1;
    return 1;
}

static int zragf_deflate_emit_trailer(zragf_stream *strm,
                                      zragf_deflate_state_z *st)
{
    zragf_size_t written = 0u;
    zragf_size_t reserve = 0u;

    if (!st)
        return 0;
    if (st->format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW)
        return 1;

    if (st->format == ZRAGF_INTERNAL_FMT_ZLIB)
        reserve = 4u;
    else if (st->format == ZRAGF_INTERNAL_FMT_GZIP)
        reserve = 8u;

    if (!zragf_pending_reserve(strm, st, reserve))
        return 0;
    if (!zragf_write_wrapper_trailer(st->format,
                                     st->running_adler,
                                     st->running_crc,
                                     st->input_isize,
                                     st->pending_out + st->pending_size,
                                     st->pending_cap - st->pending_size,
                                     &written))
        return 0;
    st->pending_size += written;
    return 1;
}

static int zragf_deflate_emit_block(zragf_stream *strm,
                                    zragf_deflate_state_z *st,
                                    const zragf_u8 *src,
                                    zragf_size_t src_size,
                                    int final_block,
                                    int force_stored)
{
    zragf_size_t bound;
    zragf_size_t written = 0u;
    zragf_u8 *history = NULL;
    zragf_size_t history_len = 0u;
    int ok;

    if (!strm || !st)
        return 0;

    bound = zragf_deflate_rfc1951_stored_bound(src_size);
    if (!zragf_pending_reserve(strm, st, bound))
        return 0;

    if (!force_stored && src_size > 0u && st->window && st->window_filled > 0u) {
        history = (zragf_u8 *)zragf_stream_alloc_bytes(strm, st->window_filled);
        if (!history)
            return 0;
        history_len = zragf_window_copy_tail(st->window,
                                             st->window_size,
                                             st->window_pos,
                                             st->window_filled,
                                             history);
    }

    if (force_stored || src_size == 0u) {
        ok = zragf_deflate_rfc1951_store_chunk_stream(src,
                                                      src_size,
                                                      st->pending_out + st->pending_size,
                                                      st->pending_cap - st->pending_size,
                                                      &written,
                                                      final_block,
                                                      &st->bitbuf,
                                                      &st->bitcount);
    } else {
        zragf_allocator saved_alloc;
        int alloc_override_active;

        zragf_stream_allocator_scope_begin(strm, &saved_alloc, &alloc_override_active);
        ok = zragf_deflate_rfc1951_compress_chunk_stream(history,
                                                         history_len,
                                                         src,
                                                         src_size,
                                                         st->pending_out + st->pending_size,
                                                         st->pending_cap - st->pending_size,
                                                         &written,
                                                         final_block,
                                                         st->level,
                                                         st->strategy,
                                                         st->tune_set,
                                                         st->tune_good_length,
                                                         st->tune_max_lazy,
                                                         st->tune_nice_length,
                                                         st->tune_max_chain,
                                                         &st->bitbuf,
                                                         &st->bitcount,
                                                         final_block ? 1 : 0);
        zragf_stream_allocator_scope_end(&saved_alloc, alloc_override_active);
    }

    zragf_stream_free_bytes(strm, history);
    if (!ok)
        return 0;

    st->pending_size += written;
    if (src && src_size > 0u && st->window && st->window_size > 0)
        zragf_window_append_bytes(st->window,
                                  st->window_size,
                                  &st->window_pos,
                                  &st->window_filled,
                                  src,
                                  src_size);
    return 1;
}

static int zragf_deflate_process_prefix(zragf_stream *strm,
                                        zragf_deflate_state_z *st,
                                        zragf_size_t bytes,
                                        int finish_on_last)
{
    zragf_size_t processed = 0u;
    zragf_size_t chunk_target = zragf_stream_chunk_target(st);

    if (!st)
        return 0;
    if (bytes == 0u)
        return 1;

    while (processed < bytes) {
        zragf_size_t remaining = bytes - processed;
        zragf_size_t chunk = (remaining > chunk_target) ? chunk_target : remaining;
        int final_block = (finish_on_last && processed + chunk == bytes) ? 1 : 0;

        if (!zragf_deflate_emit_block(strm,
                                      st,
                                      st->in_buf + processed,
                                      chunk,
                                      final_block,
                                      st->level == 0 ? 1 : 0))
            return 0;
        processed += chunk;
    }

    zragf_consume_prefix(st->in_buf, &st->in_size, bytes);
    return 1;
}

static int zragf_deflate_generate_for_flush(zragf_stream *strm,
                                            zragf_deflate_state_z *st,
                                            int flush,
                                            int *generated)
{
    zragf_size_t chunk_target;
    zragf_size_t ready_bytes;

    if (generated)
        *generated = 0;
    if (!strm || !st)
        return 0;
    if (st->final_emitted)
        return 1;

    if (!zragf_deflate_emit_header(strm, st))
        return 0;

    chunk_target = zragf_stream_chunk_target(st);

    if (flush == ZRAGF_NO_FLUSH) {
        if (st->in_size < chunk_target)
            return 1;
        ready_bytes = st->in_size - (st->in_size % chunk_target);
        if (ready_bytes == 0u)
            return 1;
        if (!zragf_deflate_process_prefix(strm, st, ready_bytes, 0))
            return 0;
        if (generated)
            *generated = 1;
        return 1;
    }

    if (flush == ZRAGF_PARTIAL_FLUSH || flush == ZRAGF_SYNC_FLUSH ||
        flush == ZRAGF_FULL_FLUSH) {
        if (st->in_size > 0u) {
            if (!zragf_deflate_emit_block(strm, st, st->in_buf, st->in_size, 0, st->level == 0 ? 1 : 0))
                return 0;
            zragf_consume_prefix(st->in_buf, &st->in_size, st->in_size);
        }
        if (!zragf_deflate_emit_block(strm, st, NULL, 0u, 0, 1))
            return 0;
        if (generated)
            *generated = 1;
        return 1;
    }

    if (flush == ZRAGF_FINISH) {
        if (st->in_size > 0u) {
            if (!zragf_deflate_emit_block(strm, st, st->in_buf, st->in_size, 1, st->level == 0 ? 1 : 0))
                return 0;
            zragf_consume_prefix(st->in_buf, &st->in_size, st->in_size);
        } else {
            if (!zragf_deflate_emit_block(strm, st, NULL, 0u, 1, 1))
                return 0;
        }
        if (!zragf_deflate_emit_trailer(strm, st))
            return 0;
        st->final_emitted = 1;
        if (generated)
            *generated = 1;
        return 1;
    }

    return 1;
}

/* ================================================================
   API NATIVA BUFFERED

   ================================================================ */
zragf_status zragf_deflate_init(zragf_stream *strm, zragf_level level)
{
    zragf_native_deflate_state *st;

    if (!strm)
        return ZRAGF_ST_NULL_POINTER;

    zragf_stream_prepare_allocators(strm);

    st = (zragf_native_deflate_state *)zragf_stream_alloc_bytes(strm, sizeof(*st));
    if (!st)
        return ZRAGF_ST_INTERNAL;

    memset(st, 0, sizeof(*st));
    st->kind = ZRAGF_STATE_KIND_NATIVE_DEFLATE;
    st->level = level;

    strm->next_in = NULL;
    strm->avail_in = 0u;
    strm->next_out = NULL;
    strm->avail_out = 0u;
    zragf_stream_reset_public_fields(strm);
    strm->state = st;

    return ZRAGF_ST_OK;
}

zragf_status zragf_deflate(zragf_stream *strm, zragf_flush flush)
{
    zragf_native_deflate_state *st;
    zragf_status rc;
    zragf_size_t out_cap;

    if (!strm || !strm->state)
        return ZRAGF_ST_NULL_POINTER;

    st = (zragf_native_deflate_state *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_NATIVE_DEFLATE)
        return ZRAGF_ST_INTERNAL;

    if (strm->avail_in > 0u && strm->next_in) {
        zragf_size_t consumed_in = strm->avail_in;
        rc = zragf_append_input(strm, &st->in_buf, &st->in_size, &st->in_cap,
                                strm->next_in, strm->avail_in);
        if (rc != ZRAGF_ST_OK)
            return rc;
        strm->total_in += (zragf_u32)consumed_in;
        strm->next_in += consumed_in;
        strm->avail_in = 0u;
    }

    if (flush == ZRAGF_NO_FLUSH)
        return ZRAGF_ST_OK;

    if (!strm->next_out || strm->avail_out == 0u)
        return ZRAGF_ST_OUTPUT_TOO_SMALL;

    out_cap = strm->avail_out;
    rc = zragf_compress(st->in_buf, st->in_size, strm->next_out, &out_cap, st->level);
    if (rc != ZRAGF_ST_OK)
        return rc;

    strm->total_out += (zragf_u32)out_cap;
    strm->next_out += out_cap;
    strm->avail_out -= out_cap;
    st->in_size = 0u;
    return ZRAGF_ST_OK;
}

zragf_status zragf_deflate_end(zragf_stream *strm)
{
    zragf_native_deflate_state *st;

    if (!strm || !strm->state)
        return ZRAGF_ST_NULL_POINTER;

    st = (zragf_native_deflate_state *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_NATIVE_DEFLATE)
        return ZRAGF_ST_INTERNAL;

    zragf_stream_free_bytes(strm, st->in_buf);
    zragf_stream_free_bytes(strm, st);
    strm->state = NULL;
    return ZRAGF_ST_OK;
}

zragf_status zragf_inflate_init(zragf_stream *strm)
{
    zragf_native_inflate_state *st;

    if (!strm)
        return ZRAGF_ST_NULL_POINTER;

    zragf_stream_prepare_allocators(strm);

    st = (zragf_native_inflate_state *)zragf_stream_alloc_bytes(strm, sizeof(*st));
    if (!st)
        return ZRAGF_ST_INTERNAL;

    memset(st, 0, sizeof(*st));
    st->kind = ZRAGF_STATE_KIND_NATIVE_INFLATE;

    strm->next_in = NULL;
    strm->avail_in = 0u;
    strm->next_out = NULL;
    strm->avail_out = 0u;
    zragf_stream_reset_public_fields(strm);
    strm->state = st;

    return ZRAGF_ST_OK;
}

zragf_status zragf_inflate(zragf_stream *strm, zragf_flush flush)
{
    zragf_native_inflate_state *st;
    zragf_status rc;
    zragf_size_t out_cap;
    zragf_info info;

    if (!strm || !strm->state)
        return ZRAGF_ST_NULL_POINTER;

    st = (zragf_native_inflate_state *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_NATIVE_INFLATE)
        return ZRAGF_ST_INTERNAL;

    if (strm->avail_in > 0u && strm->next_in) {
        zragf_size_t consumed_in = strm->avail_in;
        rc = zragf_append_input(strm, &st->in_buf, &st->in_size, &st->in_cap,
                                strm->next_in, strm->avail_in);
        if (rc != ZRAGF_ST_OK)
            return rc;
        strm->total_in += (zragf_u32)consumed_in;
        strm->next_in += consumed_in;
        strm->avail_in = 0u;
    }

    if (flush == ZRAGF_NO_FLUSH)
        return ZRAGF_ST_OK;

    if (!strm->next_out || strm->avail_out == 0u)
        return ZRAGF_ST_OUTPUT_TOO_SMALL;

    out_cap = strm->avail_out;
    rc = zragf_decompress(st->in_buf, st->in_size, strm->next_out, &out_cap, &info);
    if (rc != ZRAGF_ST_OK)
        return rc;

    strm->total_out += (zragf_u32)out_cap;
    strm->next_out += out_cap;
    strm->avail_out -= out_cap;
    st->in_size = 0u;
    return ZRAGF_ST_OK;
}

zragf_status zragf_inflate_end(zragf_stream *strm)
{
    zragf_native_inflate_state *st;

    if (!strm || !strm->state)
        return ZRAGF_ST_NULL_POINTER;

    st = (zragf_native_inflate_state *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_NATIVE_INFLATE)
        return ZRAGF_ST_INTERNAL;

    zragf_stream_free_bytes(strm, st->in_buf);
    zragf_stream_free_bytes(strm, st);
    strm->state = NULL;
    return ZRAGF_ST_OK;
}

/* ================================================================
   API ZLIB-COMPATIBLE (baseline honesto, buffered por chunk)
   ================================================================ */
const char *zragf_version(void)
{
    return "zragf/compat-0.70.0";
}

static void zragf_deflate_state_reset(zragf_stream *strm, zragf_deflate_state_z *st)
{
    if (!st)
        return;
    st->in_size = 0u;
    st->window_pos = 0;
    st->window_filled = 0u;
    st->bitbuf = 0u;
    st->bitcount = 0;
    st->blk_size = 0u;
    st->pending_pos = 0u;
    st->pending_size = 0u;
    st->final_emitted = 0;
    st->finished = 0;
    st->wrote_header = 0;
    st->running_adler = 1u;
    st->running_crc = 0u;
    st->input_isize = 0u;
    if (st->window && st->window_size > 0) {
        if (st->dict_set && st->dict_buf && st->dict_len > 0u) {
            zragf_window_load_dictionary(st->window, st->window_size, &st->window_pos,
                                         st->dict_buf, st->dict_len);
            st->window_filled = (st->dict_len > (zragf_size_t)st->window_size)
                              ? (zragf_size_t)st->window_size : st->dict_len;
        } else {
            memset(st->window, 0, (size_t)st->window_size);
        }
    }
    zragf_stream_reset_public_fields(strm);
    strm->data_type = ZRAGF_Z_BINARY;
    zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
}

int zragf_deflateInit(zragf_stream *strm, int level)
{
    return zragf_deflateInit2(strm, level, 8, 15, 8, 0);
}

int zragf_deflateInit2(zragf_stream *strm,
                       int level,
                       int method,
                       int windowBits,
                       int memLevel,
                       int strategy)
{
    zragf_internal_format fmt;
    zragf_deflate_state_z *st;

    if (!strm)
        return ZRAGF_STREAM_ERROR;
    if (method != 8)
        return ZRAGF_STREAM_ERROR;
    if (!zragf_level_valid(level) || !zragf_memlevel_valid(memLevel)
        || !zragf_strategy_valid(strategy)
        || !zragf_deflate_window_bits_valid(windowBits))
        return ZRAGF_STREAM_ERROR;

    zragf_stream_prepare_allocators(strm);

    if (level == -1)
        level = 6;

    fmt = zragf_select_format(windowBits);

    st = (zragf_deflate_state_z *)zragf_stream_alloc_bytes(strm, sizeof(*st));
    if (!st)
        return ZRAGF_MEM_ERROR;

    memset(st, 0, sizeof(*st));
    st->kind       = ZRAGF_STATE_KIND_Z_DEFLATE;
    st->format     = fmt;
    st->level      = level;
    st->method     = method;
    st->windowBits = windowBits;
    st->memLevel   = memLevel;
    st->strategy   = strategy;
    st->running_adler = 1u;
    st->running_crc   = 0u;
    st->input_isize   = 0u;
    st->dict_adler    = 1u;
    st->gzip_os       = 255;
    st->window_size   = 1 << zragf_deflate_effective_window_bits(windowBits);
    st->window_pos    = 0;
    st->window_filled = 0u;
    st->window = (zragf_u8 *)zragf_stream_alloc_bytes(strm, (zragf_size_t)st->window_size);
    if (!st->window) {
        zragf_stream_free_bytes(strm, st);
        strm->state = NULL;
        return ZRAGF_MEM_ERROR;
    }
    memset(st->window, 0, (size_t)st->window_size);

    strm->state = st;
    zragf_stream_reset_public_fields(strm);
    strm->data_type = ZRAGF_Z_BINARY;
    zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
    return ZRAGF_OK;
}

int zragf_deflateEndZ(zragf_stream *strm)
{
    zragf_deflate_state_z *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind == ZRAGF_STATE_KIND_NATIVE_DEFLATE)
        return (zragf_deflate_end(strm) == ZRAGF_ST_OK) ? ZRAGF_OK : ZRAGF_STREAM_ERROR;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;

    zragf_deflate_free_metadata(strm, st);
    zragf_stream_free_bytes(strm, st->in_buf);
    zragf_stream_free_bytes(strm, st->window);
    zragf_stream_free_bytes(strm, st->blk_out);
    zragf_stream_free_bytes(strm, st->pending_out);
    zragf_stream_free_bytes(strm, st);
    strm->state = NULL;
    return ZRAGF_OK;
}

int zragf_deflateReset(zragf_stream *strm)
{
    zragf_deflate_state_z *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind == ZRAGF_STATE_KIND_NATIVE_DEFLATE) {
        zragf_native_deflate_state *nst = (zragf_native_deflate_state *)strm->state;
        nst->in_size = 0u;
        zragf_stream_reset_public_fields(strm);
        return ZRAGF_OK;
    }
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;

    zragf_deflate_free_metadata(strm, st);
    zragf_deflate_state_reset(strm, st);
    return ZRAGF_OK;
}

int zragf_deflatePending(zragf_stream *strm, unsigned *pending, int *bits)
{
    zragf_deflate_state_z *st;
    zragf_size_t pending_bytes;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;

    pending_bytes = st->pending_size - st->pending_pos;
    if (pending)
        *pending = (unsigned)pending_bytes;
    if (bits)
        *bits = 0;
    return ZRAGF_OK;
}

unsigned long zragf_deflateBound(zragf_stream *strm, unsigned long sourceLen)
{
    zragf_deflate_state_z *st;
    unsigned long bound;

    if (!strm || !strm->state)
        return 0UL;

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return 0UL;

    bound = (unsigned long)zragf_deflate_rfc1951_stored_bound((zragf_size_t)sourceLen);
    if (st->format == ZRAGF_INTERNAL_FMT_ZLIB)
        bound += (unsigned long)(st->dict_set ? 10u : 6u);
    else if (st->format == ZRAGF_INTERNAL_FMT_GZIP)
        bound += (unsigned long)(zragf_gzip_header_wire_size(st) + 8u);
    return bound;
}

int zragf_deflateParams(zragf_stream *strm, int level, int strategy)
{
    zragf_deflate_state_z *st;
    zragf_u8 *saved_next_in;
    zragf_size_t saved_avail_in;
    int rc;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    if (!zragf_level_valid(level) || !zragf_strategy_valid(strategy))
        return ZRAGF_STREAM_ERROR;

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;
    if (level == -1)
        level = 6;
    if (st->finished || st->final_emitted)
        return ZRAGF_STREAM_ERROR;

    if (st->level == level && st->strategy == strategy)
        return ZRAGF_OK;

    if (st->in_size != 0u || st->pending_size != st->pending_pos) {
        saved_next_in = strm->next_in;
        saved_avail_in = strm->avail_in;
        strm->next_in = NULL;
        strm->avail_in = 0u;
        rc = zragf_deflateZ(strm, ZRAGF_SYNC_FLUSH);
        strm->next_in = saved_next_in;
        strm->avail_in = saved_avail_in;
        if (rc == ZRAGF_BUF_ERROR)
            return ZRAGF_BUF_ERROR;
        if (rc < 0)
            return rc;
        if (st->in_size != 0u || st->pending_size != st->pending_pos)
            return ZRAGF_BUF_ERROR;
    }

    st->level = level;
    st->strategy = strategy;
    return ZRAGF_OK;
}

int zragf_deflateCopy(zragf_stream *dest, zragf_stream *source)
{
    zragf_deflate_state_z *src;
    zragf_deflate_state_z *dst;

    if (!dest || !source || !source->state)
        return ZRAGF_STREAM_ERROR;
    if (dest->state)
        return ZRAGF_STREAM_ERROR;
    src = (zragf_deflate_state_z *)source->state;
    if (src->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;

    *dest = *source;
    zragf_stream_prepare_allocators(dest);
    dst = (zragf_deflate_state_z *)zragf_stream_alloc_bytes(dest, sizeof(*dst));
    if (!dst)
        return ZRAGF_MEM_ERROR;
    *dst = *src;
    dst->in_buf = NULL;
    dst->window = NULL;
    dst->blk_out = NULL;
    dst->pending_out = NULL;
    dst->dict_buf = NULL;
    dst->gzip_extra = NULL;
    dst->gzip_name = NULL;
    dst->gzip_comment = NULL;

    if (!zragf_stream_clone_buffer(dest, &dst->in_buf, src->in_buf, src->in_cap, src->in_size) ||
        !zragf_stream_clone_buffer(dest, &dst->window, src->window, (zragf_size_t)((src->window_size > 0) ? src->window_size : 0), (zragf_size_t)((src->window_size > 0) ? src->window_size : 0)) ||
        !zragf_stream_clone_buffer(dest, &dst->blk_out, src->blk_out, src->blk_cap, src->blk_size) ||
        !zragf_stream_clone_buffer(dest, &dst->pending_out, src->pending_out, src->pending_cap, src->pending_size) ||
        !zragf_stream_clone_buffer(dest, &dst->dict_buf, src->dict_buf, src->dict_len, src->dict_len) ||
        !zragf_stream_clone_buffer(dest, &dst->gzip_extra, src->gzip_extra, src->gzip_extra_len, src->gzip_extra_len) ||
        !zragf_stream_clone_str0(dest, &dst->gzip_name, src->gzip_name, src->gzip_name_len) ||
        !zragf_stream_clone_str0(dest, &dst->gzip_comment, src->gzip_comment, src->gzip_comment_len)) {
        zragf_deflate_state_z tmp = *dst;
        zragf_deflate_free_metadata(dest, &tmp);
        zragf_stream_free_bytes(dest, dst->in_buf);
        zragf_stream_free_bytes(dest, dst->window);
        zragf_stream_free_bytes(dest, dst->blk_out);
        zragf_stream_free_bytes(dest, dst->pending_out);
        zragf_stream_free_bytes(dest, dst);
        dest->state = NULL;
        return ZRAGF_MEM_ERROR;
    }

    dest->state = dst;
    return ZRAGF_OK;
}

int zragf_deflateTune(zragf_stream *strm, int good_length, int max_lazy, int nice_length, int max_chain)
{
    zragf_deflate_state_z *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;
    if (good_length < 0 || max_lazy < 0 || nice_length < 3 || nice_length > 258 || max_chain < 1)
        return ZRAGF_STREAM_ERROR;

    st->tune_good_length = good_length;
    st->tune_max_lazy = max_lazy;
    st->tune_nice_length = nice_length;
    st->tune_max_chain = max_chain;
    st->tune_set = 1;
    return ZRAGF_OK;
}

int zragf_deflateSetDictionary(zragf_stream *strm, const zragf_u8 *dictionary, unsigned int dictLength)
{
    zragf_deflate_state_z *st;
    zragf_u8 *copy = NULL;
    zragf_size_t keep;

    if (!strm || !strm->state || (!dictionary && dictLength != 0u))
        return ZRAGF_STREAM_ERROR;
    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->format == ZRAGF_INTERNAL_FMT_GZIP)
        return ZRAGF_STREAM_ERROR;
    if (st->wrote_header || st->final_emitted || st->in_size != 0u ||
        st->pending_size != st->pending_pos || strm->total_in != 0u)
        return ZRAGF_STREAM_ERROR;

    zragf_stream_free_bytes(strm, st->dict_buf);
    st->dict_buf = NULL;
    st->dict_len = 0u;
    st->dict_adler = 1u;
    st->dict_set = 0;

    if (dictLength == 0u) {
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
        return ZRAGF_OK;
    }

    keep = (zragf_size_t)dictLength;
    if (keep > 32768u)
        keep = 32768u;
    copy = (zragf_u8 *)zragf_stream_alloc_bytes(strm, keep);
    if (!copy)
        return ZRAGF_MEM_ERROR;
    memcpy(copy, dictionary + ((zragf_size_t)dictLength - keep), keep);

    st->dict_buf = copy;
    st->dict_len = keep;
    st->dict_adler = zragf_adler32_update(1u, dictionary, (zragf_size_t)dictLength);
    st->dict_set = 1;
    if (st->window && st->window_size > 0) {
        zragf_window_load_dictionary(st->window, st->window_size, &st->window_pos,
                                     st->dict_buf, st->dict_len);
        st->window_filled = st->dict_len;
    }
    if (st->format == ZRAGF_INTERNAL_FMT_ZLIB)
        strm->adler = st->dict_adler;
    return ZRAGF_OK;
}

int zragf_deflateSetHeader(zragf_stream *strm, zragf_gz_headerp head)
{
    zragf_deflate_state_z *st;

    if (!strm || !strm->state || !head)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE || st->format != ZRAGF_INTERNAL_FMT_GZIP)
        return ZRAGF_STREAM_ERROR;
    if (st->wrote_header || st->final_emitted || st->in_size != 0u ||
        st->pending_size != st->pending_pos || strm->total_in != 0u)
        return ZRAGF_STREAM_ERROR;

    if (!zragf_copy_gzip_header_to_state(strm, st, head))
        return ZRAGF_MEM_ERROR;
    return ZRAGF_OK;
}

int zragf_deflateZ(zragf_stream *strm, int flush)
{
    zragf_deflate_state_z *st;
    zragf_status rc;
    zragf_size_t copied = 0u;
    zragf_size_t appended = 0u;
    int generated = 0;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    if (flush < ZRAGF_NO_FLUSH || flush > ZRAGF_FINISH)
        return ZRAGF_STREAM_ERROR;

    if (((zragf_native_deflate_state *)strm->state)->kind == ZRAGF_STATE_KIND_NATIVE_DEFLATE) {
        zragf_status nrc = zragf_deflate(strm, (zragf_flush)flush);
        if (nrc == ZRAGF_ST_OK)
            return ZRAGF_OK;
        if (nrc == ZRAGF_ST_OUTPUT_TOO_SMALL)
            return ZRAGF_BUF_ERROR;
        return ZRAGF_DATA_ERROR;
    }

    st = (zragf_deflate_state_z *)strm->state;
    if (st->kind != ZRAGF_STATE_KIND_Z_DEFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->finished) {
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
        return ZRAGF_STREAM_END;
    }
    strm->msg = NULL;
    strm->data_type = ZRAGF_Z_BINARY;

    if (strm->avail_in > 0u && strm->next_in) {
        zragf_size_t consumed_in = strm->avail_in;
        appended = consumed_in;
        rc = zragf_append_input(strm, &st->in_buf, &st->in_size, &st->in_cap,
                                strm->next_in, strm->avail_in);
        if (rc != ZRAGF_ST_OK)
            return ZRAGF_MEM_ERROR;
        st->running_adler = zragf_adler32_update(st->running_adler,
                                                 strm->next_in,
                                                 consumed_in);
        st->running_crc = zragf_crc32_update_running(st->running_crc,
                                                     strm->next_in,
                                                     consumed_in);
        st->input_isize += (zragf_u32)consumed_in;
        strm->total_in += (zragf_u32)consumed_in;
        strm->next_in += consumed_in;
        strm->avail_in = 0u;
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
    }

    if (st->pending_size != st->pending_pos) {
        if (!zragf_pending_drain(strm, st, &copied))
            return ZRAGF_STREAM_ERROR;
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
        if (st->pending_size != st->pending_pos)
            return (copied > 0u || appended > 0u) ? ZRAGF_OK : ZRAGF_BUF_ERROR;
        if (st->final_emitted) {
            st->finished = 1;
            return ZRAGF_STREAM_END;
        }
    }

    if (flush == ZRAGF_NO_FLUSH && st->in_size < zragf_stream_chunk_target(st)) {
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
        return ZRAGF_OK;
    }

    if (!strm->next_out || strm->avail_out == 0u) {
        zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);
        return (flush == ZRAGF_NO_FLUSH) ? ZRAGF_OK : ZRAGF_BUF_ERROR;
    }

    if (!zragf_deflate_generate_for_flush(strm, st, flush, &generated))
        return ZRAGF_MEM_ERROR;

    copied = 0u;
    if (st->pending_size != st->pending_pos) {
        if (!zragf_pending_drain(strm, st, &copied))
            return ZRAGF_STREAM_ERROR;
    }

    zragf_stream_set_checksum(strm, st->format, st->running_adler, st->running_crc);

    if (st->final_emitted && st->pending_size == st->pending_pos) {
        st->finished = 1;
        return ZRAGF_STREAM_END;
    }

    if (copied > 0u || appended > 0u || generated)
        return ZRAGF_OK;
    if (flush == ZRAGF_NO_FLUSH)
        return ZRAGF_OK;
    return ZRAGF_BUF_ERROR;
}


static void zragf_inf_run_mark_save(const zragf_inflate_state_run *st,
                                    zragf_inf_run_mark *mark)
{
    if (!st || !mark)
        return;
    mark->in_pos = st->in_pos;
    mark->bitbuf = st->base.bitbuf;
    mark->bitcount = st->base.bitcount;
    mark->total_in_consumed = st->total_in_consumed;
}

static void zragf_inf_run_mark_restore(zragf_inflate_state_run *st,
                                       const zragf_inf_run_mark *mark)
{
    if (!st || !mark)
        return;
    st->in_pos = mark->in_pos;
    st->base.bitbuf = mark->bitbuf;
    st->base.bitcount = mark->bitcount;
    st->total_in_consumed = mark->total_in_consumed;
}

static zragf_size_t zragf_inf_run_bytes_available(const zragf_inflate_state_run *st)
{
    if (!st || st->base.in_size < st->in_pos)
        return 0u;
    return st->base.in_size - st->in_pos;
}

static void zragf_inf_run_consume_bytes(zragf_inflate_state_run *st,
                                        zragf_size_t count)
{
    if (!st || count == 0u)
        return;
    st->in_pos += count;
    st->total_in_consumed += (zragf_u32)count;
}

static void zragf_inf_run_align_byte(zragf_inflate_state_run *st)
{
    if (!st)
        return;
    st->base.bitbuf = 0u;
    st->base.bitcount = 0;
}

static void zragf_inf_run_free_tables(zragf_stream *strm,
                                      zragf_inflate_state_run *st)
{
    if (!st)
        return;
    zragf_stream_inflate_free_huff(strm, &st->huff);
}

static void zragf_inf_run_compact_input(zragf_inflate_state_run *st)
{
    zragf_size_t remaining;

    if (!st || !st->base.in_buf)
        return;

    if (st->in_pos == 0u)
        return;

    if (st->in_pos >= st->base.in_size) {
        st->base.in_size = 0u;
        st->in_pos = 0u;
        return;
    }

    if (st->in_pos < 4096u && st->in_pos * 2u < st->base.in_size)
        return;

    remaining = st->base.in_size - st->in_pos;
    memmove(st->base.in_buf, st->base.in_buf + st->in_pos, remaining);
    st->base.in_size = remaining;
    st->in_pos = 0u;
}

static zragf_status zragf_inf_run_append_input(zragf_stream *strm,
                                               zragf_inflate_state_run *st,
                                               const zragf_u8 *src,
                                               zragf_size_t len)
{
    if (!st)
        return ZRAGF_ST_NULL_POINTER;
    zragf_inf_run_compact_input(st);
    return zragf_append_input(strm,
                              &st->base.in_buf,
                              &st->base.in_size,
                              &st->base.in_cap,
                              src,
                              len);
}

static zragf_inf_run_step zragf_inf_run_getbit_nosnap(zragf_inflate_state_run *st,
                                                      int *bit)
{
    if (!st || !bit)
        return ZRAGF_INF_RUN_STEP_ERROR;

    if (st->base.bitcount == 0) {
        if (zragf_inf_run_bytes_available(st) == 0u)
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        st->base.bitbuf = st->base.in_buf[st->in_pos];
        zragf_inf_run_consume_bytes(st, 1u);
        st->base.bitcount = 8;
    }

    *bit = (int)(st->base.bitbuf & 1u);
    st->base.bitbuf >>= 1u;
    st->base.bitcount--;
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_read_bits_nosnap(zragf_inflate_state_run *st,
                                                         int n,
                                                         int *out_value)
{
    int i;
    int bit = 0;
    int value = 0;
    zragf_inf_run_step step;

    if (!st || !out_value || n < 0)
        return ZRAGF_INF_RUN_STEP_ERROR;

    for (i = 0; i < n; ++i) {
        step = zragf_inf_run_getbit_nosnap(st, &bit);
        if (step != ZRAGF_INF_RUN_STEP_OK)
            return step;
        value |= (bit & 1) << i;
    }

    *out_value = value;
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_decode_symbol_nosnap(zragf_inflate_state_run *st,
                                                             zragf_idec_table *tab,
                                                             int *out_sym)
{
    unsigned acc = 0u;
    int bits_read;
    int bit = 0;
    int i;
    zragf_inf_run_step step;

    if (!st || !tab || !out_sym)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if (!tab->table || tab->count <= 0 || tab->max_bits <= 0)
        return ZRAGF_INF_RUN_STEP_ERROR;

    for (bits_read = 1; bits_read <= tab->max_bits; ++bits_read) {
        step = zragf_inf_run_getbit_nosnap(st, &bit);
        if (step != ZRAGF_INF_RUN_STEP_OK)
            return step;
        acc |= (unsigned)(bit & 1) << (bits_read - 1);
        for (i = 0; i < tab->count; ++i) {
            if (tab->table[i].bits == bits_read && tab->table[i].code == acc) {
                *out_sym = tab->table[i].symbol;
                return ZRAGF_INF_RUN_STEP_OK;
            }
        }
    }

    return ZRAGF_INF_RUN_STEP_ERROR;
}

static zragf_inf_run_step zragf_inf_run_emit_byte(zragf_stream *strm,
                                                  zragf_inflate_state_run *st,
                                                  zragf_u8 value)
{
    if (!strm || !st)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if (!strm->next_out || strm->avail_out == 0u)
        return ZRAGF_INF_RUN_STEP_NEED_OUTPUT;

    *strm->next_out++ = value;
    strm->avail_out--;
    strm->total_out += 1u;

    zragf_inflate_window_put_byte(st->base.window,
                                  st->base.window_size,
                                  &st->base.window_pos,
                                  &st->base.window_filled,
                                  value);
    st->running_adler = zragf_adler32_update(st->running_adler, &value, 1u);
    st->running_crc = zragf_crc32_update_running(st->running_crc, &value, 1u);
    st->output_isize += 1u;
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_parse_header(zragf_inflate_state_run *st)
{
    zragf_size_t pos;
    const zragf_u8 *buf;

    if (!st)
        return ZRAGF_INF_RUN_STEP_ERROR;

    if (st->base.format == ZRAGF_INTERNAL_FMT_AUTO) {
        if (zragf_inf_run_bytes_available(st) < 2u)
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        buf = st->base.in_buf + st->in_pos;
        if (buf[0] == 0x1Fu && buf[1] == 0x8Bu)
            st->base.format = ZRAGF_INTERNAL_FMT_GZIP;
        else
            st->base.format = ZRAGF_INTERNAL_FMT_ZLIB;
    }

    if (st->base.format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW) {
        if (st->base.gzip_header_out)
            st->base.gzip_header_out->done = -1;
        st->base.saw_header = 1;
        st->run_state = ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
        return ZRAGF_INF_RUN_STEP_OK;
    }

    if (st->base.format == ZRAGF_INTERNAL_FMT_ZLIB) {
        zragf_u8 cmf;
        zragf_u8 flg;
        zragf_u32 header;
        int header_wbits;

        if (st->base.gzip_header_out)
            st->base.gzip_header_out->done = -1;

        if (zragf_inf_run_bytes_available(st) < 2u)
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        buf = st->base.in_buf + st->in_pos;
        cmf = buf[0];
        flg = buf[1];
        header = ((zragf_u32)cmf << 8) | flg;
        header_wbits = ((int)(cmf >> 4) & 0x0F) + 8;
        if ((cmf & 0x0Fu) != 8u)
            return ZRAGF_INF_RUN_STEP_ERROR;
        if ((cmf >> 4) > 7u)
            return ZRAGF_INF_RUN_STEP_ERROR;
        if ((header % 31u) != 0u)
            return ZRAGF_INF_RUN_STEP_ERROR;
        if (st->base.max_window_bits != 0 && header_wbits > st->base.max_window_bits)
            return ZRAGF_INF_RUN_STEP_ERROR;
        if (st->base.max_window_bits == 0) {
            st->base.max_window_bits = header_wbits;
            st->base.max_window_size = 1 << header_wbits;
        }
        if ((flg & 0x20u) != 0u) {
            if (zragf_inf_run_bytes_available(st) < 6u)
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            st->base.expected_dict_adler = zragf_read_u32_be(buf + 2u);
            st->base.need_dict = 1;
            st->base.saw_header = 1;
            zragf_inf_run_consume_bytes(st, 6u);
            st->run_state = ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
            return ZRAGF_INF_RUN_STEP_NEED_DICT;
        }
        st->base.saw_header = 1;
        zragf_inf_run_consume_bytes(st, 2u);
        st->run_state = ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
        return ZRAGF_INF_RUN_STEP_OK;
    }

    pos = st->in_pos;
    if (st->base.in_size - pos < 10u) {
        if (!zragf_inflate_gzip_meta_limit_ok(st->base.in_size - pos))
            return ZRAGF_INF_RUN_STEP_ERROR;
        return ZRAGF_INF_RUN_STEP_NEED_INPUT;
    }
    buf = st->base.in_buf;
    if (buf[pos + 0u] != 0x1Fu || buf[pos + 1u] != 0x8Bu || buf[pos + 2u] != 8u)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if ((buf[pos + 3u] & 0xE0u) != 0u)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if (!zragf_inflate_gzip_meta_limit_ok(10u))
        return ZRAGF_INF_RUN_STEP_ERROR;
    {
        zragf_u8 flg = buf[pos + 3u];
        zragf_size_t header_start = pos;
        zragf_size_t extra_data_pos = 0u;
        zragf_u16 xlen = 0u;
        zragf_size_t name_start = 0u;
        zragf_size_t name_len = 0u;
        zragf_size_t comment_start = 0u;
        zragf_size_t comment_len = 0u;
        zragf_u32 hdr_crc;
        zragf_gz_header *hout = st->base.gzip_header_out;

        pos += 10u;
        if ((flg & 0x04u) != 0u) {
            if (st->base.in_size - pos < 2u) {
                if (!zragf_inflate_gzip_meta_limit_ok(st->base.in_size - header_start))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            }
            xlen = (zragf_u16)buf[pos] | (zragf_u16)((zragf_u16)buf[pos + 1u] << 8);
            pos += 2u;
            if (!zragf_inflate_gzip_meta_limit_ok(pos - header_start))
                return ZRAGF_INF_RUN_STEP_ERROR;
            extra_data_pos = pos;
            if (!zragf_inflate_gzip_meta_limit_ok((pos - header_start) + (zragf_size_t)xlen))
                return ZRAGF_INF_RUN_STEP_ERROR;
            if (st->base.in_size - pos < (zragf_size_t)xlen)
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            pos += (zragf_size_t)xlen;
        }
        if ((flg & 0x08u) != 0u) {
            name_start = pos;
            while (pos < st->base.in_size && buf[pos] != 0u) {
                if (!zragf_inflate_gzip_meta_limit_ok((pos - header_start) + 1u))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                pos++;
            }
            if (pos >= st->base.in_size) {
                if (!zragf_inflate_gzip_meta_limit_ok(st->base.in_size - header_start))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            }
            name_len = pos - name_start;
            pos++;
            if (!zragf_inflate_gzip_meta_limit_ok(pos - header_start))
                return ZRAGF_INF_RUN_STEP_ERROR;
        }
        if ((flg & 0x10u) != 0u) {
            comment_start = pos;
            while (pos < st->base.in_size && buf[pos] != 0u) {
                if (!zragf_inflate_gzip_meta_limit_ok((pos - header_start) + 1u))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                pos++;
            }
            if (pos >= st->base.in_size) {
                if (!zragf_inflate_gzip_meta_limit_ok(st->base.in_size - header_start))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            }
            comment_len = pos - comment_start;
            pos++;
            if (!zragf_inflate_gzip_meta_limit_ok(pos - header_start))
                return ZRAGF_INF_RUN_STEP_ERROR;
        }
        if ((flg & 0x02u) != 0u) {
            zragf_u16 stored_hcrc;
            if (st->base.in_size - pos < 2u) {
                if (!zragf_inflate_gzip_meta_limit_ok(st->base.in_size - header_start))
                    return ZRAGF_INF_RUN_STEP_ERROR;
                return ZRAGF_INF_RUN_STEP_NEED_INPUT;
            }
            if (!zragf_inflate_gzip_meta_limit_ok((pos - header_start) + 2u))
                return ZRAGF_INF_RUN_STEP_ERROR;
            hdr_crc = zragf_crc32_update_running(0u, buf + header_start, pos - header_start);
            stored_hcrc = (zragf_u16)buf[pos] | (zragf_u16)((zragf_u16)buf[pos + 1u] << 8);
            if (st->base.validate_checks && ((zragf_u16)hdr_crc) != stored_hcrc)
                return ZRAGF_INF_RUN_STEP_ERROR;
            pos += 2u;
        }

        if (hout) {
            hout->text = (flg & 0x01u) ? 1 : 0;
            hout->time = zragf_read_u32_le(buf + header_start + 4u);
            hout->xflags = buf[header_start + 8u];
            hout->os = buf[header_start + 9u];
            hout->hcrc = (flg & 0x02u) ? 1 : 0;
            if ((flg & 0x04u) != 0u) {
                hout->extra_len = xlen;
                if (hout->extra && hout->extra_max > 0u) {
                    zragf_size_t copy_len = (zragf_size_t)xlen;
                    if (copy_len > (zragf_size_t)hout->extra_max)
                        copy_len = (zragf_size_t)hout->extra_max;
                    if (copy_len > 0u)
                        memcpy(hout->extra, buf + extra_data_pos, copy_len);
                }
            } else {
                hout->extra_len = 0u;
                if (hout->extra)
                    hout->extra = NULL;
            }
            if ((flg & 0x08u) != 0u)
                zragf_fill_gzip_string_field(&hout->name, hout->name_max, buf + name_start, name_len);
            else if (hout->name)
                hout->name = NULL;
            if ((flg & 0x10u) != 0u)
                zragf_fill_gzip_string_field(&hout->comment, hout->comm_max, buf + comment_start, comment_len);
            else if (hout->comment)
                hout->comment = NULL;
            hout->done = 1;
        }
    }

    st->base.saw_header = 1;
    zragf_inf_run_consume_bytes(st, pos - st->in_pos);
    st->run_state = ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_parse_dynamic_tables(zragf_stream *strm,
                                                               zragf_inflate_state_run *st)
{
    int hlit;
    int hdist;
    int hclen;
    int cl_len[19];
    int lens[286 + 30];
    int litlen_len[ZRAGF_INF_MAX_LITLEN];
    int dist_len[ZRAGF_INF_MAX_DIST];
    zragf_idec_table cl_tab;
    int total;
    int i;
    int j;
    zragf_inf_run_step step;

    if (!st)
        return ZRAGF_INF_RUN_STEP_ERROR;

    memset(&cl_tab, 0, sizeof(cl_tab));
    memset(cl_len, 0, sizeof(cl_len));
    memset(lens, 0, sizeof(lens));
    memset(litlen_len, 0, sizeof(litlen_len));
    memset(dist_len, 0, sizeof(dist_len));

    step = zragf_inf_run_read_bits_nosnap(st, 5, &hlit);
    if (step != ZRAGF_INF_RUN_STEP_OK)
        return step;
    step = zragf_inf_run_read_bits_nosnap(st, 5, &hdist);
    if (step != ZRAGF_INF_RUN_STEP_OK)
        return step;
    step = zragf_inf_run_read_bits_nosnap(st, 4, &hclen);
    if (step != ZRAGF_INF_RUN_STEP_OK)
        return step;

    hlit += 257;
    hdist += 1;
    hclen += 4;

    if (hlit < 257 || hlit > 286 || hdist < 1 || hdist > 30)
        return ZRAGF_INF_RUN_STEP_ERROR;

    for (i = 0; i < hclen; ++i) {
        int len3 = 0;
        step = zragf_inf_run_read_bits_nosnap(st, 3, &len3);
        if (step != ZRAGF_INF_RUN_STEP_OK)
            return step;
        cl_len[zragf_inf_cl_order[i]] = len3;
    }

    if (!zragf_stream_inflate_build_table(strm, cl_len, 19, 7, &cl_tab))
        return ZRAGF_INF_RUN_STEP_ERROR;

    total = hlit + hdist;
    i = 0;
    while (i < total) {
        int sym = 0;
        step = zragf_inf_run_decode_symbol_nosnap(st, &cl_tab, &sym);
        if (step != ZRAGF_INF_RUN_STEP_OK) {
            zragf_stream_bridge_free(strm, cl_tab.table);
            return step;
        }
        if (sym <= 15) {
            lens[i++] = sym;
        } else if (sym == 16) {
            int repeat = 0;
            int prev;
            step = zragf_inf_run_read_bits_nosnap(st, 2, &repeat);
            if (step != ZRAGF_INF_RUN_STEP_OK) {
                zragf_stream_bridge_free(strm, cl_tab.table);
                return step;
            }
            if (i == 0) {
                zragf_stream_bridge_free(strm, cl_tab.table);
                return ZRAGF_INF_RUN_STEP_ERROR;
            }
            prev = lens[i - 1];
            repeat += 3;
            while (repeat-- > 0) {
                if (i >= total) {
                    zragf_stream_bridge_free(strm, cl_tab.table);
                    return ZRAGF_INF_RUN_STEP_ERROR;
                }
                lens[i++] = prev;
            }
        } else if (sym == 17) {
            int repeat = 0;
            step = zragf_inf_run_read_bits_nosnap(st, 3, &repeat);
            if (step != ZRAGF_INF_RUN_STEP_OK) {
                zragf_stream_bridge_free(strm, cl_tab.table);
                return step;
            }
            repeat += 3;
            while (repeat-- > 0) {
                if (i >= total) {
                    zragf_stream_bridge_free(strm, cl_tab.table);
                    return ZRAGF_INF_RUN_STEP_ERROR;
                }
                lens[i++] = 0;
            }
        } else if (sym == 18) {
            int repeat = 0;
            step = zragf_inf_run_read_bits_nosnap(st, 7, &repeat);
            if (step != ZRAGF_INF_RUN_STEP_OK) {
                zragf_stream_bridge_free(strm, cl_tab.table);
                return step;
            }
            repeat += 11;
            while (repeat-- > 0) {
                if (i >= total) {
                    zragf_stream_bridge_free(strm, cl_tab.table);
                    return ZRAGF_INF_RUN_STEP_ERROR;
                }
                lens[i++] = 0;
            }
        } else {
            zragf_stream_bridge_free(strm, cl_tab.table);
            return ZRAGF_INF_RUN_STEP_ERROR;
        }
    }

    zragf_stream_bridge_free(strm, cl_tab.table);
    cl_tab.table = NULL;

    for (i = 0; i < hlit; ++i)
        litlen_len[i] = lens[i];
    for (j = 0; j < hdist; ++j)
        dist_len[j] = lens[hlit + j];

    if (litlen_len[256] == 0)
        return ZRAGF_INF_RUN_STEP_ERROR;

    if (!zragf_inflate_prepare_dist_lengths(dist_len,
                                            ZRAGF_INF_MAX_DIST,
                                            &st->dist_table_dummy))
        return ZRAGF_INF_RUN_STEP_ERROR;

    zragf_inf_run_free_tables(strm, st);
    if (!zragf_stream_inflate_build_table(strm, litlen_len,
                                           ZRAGF_INF_MAX_LITLEN,
                                           9,
                                           &st->huff.litlen))
        return ZRAGF_INF_RUN_STEP_ERROR;

    if (!zragf_stream_inflate_build_table(strm, dist_len,
                                           ZRAGF_INF_MAX_DIST,
                                           5,
                                           &st->huff.dist)) {
        zragf_inf_run_free_tables(strm, st);
        return ZRAGF_INF_RUN_STEP_ERROR;
    }

    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_process_block_header(zragf_stream *strm,
                                                               zragf_inflate_state_run *st)
{
    zragf_inf_run_mark mark;
    zragf_inf_run_step step;
    int bfinal = 0;
    int btype = 0;

    if (!st)
        return ZRAGF_INF_RUN_STEP_ERROR;

    zragf_inf_run_mark_save(st, &mark);
    step = zragf_inf_run_read_bits_nosnap(st, 1, &bfinal);
    if (step != ZRAGF_INF_RUN_STEP_OK) {
        zragf_inf_run_mark_restore(st, &mark);
        return step;
    }
    step = zragf_inf_run_read_bits_nosnap(st, 2, &btype);
    if (step != ZRAGF_INF_RUN_STEP_OK) {
        zragf_inf_run_mark_restore(st, &mark);
        return step;
    }

    st->block_final = bfinal & 1;
    st->base.hit_final_block = st->block_final;

    if (btype == 0) {
        zragf_u32 len;
        zragf_u32 nlen;
        zragf_inf_run_align_byte(st);
        if (zragf_inf_run_bytes_available(st) < 4u) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        }
        len = (zragf_u32)st->base.in_buf[st->in_pos]
            | ((zragf_u32)st->base.in_buf[st->in_pos + 1u] << 8);
        nlen = (zragf_u32)st->base.in_buf[st->in_pos + 2u]
             | ((zragf_u32)st->base.in_buf[st->in_pos + 3u] << 8);
        if ((len ^ nlen) != 0xFFFFu)
            return ZRAGF_INF_RUN_STEP_ERROR;
        st->dist_table_dummy = 0;
        zragf_inf_run_consume_bytes(st, 4u);
        st->copy_remaining = (int)len;
        st->copy_distance = 0;
        if (st->copy_remaining == 0) {
            st->run_state = st->block_final ? ZRAGF_INF_RUN_STATE_TRAILER
                                            : ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
            if (st->run_state == ZRAGF_INF_RUN_STATE_TRAILER)
                zragf_inf_run_align_byte(st);
        } else {
            st->run_state = ZRAGF_INF_RUN_STATE_STORED_COPY;
        }
        return ZRAGF_INF_RUN_STEP_OK;
    }

    if (btype == 1) {
        zragf_inf_run_free_tables(strm, st);
        if (!zragf_stream_inflate_init_fixed(strm, &st->huff)) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_ERROR;
        }
        st->dist_table_dummy = 0;
        st->run_state = ZRAGF_INF_RUN_STATE_COMPRESSED;
        return ZRAGF_INF_RUN_STEP_OK;
    }

    if (btype == 2) {
        step = zragf_inf_run_parse_dynamic_tables(strm, st);
        if (step != ZRAGF_INF_RUN_STEP_OK) {
            if (step == ZRAGF_INF_RUN_STEP_NEED_INPUT)
                zragf_inf_run_mark_restore(st, &mark);
            return step;
        }
        st->run_state = ZRAGF_INF_RUN_STATE_COMPRESSED;
        return ZRAGF_INF_RUN_STEP_OK;
    }

    return ZRAGF_INF_RUN_STEP_ERROR;
}

static zragf_inf_run_step zragf_inf_run_copy_stored(zragf_stream *strm,
                                                    zragf_inflate_state_run *st)
{
    while (st->copy_remaining > 0) {
        zragf_u8 byte;
        zragf_inf_run_step step;

        if (zragf_inf_run_bytes_available(st) == 0u)
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        if (!strm->next_out || strm->avail_out == 0u)
            return ZRAGF_INF_RUN_STEP_NEED_OUTPUT;

        byte = st->base.in_buf[st->in_pos];
        zragf_inf_run_consume_bytes(st, 1u);
        step = zragf_inf_run_emit_byte(strm, st, byte);
        if (step != ZRAGF_INF_RUN_STEP_OK)
            return step;
        st->copy_remaining--;
    }

    st->run_state = st->block_final ? ZRAGF_INF_RUN_STATE_TRAILER
                                    : ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
    if (st->run_state == ZRAGF_INF_RUN_STATE_TRAILER)
        zragf_inf_run_align_byte(st);
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_copy_match(zragf_stream *strm,
                                                   zragf_inflate_state_run *st)
{
    while (st->copy_remaining > 0) {
        int ref_pos;
        zragf_u8 byte;
        zragf_inf_run_step step;

        if (!strm->next_out || strm->avail_out == 0u)
            return ZRAGF_INF_RUN_STEP_NEED_OUTPUT;
        ref_pos = (st->base.window_pos - st->copy_distance + st->base.window_size)
                % st->base.window_size;
        byte = st->base.window[ref_pos];
        step = zragf_inf_run_emit_byte(strm, st, byte);
        if (step != ZRAGF_INF_RUN_STEP_OK)
            return step;
        st->copy_remaining--;
    }

    st->run_state = ZRAGF_INF_RUN_STATE_COMPRESSED;
    return ZRAGF_INF_RUN_STEP_OK;
}

static zragf_inf_run_step zragf_inf_run_process_compressed(zragf_stream *strm,
                                                           zragf_inflate_state_run *st)
{
    zragf_inf_run_mark mark;
    zragf_inf_run_step step;
    int sym = 0;

    if (!strm || !st)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if (!st->huff.litlen.table || !st->huff.dist.table)
        return ZRAGF_INF_RUN_STEP_ERROR;

    /* Do not require output space before reading the next symbol.
       End-of-block (256) can be consumed without producing output, and
       RFC 1951 permits a following empty final block plus wrapper trailer.
       Requiring avail_out here incorrectly rejects streams whose exact
       uncompressed size fills the caller buffer before EOB is consumed. */
    zragf_inf_run_mark_save(st, &mark);
    step = zragf_inf_run_decode_symbol_nosnap(st, &st->huff.litlen, &sym);
    if (step != ZRAGF_INF_RUN_STEP_OK) {
        zragf_inf_run_mark_restore(st, &mark);
        return step;
    }

    if (sym < 256) {
        if (!strm->next_out || strm->avail_out == 0u) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_NEED_OUTPUT;
        }
        step = zragf_inf_run_emit_byte(strm, st, (zragf_u8)sym);
        if (step != ZRAGF_INF_RUN_STEP_OK) {
            zragf_inf_run_mark_restore(st, &mark);
            return step;
        }
        return ZRAGF_INF_RUN_STEP_OK;
    }

    if (sym == 256) {
        zragf_inf_run_free_tables(strm, st);
        st->run_state = st->block_final ? ZRAGF_INF_RUN_STATE_TRAILER
                                        : ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
        if (st->run_state == ZRAGF_INF_RUN_STATE_TRAILER)
            zragf_inf_run_align_byte(st);
        return ZRAGF_INF_RUN_STEP_OK;
    }

    if (sym >= 257 && sym <= 285) {
        int length;
        int extra_bits;
        int extra = 0;
        int dist_sym = 0;
        int dist;

        if (!zragf_inflate_length_symbol_info(sym, &length, &extra_bits)) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_ERROR;
        }

        if (extra_bits > 0) {
            step = zragf_inf_run_read_bits_nosnap(st, extra_bits, &extra);
            if (step != ZRAGF_INF_RUN_STEP_OK) {
                zragf_inf_run_mark_restore(st, &mark);
                return step;
            }
            length += extra;
        }

        step = zragf_inf_run_decode_symbol_nosnap(st, &st->huff.dist, &dist_sym);
        if (step != ZRAGF_INF_RUN_STEP_OK) {
            zragf_inf_run_mark_restore(st, &mark);
            return step;
        }
        if (!zragf_inflate_distance_symbol_info(dist_sym, &dist, &extra_bits)) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_ERROR;
        }

        if (extra_bits > 0) {
            step = zragf_inf_run_read_bits_nosnap(st, extra_bits, &extra);
            if (step != ZRAGF_INF_RUN_STEP_OK) {
                zragf_inf_run_mark_restore(st, &mark);
                return step;
            }
            dist += extra;
        }

        if (st->dist_table_dummy || !zragf_inflate_distance_ok(st->base.window_filled,
                                                               st->base.max_window_size,
                                                               dist)) {
            zragf_inf_run_mark_restore(st, &mark);
            return ZRAGF_INF_RUN_STEP_ERROR;
        }

        st->copy_distance = dist;
        st->copy_remaining = length;
        st->run_state = ZRAGF_INF_RUN_STATE_MATCH_COPY;
        return ZRAGF_INF_RUN_STEP_OK;
    }

    zragf_inf_run_mark_restore(st, &mark);
    return ZRAGF_INF_RUN_STEP_ERROR;
}

static zragf_inf_run_step zragf_inf_run_parse_trailer(zragf_inflate_state_run *st)
{
    zragf_size_t avail;
    const zragf_u8 *p;

    if (!st)
        return ZRAGF_INF_RUN_STEP_ERROR;

    zragf_inf_run_align_byte(st);

    if (st->base.format == ZRAGF_INTERNAL_FMT_DEFLATE_RAW) {
        st->run_state = ZRAGF_INF_RUN_STATE_DONE;
        return ZRAGF_INF_RUN_STEP_STREAM_END;
    }

    avail = zragf_inf_run_bytes_available(st);
    p = st->base.in_buf + st->in_pos;

    if (st->base.format == ZRAGF_INTERNAL_FMT_ZLIB) {
        zragf_u32 want_adler;
        if (avail < 4u)
            return ZRAGF_INF_RUN_STEP_NEED_INPUT;
        want_adler = zragf_read_u32_be(p);
        if (st->base.validate_checks && !st->base.sync_skip_checks && want_adler != st->running_adler)
            return ZRAGF_INF_RUN_STEP_ERROR;
        zragf_inf_run_consume_bytes(st, 4u);
        st->run_state = ZRAGF_INF_RUN_STATE_DONE;
        return ZRAGF_INF_RUN_STEP_STREAM_END;
    }

    if (avail < 8u)
        return ZRAGF_INF_RUN_STEP_NEED_INPUT;
    if (st->base.validate_checks && !st->base.sync_skip_checks && zragf_read_u32_le(p) != st->running_crc)
        return ZRAGF_INF_RUN_STEP_ERROR;
    if (st->base.validate_checks && !st->base.sync_skip_checks && zragf_read_u32_le(p + 4u) != st->output_isize)
        return ZRAGF_INF_RUN_STEP_ERROR;
    zragf_inf_run_consume_bytes(st, 8u);
    st->run_state = ZRAGF_INF_RUN_STATE_DONE;
    return ZRAGF_INF_RUN_STEP_STREAM_END;
}

static zragf_inf_run_step zragf_inf_run_step_once(zragf_stream *strm,
                                                  zragf_inflate_state_run *st)
{
    if (!st)
        return ZRAGF_INF_RUN_STEP_ERROR;

    switch (st->run_state) {
        case ZRAGF_INF_RUN_STATE_HEADER:
            return zragf_inf_run_parse_header(st);
        case ZRAGF_INF_RUN_STATE_BLOCK_HEADER:
            return zragf_inf_run_process_block_header(strm, st);
        case ZRAGF_INF_RUN_STATE_STORED_COPY:
            return zragf_inf_run_copy_stored(strm, st);
        case ZRAGF_INF_RUN_STATE_COMPRESSED:
            return zragf_inf_run_process_compressed(strm, st);
        case ZRAGF_INF_RUN_STATE_MATCH_COPY:
            return zragf_inf_run_copy_match(strm, st);
        case ZRAGF_INF_RUN_STATE_TRAILER:
            return zragf_inf_run_parse_trailer(st);
        case ZRAGF_INF_RUN_STATE_DONE:
            return ZRAGF_INF_RUN_STEP_STREAM_END;
        case ZRAGF_INF_RUN_STATE_ERROR:
        default:
            return ZRAGF_INF_RUN_STEP_ERROR;
    }
}

static void zragf_inf_run_reset_state(zragf_stream *strm,
                                      zragf_inflate_state_run *st)
{
    if (!st)
        return;
    zragf_inf_run_free_tables(strm, st);
    zragf_inflate_reset_metadata(strm, st);
    st->base.format = st->base.init_format;
    st->run_state = ZRAGF_INF_RUN_STATE_HEADER;
    st->in_pos = 0u;
    st->total_in_consumed = 0u;
    st->running_adler = 1u;
    st->running_crc = 0u;
    st->output_isize = 0u;
    st->block_final = 0;
    st->copy_distance = 0;
    st->copy_remaining = 0;
    st->dist_table_dummy = 0;
    st->base.in_size = 0u;
    st->base.bitbuf = 0u;
    st->base.bitcount = 0;
    st->base.window_pos = 0;
    st->base.window_filled = 0u;
    st->base.hit_final_block = 0;
    st->base.sync_skip_checks = 0;
    if (st->base.window)
        memset(st->base.window, 0, (size_t)st->base.window_size);
    zragf_stream_reset_public_fields(strm);
    zragf_stream_set_checksum(strm,
                              st->base.format,
                              st->running_adler,
                              st->running_crc);
}

int zragf_inflateInit(zragf_stream *strm)
{
    return zragf_inflateInit2(strm, 15);
}

int zragf_inflateInit2(zragf_stream *strm, int windowBits)
{
    zragf_internal_format fmt;
    zragf_inflate_state_run *st;
    int max_window_bits = 0;

    if (!strm)
        return ZRAGF_STREAM_ERROR;
    if (!zragf_inflate_window_bits_parse(windowBits, &fmt, &max_window_bits))
        return ZRAGF_STREAM_ERROR;

    zragf_stream_prepare_allocators(strm);

    st = (zragf_inflate_state_run *)zragf_stream_alloc_bytes(strm, sizeof(*st));
    if (!st)
        return ZRAGF_MEM_ERROR;

    memset(st, 0, sizeof(*st));
    st->base.validate_checks = 1;
    st->base.kind = ZRAGF_STATE_KIND_Z_INFLATE;
    st->base.format = fmt;
    st->base.init_format = fmt;
    st->base.windowBits = windowBits;
    st->base.requested_windowBits = windowBits;
    st->base.max_window_bits = max_window_bits;
    st->base.max_window_size = (max_window_bits == 0) ? (1 << 15) : (1 << max_window_bits);
    st->base.window_size = 1 << 15;
    st->base.expected_dict_adler = 1u;
    st->base.window = (zragf_u8 *)zragf_stream_alloc_bytes(strm, (zragf_size_t)st->base.window_size);
    if (!st->base.window) {
        zragf_stream_free_bytes(strm, st);
        return ZRAGF_MEM_ERROR;
    }

    strm->state = st;
    zragf_inf_run_reset_state(strm, st);
    return ZRAGF_OK;
}

int zragf_inflateReset(zragf_stream *strm)
{
    zragf_inflate_state_run *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind == ZRAGF_STATE_KIND_NATIVE_INFLATE) {
        zragf_native_inflate_state *nst = (zragf_native_inflate_state *)strm->state;
        nst->in_size = 0u;
        zragf_stream_reset_public_fields(strm);
        return ZRAGF_OK;
    }
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;

    zragf_inf_run_reset_state(strm, st);
    return ZRAGF_OK;
}

int zragf_inflateReset2(zragf_stream *strm, int windowBits)
{
    zragf_inflate_state_run *st;
    zragf_internal_format fmt;
    int max_window_bits = 0;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;
    if (!zragf_inflate_window_bits_parse(windowBits, &fmt, &max_window_bits))
        return ZRAGF_STREAM_ERROR;

    st->base.init_format = fmt;
    st->base.windowBits = windowBits;
    st->base.requested_windowBits = windowBits;
    st->base.max_window_bits = max_window_bits;
    st->base.max_window_size = (max_window_bits == 0) ? (1 << 15) : (1 << max_window_bits);
    zragf_inf_run_reset_state(strm, st);
    return ZRAGF_OK;
}

int zragf_inflateCopy(zragf_stream *dest, zragf_stream *source)
{
    zragf_inflate_state_run *src;
    zragf_inflate_state_run *dst;

    if (!dest || !source || !source->state)
        return ZRAGF_STREAM_ERROR;
    if (dest->state)
        return ZRAGF_STREAM_ERROR;
    src = (zragf_inflate_state_run *)source->state;
    if (src->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;

    *dest = *source;
    zragf_stream_prepare_allocators(dest);
    dst = (zragf_inflate_state_run *)zragf_stream_alloc_bytes(dest, sizeof(*dst));
    if (!dst)
        return ZRAGF_MEM_ERROR;
    *dst = *src;
    memset(&dst->huff, 0, sizeof(dst->huff));
    dst->base.in_buf = NULL;
    dst->base.window = NULL;
    dst->base.dict_buf = NULL;

    if (!zragf_stream_clone_buffer(dest, &dst->base.in_buf, src->base.in_buf, src->base.in_cap, src->base.in_size) ||
        !zragf_stream_clone_buffer(dest, &dst->base.window, src->base.window, (zragf_size_t)((src->base.window_size > 0) ? src->base.window_size : 0), (zragf_size_t)((src->base.window_size > 0) ? src->base.window_size : 0)) ||
        !zragf_stream_clone_buffer(dest, &dst->base.dict_buf, src->base.dict_buf, src->base.dict_len, src->base.dict_len) ||
        !zragf_clone_idec_table(dest, &dst->huff.litlen, &src->huff.litlen) ||
        !zragf_clone_idec_table(dest, &dst->huff.dist, &src->huff.dist)) {
        zragf_inflate_free(&dst->huff);
        zragf_stream_free_bytes(dest, dst->base.in_buf);
        zragf_stream_free_bytes(dest, dst->base.window);
        zragf_stream_free_bytes(dest, dst->base.dict_buf);
        zragf_stream_free_bytes(dest, dst);
        dest->state = NULL;
        return ZRAGF_MEM_ERROR;
    }

    dest->state = dst;
    return ZRAGF_OK;
}

int zragf_inflatePrime(zragf_stream *strm, int bits, int value)
{
    zragf_inflate_state_run *st;
    unsigned mask;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->base.init_format != ZRAGF_INTERNAL_FMT_DEFLATE_RAW)
        return ZRAGF_STREAM_ERROR;
    if (st->total_in_consumed != 0u || st->base.in_size != 0u || st->run_state != ZRAGF_INF_RUN_STATE_HEADER)
        return ZRAGF_STREAM_ERROR;
    if (bits < 0) {
        st->base.bitbuf = 0u;
        st->base.bitcount = 0;
        return ZRAGF_OK;
    }
    if (bits > 16)
        return ZRAGF_STREAM_ERROR;
    if (st->base.bitcount + bits > 32)
        return ZRAGF_STREAM_ERROR;
    mask = (bits == 32) ? 0xFFFFFFFFu : ((bits == 0) ? 0u : ((1u << bits) - 1u));
    st->base.bitbuf |= ((zragf_u32)((unsigned)value & mask)) << st->base.bitcount;
    st->base.bitcount += bits;
    return ZRAGF_OK;
}

int zragf_inflateValidate(zragf_stream *strm, int check)
{
    zragf_inflate_state_run *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;

    st->base.validate_checks = check ? 1 : 0;
    return ZRAGF_OK;
}

int zragf_inflateSync(zragf_stream *strm)
{
    zragf_inflate_state_run *st;
    zragf_size_t start;
    zragf_size_t limit;
    zragf_size_t i;
    zragf_u32 total_before;
    zragf_size_t pos_before;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_inflate_state_run *)strm->state;

    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;

    total_before = st->total_in_consumed;
    pos_before = st->in_pos;

    zragf_inf_run_align_byte(st);
    start = st->in_pos;
    limit = st->base.in_size;
    if (start >= limit)
        return ZRAGF_BUF_ERROR;

    for (i = start; i + 3u < limit; ++i) {
        if (st->base.in_buf[i] == 0x00u && st->base.in_buf[i + 1u] == 0x00u &&
            st->base.in_buf[i + 2u] == 0xFFu && st->base.in_buf[i + 3u] == 0xFFu) {
            zragf_inf_run_free_tables(strm, st);
            st->in_pos = i + 4u;
            st->base.bitbuf = 0u;
            st->base.bitcount = 0;
            st->run_state = ZRAGF_INF_RUN_STATE_BLOCK_HEADER;
            st->base.need_dict = 0;
            st->base.hit_final_block = 0;
            st->copy_distance = 0;
            st->copy_remaining = 0;
            st->dist_table_dummy = 0;
            st->base.sync_skip_checks = 1;
            zragf_window_load_dictionary(st->base.window,
                                         st->base.window_size,
                                         &st->base.window_pos,
                                         NULL,
                                         0u);
            st->base.window_filled = 0u;
            st->total_in_consumed = total_before + (zragf_u32)(st->in_pos - pos_before);
            strm->total_in = st->total_in_consumed;
            zragf_inf_run_compact_input(st);
            return ZRAGF_OK;
        }
    }

    if (limit - start <= 3u) {
        st->in_pos = start;
        return ZRAGF_DATA_ERROR;
    }

    st->in_pos = limit - 3u;
    st->total_in_consumed = total_before + (zragf_u32)(st->in_pos - pos_before);
    strm->total_in = st->total_in_consumed;
    zragf_inf_run_compact_input(st);
    return ZRAGF_DATA_ERROR;
}

int zragf_inflateSetDictionary(zragf_stream *strm, const zragf_u8 *dictionary, unsigned int dictLength)
{
    zragf_inflate_state_run *st;
    zragf_u8 *copy = NULL;
    zragf_size_t keep;
    zragf_u32 adler;

    if (!strm || !strm->state || (!dictionary && dictLength != 0u))
        return ZRAGF_STREAM_ERROR;
    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->base.format == ZRAGF_INTERNAL_FMT_GZIP)
        return ZRAGF_STREAM_ERROR;
    if (st->base.format != ZRAGF_INTERNAL_FMT_DEFLATE_RAW && !st->base.need_dict)
        return ZRAGF_STREAM_ERROR;

    adler = zragf_adler32_update(1u, dictionary, (zragf_size_t)dictLength);
    if (st->base.format != ZRAGF_INTERNAL_FMT_DEFLATE_RAW && adler != st->base.expected_dict_adler)
        return ZRAGF_DATA_ERROR;

    zragf_stream_free_bytes(strm, st->base.dict_buf);
    st->base.dict_buf = NULL;
    st->base.dict_len = 0u;

    keep = (zragf_size_t)dictLength;
    if (keep > (zragf_size_t)st->base.window_size)
        keep = (zragf_size_t)st->base.window_size;
    if (keep > 0u) {
        copy = (zragf_u8 *)zragf_stream_alloc_bytes(strm, keep);
        if (!copy)
            return ZRAGF_MEM_ERROR;
        memcpy(copy, dictionary + ((zragf_size_t)dictLength - keep), keep);
        st->base.dict_buf = copy;
        st->base.dict_len = keep;
        zragf_window_load_dictionary(st->base.window,
                                     st->base.window_size,
                                     &st->base.window_pos,
                                     st->base.dict_buf,
                                     st->base.dict_len);
        st->base.window_filled = keep;
    } else {
        zragf_window_load_dictionary(st->base.window,
                                     st->base.window_size,
                                     &st->base.window_pos,
                                     NULL,
                                     0u);
        st->base.window_filled = 0u;
    }

    st->base.need_dict = 0;
    st->base.expected_dict_adler = 1u;
    zragf_stream_set_checksum(strm, st->base.format, st->running_adler, st->running_crc);
    return ZRAGF_OK;
}

int zragf_inflateGetHeader(zragf_stream *strm, zragf_gz_headerp head)
{
    zragf_inflate_state_run *st;

    if (!strm || !strm->state || !head)
        return ZRAGF_STREAM_ERROR;
    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->base.saw_header || st->total_in_consumed != 0u)
        return ZRAGF_STREAM_ERROR;

    head->done = 0;
    st->base.gzip_header_out = head;
    return ZRAGF_OK;
}

int zragf_inflateEndZ(zragf_stream *strm)
{
    zragf_inflate_state_run *st;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;

    st = (zragf_inflate_state_run *)strm->state;
    if (((zragf_native_inflate_state *)strm->state)->kind == ZRAGF_STATE_KIND_NATIVE_INFLATE)
        return (zragf_inflate_end(strm) == ZRAGF_ST_OK) ? ZRAGF_OK : ZRAGF_STREAM_ERROR;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;

    zragf_inf_run_free_tables(strm, st);
    zragf_stream_free_bytes(strm, st->base.dict_buf);
    zragf_stream_free_bytes(strm, st->base.in_buf);
    zragf_stream_free_bytes(strm, st->base.window);
    zragf_stream_free_bytes(strm, st);
    strm->state = NULL;
    return ZRAGF_OK;
}

int zragf_inflateZ(zragf_stream *strm, int flush)
{
    zragf_inflate_state_run *st;
    zragf_status rc;
    zragf_u32 total_in_before;
    zragf_u32 total_out_before;
    zragf_size_t appended = 0u;

    if (!strm || !strm->state)
        return ZRAGF_STREAM_ERROR;
    if (flush < ZRAGF_NO_FLUSH || flush > ZRAGF_FINISH)
        return ZRAGF_STREAM_ERROR;

    if (((zragf_native_inflate_state *)strm->state)->kind == ZRAGF_STATE_KIND_NATIVE_INFLATE) {
        zragf_status nrc = zragf_inflate(strm, (zragf_flush)flush);
        if (nrc == ZRAGF_ST_OK)
            return ZRAGF_OK;
        if (nrc == ZRAGF_ST_OUTPUT_TOO_SMALL)
            return ZRAGF_BUF_ERROR;
        return ZRAGF_DATA_ERROR;
    }

    st = (zragf_inflate_state_run *)strm->state;
    if (st->base.kind != ZRAGF_STATE_KIND_Z_INFLATE)
        return ZRAGF_STREAM_ERROR;
    if (st->run_state == ZRAGF_INF_RUN_STATE_DONE) {
        zragf_stream_set_checksum(strm, st->base.format, st->running_adler, st->running_crc);
        return ZRAGF_STREAM_END;
    }
    if (st->run_state == ZRAGF_INF_RUN_STATE_ERROR)
        return ZRAGF_DATA_ERROR;
    if (st->base.need_dict) {
        strm->adler = st->base.expected_dict_adler;
        return ZRAGF_NEED_DICT;
    }

    total_in_before = st->total_in_consumed;
    total_out_before = strm->total_out;
    strm->msg = NULL;
    strm->data_type = ZRAGF_Z_UNKNOWN;

    if (strm->avail_in > 0u && strm->next_in) {
        appended = strm->avail_in;
        rc = zragf_inf_run_append_input(strm, st, strm->next_in, strm->avail_in);
        if (rc != ZRAGF_ST_OK)
            return ZRAGF_MEM_ERROR;
        strm->next_in += appended;
        strm->avail_in = 0u;
    }

    for (;;) {
        zragf_inf_run_step step = zragf_inf_run_step_once(strm, st);
        zragf_u32 consumed = st->total_in_consumed - total_in_before;
        zragf_u32 produced = strm->total_out - total_out_before;

        strm->total_in = st->total_in_consumed;
        zragf_inf_run_compact_input(st);
        zragf_stream_set_checksum(strm, st->base.format, st->running_adler, st->running_crc);

        if (step == ZRAGF_INF_RUN_STEP_OK)
            continue;
        if (step == ZRAGF_INF_RUN_STEP_STREAM_END)
            return ZRAGF_STREAM_END;
        if (step == ZRAGF_INF_RUN_STEP_NEED_DICT) {
            strm->adler = st->base.expected_dict_adler;
            return ZRAGF_NEED_DICT;
        }
        if (step == ZRAGF_INF_RUN_STEP_NEED_INPUT) {
            if (produced > 0u || consumed > 0u || appended > 0u)
                return ZRAGF_OK;
            if (flush == ZRAGF_FINISH)
                return ZRAGF_BUF_ERROR;
            return ZRAGF_OK;
        }
        if (step == ZRAGF_INF_RUN_STEP_NEED_OUTPUT) {
            if (produced > 0u || consumed > 0u || appended > 0u)
                return ZRAGF_OK;
            return ZRAGF_BUF_ERROR;
        }
        st->run_state = ZRAGF_INF_RUN_STATE_ERROR;
        return ZRAGF_DATA_ERROR;
    }
}
