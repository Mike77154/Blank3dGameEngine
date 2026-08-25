/* zragf_compress.c - implementación de zragf_compress */

#include "zragflib_internal.h"
#include "zragf_frame.h"
#include "zragf_lz.h"
#include "zragf_bitio.h"
#include "zragf_huff.h"
#include "zragf_crc32.h"

static zragf_status
zragf_encode_lz_to_buffer(const zragf_u8 *in,
                          zragf_u32 in_size,
                          zragf_u8 *lz_buf,
                          zragf_size_t lz_cap,
                          zragf_size_t *lz_size_out,
                          zragf_level level)

{
    zragf_u32 pos = 0u;
    zragf_u32 max_window;
    zragf_u32 max_len;
    zragf_size_t out_pos = 0u;
    zragf_u32 literal_run_len   = 0;
    zragf_u32 literal_run_start = 0;

    if (!in || !lz_buf || !lz_size_out)
        return ZRAGF_ERR_NULL_POINTER;

    switch (level) {
    case ZRAGF_LEVEL_FAST:
        max_window = 2048u;
        max_len    = 32u;
        break;
    case ZRAGF_LEVEL_MAX:
        max_window = ZRAGF_MAX_WINDOW;
        max_len    = ZRAGF_MAX_MATCH;
        break;
    case ZRAGF_LEVEL_DEFAULT:
    default:
        max_window = 16384u;
        max_len    = 64u;
        break;
    }

    while (pos < in_size) {
        zragf_u32 offset = 0;
        zragf_u32 match_len = 0;

        match_len = zragf_find_match(in, pos, in_size,
                                     &offset, max_window, max_len);

        if (match_len >= ZRAGF_MIN_MATCH) {
            if (literal_run_len > 0) {
                zragf_u32 len    = literal_run_len;
                zragf_u32 litpos = literal_run_start;

                while (len > 0) {
                    zragf_u32 chunk = len;
                    zragf_u8  ctrl;

                    if (chunk > ZRAGF_MAX_LITERAL)
                        chunk = ZRAGF_MAX_LITERAL;

                    if (out_pos + 1u + chunk > lz_cap)
                        return ZRAGF_ERR_OUTPUT_TOO_SMALL;

                    ctrl = (zragf_u8)(chunk - 1u);
                    lz_buf[out_pos++] = ctrl;

                    {
                        zragf_u32 i;
                        for (i = 0; i < chunk; ++i)
                            lz_buf[out_pos++] = in[litpos + i];
                    }

                    litpos += chunk;
                    len    -= chunk;
                }

                literal_run_len = 0;
            }

            {
                zragf_u32 remaining = match_len;

                while (remaining > 0) {
                    zragf_u32 chunk = remaining;
                    zragf_u8  ctrl;
                    zragf_u16 off_minus_1;

                    if (chunk > ZRAGF_MAX_MATCH)
                        chunk = ZRAGF_MAX_MATCH;

                    if (out_pos + 3u > lz_cap)
                        return ZRAGF_ERR_OUTPUT_TOO_SMALL;

                    ctrl = (zragf_u8)(0x80u |
                           (zragf_u8)(chunk - ZRAGF_MIN_MATCH));
                    lz_buf[out_pos++] = ctrl;

                    off_minus_1 = (zragf_u16)(offset - 1u);
                    lz_buf[out_pos++] = (zragf_u8)(off_minus_1 & 0xFFu);
                    lz_buf[out_pos++] = (zragf_u8)((off_minus_1 >> 8) & 0xFFu);

                    remaining -= chunk;
                    pos       += chunk;
                }
            }
        } else {
            if (literal_run_len == 0)
                literal_run_start = pos;
            ++literal_run_len;
            ++pos;

            if (literal_run_len == ZRAGF_MAX_LITERAL) {
                zragf_u32 len    = literal_run_len;
                zragf_u32 litpos = literal_run_start;
                zragf_u8  ctrl;

                if (out_pos + 1u + len > lz_cap)
                    return ZRAGF_ERR_OUTPUT_TOO_SMALL;

                ctrl = (zragf_u8)(len - 1u);
                lz_buf[out_pos++] = ctrl;

                {
                    zragf_u32 i;
                    for (i = 0; i < len; ++i)
                        lz_buf[out_pos++] = in[litpos + i];
                }

                literal_run_len = 0;
            }
        }
    }

    if (literal_run_len > 0) {
        zragf_u32 len    = literal_run_len;
        zragf_u32 litpos = literal_run_start;
        zragf_u8  ctrl;

        if (out_pos + 1u + len > lz_cap)
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;

        ctrl = (zragf_u8)(len - 1u);
        lz_buf[out_pos++] = ctrl;

        {
            zragf_u32 i;
            for (i = 0; i < len; ++i)
                lz_buf[out_pos++] = in[litpos + i];
        }
    }

    *lz_size_out = out_pos;
    return ZRAGF_OK;
}

zragf_size_t zragf_compress_workspace_bound(zragf_size_t sourceLen)
{
    if (sourceLen > (((zragf_size_t)-1) - 16u) / 2u)
        return 0u;
    return sourceLen * 2u + 16u;
}

