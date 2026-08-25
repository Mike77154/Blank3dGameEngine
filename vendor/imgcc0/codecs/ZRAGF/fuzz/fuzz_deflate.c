#include <stddef.h>
#include <string.h>

#include <zlib.h>
#ifdef ZRAGF_HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#include "zragflib.h"

#define FZ_MAX_PAYLOAD 4096u
#define FZ_MAX_OUTPUT  (FZ_MAX_PAYLOAD * 4u + 2048u)

static int fz_zlib_inflate(const unsigned char *src,
                           size_t src_len,
                           int window_bits,
                           const unsigned char *dict,
                           unsigned dict_len,
                           unsigned char *dst,
                           size_t dst_cap,
                           size_t *dst_len)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, window_bits) != Z_OK)
        return 0;
    if (dict_len > 0u && window_bits < 0) {
        if (inflateSetDictionary(&zs, dict, dict_len) != Z_OK) {
            inflateEnd(&zs);
            return 0;
        }
    }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_cap;
    rc = inflate(&zs, Z_FINISH);
    if (rc == Z_NEED_DICT) {
        if (dict_len == 0u || inflateSetDictionary(&zs, dict, dict_len) != Z_OK) {
            inflateEnd(&zs);
            return 0;
        }
        rc = inflate(&zs, Z_FINISH);
    }
    *dst_len = (size_t)zs.total_out;
    inflateEnd(&zs);
    return rc == Z_STREAM_END;
}

static int fz_zragf_inflate(const unsigned char *src,
                            size_t src_len,
                            int window_bits,
                            const unsigned char *dict,
                            unsigned dict_len,
                            unsigned char *dst,
                            size_t dst_cap,
                            size_t *dst_len)
{
    zragf_stream s;
    int rc;

    memset(&s, 0, sizeof(s));
    if (zragf_inflateInit2(&s, window_bits) != ZRAGF_OK)
        return 0;
    if (dict_len > 0u && window_bits < 0) {
        if (zragf_inflateSetDictionary(&s, dict, dict_len) != ZRAGF_OK) {
            zragf_inflateEndZ(&s);
            return 0;
        }
    }
    s.next_in = (zragf_u8 *)src;
    s.avail_in = src_len;
    s.next_out = dst;
    s.avail_out = dst_cap;
    rc = zragf_inflateZ(&s, ZRAGF_FINISH);
    if (rc == ZRAGF_NEED_DICT) {
        if (dict_len == 0u || zragf_inflateSetDictionary(&s, dict, dict_len) != ZRAGF_OK) {
            zragf_inflateEndZ(&s);
            return 0;
        }
        rc = zragf_inflateZ(&s, ZRAGF_FINISH);
    }
    *dst_len = (size_t)s.total_out;
    zragf_inflateEndZ(&s);
    return rc == ZRAGF_STREAM_END;
}

