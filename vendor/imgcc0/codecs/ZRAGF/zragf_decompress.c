#include "zragflib_internal.h"
#include "zragf_frame.h"
#include "zragf_bitio.h"
#include "zragf_huff.h"
#include "zragf_crc32.h"

static zragf_status
zragf_decode_lz_stream(const zragf_u8 *in,
                       zragf_size_t in_size,
                       zragf_u8 *out,
                       zragf_size_t out_cap,
                       zragf_u32 expected_uncompressed,
                       zragf_size_t *out_pos_out)
{
    zragf_size_t in_pos;
    zragf_size_t out_pos;

    in_pos = 0u;
    out_pos = 0u;

    if (!in || !out || !out_pos_out)
        return ZRAGF_ERR_NULL_POINTER;

    while (in_pos < in_size && out_pos < out_cap) {
        zragf_u8 ctrl;
        zragf_u32 len;

        ctrl = in[in_pos++];
        if ((ctrl & 0x80u) == 0u) {
            len = (zragf_u32)(ctrl & 0x7Fu) + 1u;

            if (in_pos + len > in_size)
                return ZRAGF_ERR_CORRUPTED_DATA;
            if (out_pos + len > out_cap)
                return ZRAGF_ERR_OUTPUT_TOO_SMALL;

            {
                zragf_u32 i;
                for (i = 0; i < len; ++i)
                    out[out_pos++] = in[in_pos++];
            }
        } else {
            zragf_u32 match_len;
            zragf_u32 offset;
            zragf_u16 off_minus_1;

            match_len = (zragf_u32)(ctrl & 0x7Fu) + (zragf_u32)ZRAGF_MIN_MATCH;

            if (in_pos + 2u > in_size)
                return ZRAGF_ERR_CORRUPTED_DATA;

            off_minus_1 = (zragf_u16)(in[in_pos] |
                         ((zragf_u16)in[in_pos + 1] << 8));
            in_pos += 2u;

            offset = (zragf_u32)off_minus_1 + 1u;
            if (offset == 0u || offset > out_pos)
                return ZRAGF_ERR_CORRUPTED_DATA;
            if (out_pos + match_len > out_cap)
                return ZRAGF_ERR_OUTPUT_TOO_SMALL;

            {
                zragf_u32 i;
                zragf_size_t src_pos = out_pos - offset;
                for (i = 0; i < match_len; ++i) {
                    out[out_pos] = out[src_pos];
                    ++out_pos;
                    ++src_pos;
                }
            }
        }
    }

    if (out_pos != (zragf_size_t)expected_uncompressed) {
        if (out_pos < (zragf_size_t)expected_uncompressed)
            return ZRAGF_ERR_OUTPUT_TOO_SMALL;
        return ZRAGF_ERR_CORRUPTED_DATA;
    }

    *out_pos_out = out_pos;
    return ZRAGF_OK;
}

zragf_status zragf_decompress_workspace_bound(const void *in_data,
                                             zragf_size_t in_size,
                                             zragf_size_t *needed)
{
    const zragf_u8 *in;
    zragf_size_t in_pos;
    zragf_u32 expected_uncompressed;
    zragf_u8 flags;
    zragf_status st;
    zragf_size_t frame_remaining;

    if (!in_data || !needed)
        return ZRAGF_ST_NULL_POINTER;

    in = (const zragf_u8 *)in_data;
    flags = 0u;
    st = zragf_frame_read_header(in, in_size, &expected_uncompressed, &flags, &in_pos);
    if (st != ZRAGF_OK)
        return st;

    frame_remaining = in_size - in_pos;
    if ((flags & ZRAGF_FLAG_CHECKSUM) != 0u) {
        if (frame_remaining < 4u)
            return ZRAGF_ERR_CORRUPTED_DATA;
        frame_remaining -= 4u;
    }

    if ((flags & ZRAGF_FLAG_HUFFMAN) == 0u || expected_uncompressed == 0u) {
        *needed = 0u;
        return ZRAGF_ST_OK;
    }

    if (frame_remaining < 4u + 1u + ZRAGF_HUFF_SYMS)
        return ZRAGF_ERR_CORRUPTED_DATA;

    *needed = (zragf_size_t)zragf_read_u32_le(in + in_pos);
    return ZRAGF_ST_OK;
}