static zragf_status
zragf_compress_impl(const void *in_data,
                    zragf_size_t in_size,
                    void *out_data,
                    zragf_size_t *out_size,
                    void *workspace,
                    zragf_size_t workspace_size,
                    int workspace_required,
                    zragf_level level)
{
    const zragf_u8 *in;
    zragf_u8 *out;
    zragf_size_t out_cap;
    zragf_size_t out_pos;
    zragf_status st;
    zragf_u8 flags = ZRAGF_FLAG_NONE;

    if (!in_data || !out_data || !out_size)
        return ZRAGF_ERR_NULL_POINTER;

    in      = (const zragf_u8 *)in_data;
    out     = (zragf_u8 *)out_data;
    out_cap = *out_size;

    if (in_size == 0u) {
        zragf_u32 crc = zragf_crc32((const zragf_u8 *)in_data, 0u);

        flags |= ZRAGF_FLAG_CHECKSUM;

        st = zragf_frame_write_header(out, out_cap, 0u, flags, &out_pos);
        if (st != ZRAGF_OK)
            return st;

        if (out_pos + 4u > out_cap)
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;
        zragf_write_u32_le(out + out_pos, crc);
        out_pos += 4u;

        *out_size = out_pos;
        return ZRAGF_OK;
    }

    flags |= ZRAGF_FLAG_HUFFMAN;
    flags |= ZRAGF_FLAG_CHECKSUM;

    st = zragf_frame_write_header(out, out_cap,
                                  (zragf_u32)in_size,
                                  flags,
                                  &out_pos);
    if (st != ZRAGF_OK)
        return st;

    {
        zragf_size_t lz_cap;
        zragf_size_t lz_size;
        zragf_u8 *lz_buf;
        zragf_u8 *owned_buf;
        zragf_u32 freq[ZRAGF_HUFF_SYMS];
        zragf_huff_tables htab;
        zragf_u8 max_bits;
        zragf_size_t payload_cap;
        zragf_size_t payload_written;
        zragf_bitwriter bw;
        zragf_size_t i;

        lz_size = 0u;
        max_bits = 0u;
        payload_written = 0u;
        owned_buf = NULL;
        lz_cap = zragf_compress_workspace_bound(in_size);
        if (lz_cap == 0u)
            return ZRAGF_ST_INVALID_ARGUMENT;

        if (workspace && workspace_size >= lz_cap) {
            lz_buf = (zragf_u8 *)workspace;
        } else {
            if (workspace_required)
                return ZRAGF_ST_WORKSPACE_TOO_SMALL;
            lz_buf = (zragf_u8 *)zragf_alloc_default(NULL, 1u, lz_cap);
            if (!lz_buf)
                return ZRAGF_ST_NO_MEMORY;
            owned_buf = lz_buf;
        }

        st = zragf_encode_lz_to_buffer(in,
                                       (zragf_u32)in_size,
                                       lz_buf,
                                       lz_cap,
                                       &lz_size,
                                       level);
        if (st != ZRAGF_OK) {
            zragf_free_default(NULL, owned_buf);
            return st;
        }

        for (i = 0; i < ZRAGF_HUFF_SYMS; ++i)
            freq[i] = 0u;

        for (i = 0; i < lz_size; ++i)
            freq[lz_buf[i]]++;

        st = zragf_huff_build_from_freq(freq, &htab, &max_bits);
        if (st != ZRAGF_OK) {
            zragf_free_default(NULL, owned_buf);
            return st;
        }

        if (out_pos + 4u + 1u + ZRAGF_HUFF_SYMS > out_cap) {
            zragf_free_default(NULL, owned_buf);
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;
        }

        zragf_write_u32_le(out + out_pos, (zragf_u32)lz_size);
        out_pos += 4u;
        out[out_pos++] = max_bits;

        {
            zragf_u32 s;
            for (s = 0; s < ZRAGF_HUFF_SYMS; ++s)
                out[out_pos++] = htab.bits[s];
        }

        payload_cap = out_cap - out_pos;

        st = zragf_bw_init(&bw, out + out_pos, payload_cap);
        if (st != ZRAGF_OK) {
            zragf_free_default(NULL, owned_buf);
            return st;
        }

        for (i = 0; i < lz_size; ++i) {
            zragf_u8 sym = lz_buf[i];
            zragf_u16 code = htab.code[sym];
            zragf_u8 bits = htab.bits[sym];
            if (bits == 0u) {
                zragf_free_default(NULL, owned_buf);
                return ZRAGF_ERR_INTERNAL;
            }
            st = zragf_bw_put_bits(&bw, code, bits);
            if (st != ZRAGF_OK) {
                zragf_free_default(NULL, owned_buf);
                return st;
            }
        }

        st = zragf_bw_flush(&bw, &payload_written);
        zragf_free_default(NULL, owned_buf);
        if (st != ZRAGF_OK)
            return st;

        out_pos += payload_written;
    }

    {
        zragf_u32 crc = zragf_crc32(in, in_size);

        if (out_pos + 4u > out_cap)
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;

        zragf_write_u32_le(out + out_pos, crc);
        out_pos += 4u;
    }

    *out_size = out_pos;
    return ZRAGF_OK;
}

zragf_status
zragf_compress(const void *in_data,
               zragf_size_t in_size,
               void *out_data,
               zragf_size_t *out_size,
               zragf_level level)
{
    return zragf_compress_impl(in_data, in_size, out_data, out_size,
                               NULL, 0u, 0, level);
}

zragf_status
zragf_compress_with_workspace(const void *in_data,
                              zragf_size_t in_size,
                              void *out_data,
                              zragf_size_t *out_size,
                              void *workspace,
                              zragf_size_t workspace_size,
                              zragf_level level)
{
    return zragf_compress_impl(in_data, in_size, out_data, out_size,
                               workspace, workspace_size, 1, level);
}