#ifdef ZRAGF_HAVE_LIBDEFLATE
static int fz_libdeflate_inflate(const unsigned char *src,
                                 size_t src_len,
                                 int fmt,
                                 unsigned char *dst,
                                 size_t dst_cap,
                                 size_t *dst_len)
{
    struct libdeflate_decompressor *d;
    enum libdeflate_result res;
    size_t in_used = 0u;
    size_t out_used = 0u;

    d = libdeflate_alloc_decompressor();
    if (!d)
        return 0;
    if (fmt == 0)
        res = libdeflate_deflate_decompress_ex(d, src, src_len, dst, dst_cap, &in_used, &out_used);
    else if (fmt == 1)
        res = libdeflate_zlib_decompress_ex(d, src, src_len, dst, dst_cap, &in_used, &out_used);
    else
        res = libdeflate_gzip_decompress_ex(d, src, src_len, dst, dst_cap, &in_used, &out_used);
    libdeflate_free_decompressor(d);
    *dst_len = out_used;
    return res == LIBDEFLATE_SUCCESS;
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    unsigned char payload[FZ_MAX_PAYLOAD];
    unsigned char dict[256];
    unsigned char comp[FZ_MAX_OUTPUT];
    unsigned char out_a[FZ_MAX_OUTPUT];
    unsigned char out_b[FZ_MAX_OUTPUT];
#ifdef ZRAGF_HAVE_LIBDEFLATE
    unsigned char out_c[FZ_MAX_OUTPUT];
#endif
    size_t payload_len;
    size_t dict_len;
    size_t out_a_len = 0u, out_b_len = 0u;
#ifdef ZRAGF_HAVE_LIBDEFLATE
    size_t out_c_len = 0u;
#endif
    zragf_stream s;
    size_t comp_len = 0u;
    size_t pos = 0u;
    int rc;
    int wb;
    int fmt;
    int level;
    int strategy;
    int use_dict;
    unsigned step = 0u;

    if (size < 3u)
        return 0;

    payload_len = size - 2u;
    if (payload_len > FZ_MAX_PAYLOAD)
        payload_len = FZ_MAX_PAYLOAD;
    memcpy(payload, data + 2u, payload_len);

    dict_len = (payload_len > sizeof(dict)) ? sizeof(dict) : payload_len;
    if (dict_len > 0u)
        memcpy(dict, payload, dict_len);

    switch (data[0] % 3u) {
        case 0: wb = -15; fmt = 0; break;
        case 1: wb = 15; fmt = 1; break;
        default: wb = 31; fmt = 2; break;
    }
    level = 1 + (int)(data[1] % 9u);
    switch ((data[0] >> 2) % 5u) {
        case 0: strategy = ZRAGF_Z_DEFAULT_STRATEGY; break;
        case 1: strategy = ZRAGF_Z_FILTERED; break;
        case 2: strategy = ZRAGF_Z_HUFFMAN_ONLY; break;
        case 3: strategy = ZRAGF_Z_RLE; break;
        default: strategy = ZRAGF_Z_FIXED; break;
    }
    use_dict = ((data[0] & 0x80u) != 0u && wb != 31 && dict_len > 0u) ? 1 : 0;

    memset(&s, 0, sizeof(s));
    if (zragf_deflateInit2(&s, level, 8, wb, 8, strategy) != ZRAGF_OK)
        return 0;
    if (use_dict)
        (void)zragf_deflateSetDictionary(&s, dict, (unsigned)dict_len);

    while (1) {
        size_t in_chunk = (pos < payload_len) ? (1u + ((payload[pos] + step * 9u) & 0x1Fu)) : 0u;
        int flush = ZRAGF_NO_FLUSH;
        if (in_chunk > payload_len - pos)
            in_chunk = payload_len - pos;
        s.next_in = payload + pos;
        s.avail_in = in_chunk;
        if (pos + in_chunk >= payload_len)
            flush = ZRAGF_FINISH;
        else if ((step & 7u) == 3u)
            flush = ZRAGF_SYNC_FLUSH;
        do {
            size_t out_space = sizeof(comp) - (size_t)s.total_out;
            size_t out_chunk = (out_space > 0u) ? (1u + ((step * 13u) & 0x3Fu)) : 0u;
            if (out_chunk > out_space)
                out_chunk = out_space;
            if (out_chunk == 0u) {
                zragf_deflateEndZ(&s);
                return 0;
            }
            s.next_out = comp + (size_t)s.total_out;
            s.avail_out = out_chunk;
            rc = zragf_deflateZ(&s, flush);
            if (rc == ZRAGF_STREAM_END) {
                comp_len = (size_t)s.total_out;
                zragf_deflateEndZ(&s);
                goto compressed;
            }
            if (rc < 0 && rc != ZRAGF_BUF_ERROR) {
                zragf_deflateEndZ(&s);
                return 0;
            }
        } while (s.avail_in > 0u || s.avail_out == 0u);
        pos += in_chunk;
        step++;
    }

compressed:
    if (!fz_zragf_inflate(comp, comp_len, wb,
                          use_dict ? dict : NULL,
                          use_dict ? (unsigned)dict_len : 0u,
                          out_a, sizeof(out_a), &out_a_len))
        return 0;
    if (out_a_len != payload_len || memcmp(out_a, payload, payload_len) != 0)
        return 0;

    if (!fz_zlib_inflate(comp, comp_len, wb,
                         use_dict ? dict : NULL,
                         use_dict ? (unsigned)dict_len : 0u,
                         out_b, sizeof(out_b), &out_b_len))
        return 0;
    if (out_b_len != payload_len || memcmp(out_b, payload, payload_len) != 0)
        return 0;

#ifdef ZRAGF_HAVE_LIBDEFLATE
    if (!use_dict) {
        if (!fz_libdeflate_inflate(comp, comp_len, fmt, out_c, sizeof(out_c), &out_c_len))
            return 0;
        if (out_c_len != payload_len || memcmp(out_c, payload, payload_len) != 0)
            return 0;
    }
#endif

    return 0;
}