static zragf_status
zragf_decompress_impl(const void *in_data,
                      zragf_size_t in_size,
                      void *out_data,
                      zragf_size_t *out_size,
                      void *workspace,
                      zragf_size_t workspace_size,
                      int workspace_required,
                      zragf_info *info)
{
    const zragf_u8 *in;
    zragf_u8 *out;
    zragf_size_t out_cap;
    zragf_size_t out_pos;
    zragf_size_t in_pos;
    zragf_u32 expected_uncompressed;
    zragf_status st;
    zragf_u8 flags;
    zragf_size_t frame_remaining;
    zragf_u32 stored_crc;
    int has_checksum;

    out_pos = 0u;
    flags = 0u;
    stored_crc = 0u;
    has_checksum = 0;

    if (!in_data || !out_data || !out_size)
        return ZRAGF_ERR_NULL_POINTER;

    in      = (const zragf_u8 *)in_data;
    out     = (zragf_u8 *)out_data;
    out_cap = *out_size;

    st = zragf_frame_read_header(in, in_size,
                                 &expected_uncompressed,
                                 &flags,
                                 &in_pos);
    if (st != ZRAGF_OK)
        return st;

    frame_remaining = in_size - in_pos;
    has_checksum = ((flags & ZRAGF_FLAG_CHECKSUM) != 0u);

    if (has_checksum) {
        if (frame_remaining < 4u)
            return ZRAGF_ERR_CORRUPTED_DATA;
        frame_remaining -= 4u;
        stored_crc = zragf_read_u32_le(in + in_pos + frame_remaining);
    }

    if (expected_uncompressed == 0u) {
        if (has_checksum) {
            zragf_u32 crc = zragf_crc32(NULL, 0u);
            if (crc != stored_crc)
                return ZRAGF_ERR_CORRUPTED_DATA;
        }
        *out_size = 0u;
        if (info) {
            info->uncompressed_size = 0u;
            info->used_input = (zragf_u32)in_size;
            info->used_output = 0u;
        }
        return ZRAGF_OK;
    }

    if ((flags & ZRAGF_FLAG_HUFFMAN) == 0u) {
        st = zragf_decode_lz_stream(in + in_pos,
                                    frame_remaining,
                                    out,
                                    out_cap,
                                    expected_uncompressed,
                                    &out_pos);
        if (st != ZRAGF_OK)
            return st;
        if (has_checksum) {
            zragf_u32 crc = zragf_crc32(out, out_pos);
            if (crc != stored_crc)
                return ZRAGF_ERR_CORRUPTED_DATA;
        }
        *out_size = out_pos;
        if (info) {
            info->uncompressed_size = expected_uncompressed;
            info->used_input = (zragf_u32)in_size;
            info->used_output = (zragf_u32)out_pos;
        }
        return ZRAGF_OK;
    } else {
        zragf_size_t remaining;
        zragf_size_t lz_size;
        zragf_u8 max_bits;
        zragf_u8 lens[ZRAGF_HUFF_SYMS];
        zragf_huff_tables htab;
        zragf_bitreader br;
        zragf_u8 *lz_buf;
        zragf_u8 *owned_buf;
        zragf_size_t i;
        zragf_size_t bits_bytes;
        zragf_size_t used_bits_bytes;

        remaining = frame_remaining;
        lz_size = 0u;
        max_bits = 0u;
        bits_bytes = 0u;
        used_bits_bytes = 0u;
        owned_buf = NULL;

        if (remaining < 4u + 1u + ZRAGF_HUFF_SYMS)
            return ZRAGF_ERR_CORRUPTED_DATA;

        lz_size = (zragf_size_t)zragf_read_u32_le(in + in_pos);
        in_pos += 4u;
        remaining = frame_remaining - 4u;

        if (remaining < 1u + ZRAGF_HUFF_SYMS)
            return ZRAGF_ERR_CORRUPTED_DATA;

        max_bits = in[in_pos++];
        {
            zragf_u32 s;
            for (s = 0; s < ZRAGF_HUFF_SYMS; ++s)
                lens[s] = in[in_pos++];
        }

        st = zragf_huff_build_from_lengths(lens, max_bits, &htab);
        if (st != ZRAGF_OK)
            return st;

        remaining = frame_remaining - (4u + 1u + ZRAGF_HUFF_SYMS);
        bits_bytes = remaining;

        if (lz_size == 0u) {
            if (expected_uncompressed != 0u)
                return ZRAGF_ERR_CORRUPTED_DATA;
            if (has_checksum) {
                zragf_u32 crc = zragf_crc32(NULL, 0u);
                if (crc != stored_crc)
                    return ZRAGF_ERR_CORRUPTED_DATA;
            }
            *out_size = 0u;
            if (info) {
                info->uncompressed_size = 0u;
                info->used_input = (zragf_u32)in_size;
                info->used_output = 0u;
            }
            return ZRAGF_OK;
        }

        if (workspace && workspace_size >= lz_size) {
            lz_buf = (zragf_u8 *)workspace;
        } else {
            if (workspace_required)
                return ZRAGF_ST_WORKSPACE_TOO_SMALL;
            lz_buf = (zragf_u8 *)zragf_alloc_default(NULL, 1u, lz_size);
            if (!lz_buf)
                return ZRAGF_ST_NO_MEMORY;
            owned_buf = lz_buf;
        }

        st = zragf_br_init(&br, in + in_pos, bits_bytes);
        if (st != ZRAGF_OK) {
            zragf_free_default(NULL, owned_buf);
            return st;
        }

        for (i = 0; i < lz_size; ++i) {
            zragf_u32 cur_code;
            zragf_u8 cur_len;
            int found;

            cur_code = 0u;
            cur_len = 0u;
            found = 0;
            while (!found) {
                zragf_u32 bit;
                zragf_u32 s;
                if (cur_len >= max_bits) {
                    zragf_free_default(NULL, owned_buf);
                    return ZRAGF_ERR_CORRUPTED_DATA;
                }
                st = zragf_br_get_bit(&br, &bit);
                if (st != ZRAGF_OK) {
                    zragf_free_default(NULL, owned_buf);
                    return st;
                }
                cur_code = (zragf_u32)((cur_code << 1) | bit);
                cur_len++;
                for (s = 0; s < ZRAGF_HUFF_SYMS; ++s) {
                    if (htab.bits[s] == cur_len && htab.code[s] == cur_code) {
                        lz_buf[i] = (zragf_u8)s;
                        found = 1;
                        break;
                    }
                }
            }
        }

        used_bits_bytes = br.pos;
        in_pos += used_bits_bytes;
        (void)in_pos;

        st = zragf_decode_lz_stream(lz_buf,
                                    lz_size,
                                    out,
                                    out_cap,
                                    expected_uncompressed,
                                    &out_pos);
        zragf_free_default(NULL, owned_buf);
        if (st != ZRAGF_OK)
            return st;

        if (has_checksum) {
            zragf_u32 crc = zragf_crc32(out, out_pos);
            if (crc != stored_crc)
                return ZRAGF_ERR_CORRUPTED_DATA;
        }

        *out_size = out_pos;
        if (info) {
            info->uncompressed_size = expected_uncompressed;
            info->used_input = (zragf_u32)in_size;
            info->used_output = (zragf_u32)out_pos;
        }
        return ZRAGF_OK;
    }
}

zragf_status
zragf_decompress(const void *in_data,
                 zragf_size_t in_size,
                 void *out_data,
                 zragf_size_t *out_size,
                 zragf_info *info)
{
    return zragf_decompress_impl(in_data, in_size, out_data, out_size,
                                 NULL, 0u, 0, info);
}

zragf_status
zragf_decompress_with_workspace(const void *in_data,
                                zragf_size_t in_size,
                                void *out_data,
                                zragf_size_t *out_size,
                                void *workspace,
                                zragf_size_t workspace_size,
                                zragf_info *info)
{
    return zragf_decompress_impl(in_data, in_size, out_data, out_size,
                                 workspace, workspace_size, 1, info);
}
