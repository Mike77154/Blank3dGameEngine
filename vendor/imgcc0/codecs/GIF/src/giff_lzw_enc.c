#include "giff_internal.h"

#define GIFF_LZW_NO_PREFIX 0xFFFFu

static void giff_lzw_init_code_lengths(giff_encoder* enc)
{
    giff_u16 i;

    if (enc == 0 || enc->lzw_code_len == 0) {
        return;
    }

    giff_mem_zero(enc->lzw_code_len, (giff_u32)GIFF_LZW_TABLE_SIZE);
    for (i = 0u; i < enc->lzw_clear_code; ++i) {
        enc->lzw_code_len[i] = 1u;
    }
}

static void giff_lzw_enc_clear_dict(giff_encoder* enc)
{
    if (enc->hash_used != 0) {
        giff_mem_zero(enc->hash_used, (giff_u32)GIFF_LZW_ENC_HASH_SIZE);
    }
    if (enc->hash_prefix != 0) {
        giff_mem_zero(enc->hash_prefix, (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16)));
    }
    if (enc->hash_code != 0) {
        giff_mem_zero(enc->hash_code, (giff_u32)(GIFF_LZW_ENC_HASH_SIZE * sizeof(giff_u16)));
    }
    if (enc->hash_suffix != 0) {
        giff_mem_zero(enc->hash_suffix, (giff_u32)GIFF_LZW_ENC_HASH_SIZE);
    }

    enc->lzw_next_code = (giff_u16)(enc->lzw_end_code + 1u);
    enc->lzw_code_size = (giff_u8)(enc->lzw_min_code_size + 1u);
    enc->lzw_inputs_since_clear = 0u;
    enc->lzw_codes_since_clear = 0u;
    enc->lzw_stall_run = 0u;
    enc->lzw_prefix_len = 0u;
    giff_lzw_init_code_lengths(enc);
}

static giff_u32 giff_lzw_hash(giff_u16 prefix, giff_u8 suffix)
{
    giff_u32 value;

    value = (giff_u32)prefix;
    value = (value << 5u) ^ (giff_u32)suffix ^ (value >> 1u);
    return value & (giff_u32)(GIFF_LZW_ENC_HASH_SIZE - 1u);
}

static int giff_lzw_find(
    giff_encoder* enc,
    giff_u16 prefix,
    giff_u8 suffix,
    giff_u16* out_code,
    giff_u32* out_slot
)
{
    giff_u32 slot;
    giff_u32 probe;
    giff_u32 mask;

    mask = (giff_u32)(GIFF_LZW_ENC_HASH_SIZE - 1u);
    slot = giff_lzw_hash(prefix, suffix);

    for (probe = 0u; probe < (giff_u32)GIFF_LZW_ENC_HASH_SIZE; ++probe) {
        if (!enc->hash_used[slot]) {
            if (out_slot != 0) {
                *out_slot = slot;
            }
            return 0;
        }

        if (enc->hash_prefix[slot] == prefix && enc->hash_suffix[slot] == suffix) {
            if (out_code != 0) {
                *out_code = enc->hash_code[slot];
            }
            if (out_slot != 0) {
                *out_slot = slot;
            }
            return 1;
        }

        slot = (slot + 1u) & mask;
    }

    if (out_slot != 0) {
        *out_slot = 0u;
    }
    return 0;
}

static giff_result giff_lzw_flush_packet(giff_encoder* enc)
{
    giff_result rc;
    giff_u8 count;

    if (enc->packet_size == 0u) {
        return GIFF_OK;
    }

    count = enc->packet_size;
    rc = giff_encoder_write_bytes(enc, &count, 1u);
    if (rc != GIFF_OK) {
        return rc;
    }

    rc = giff_encoder_write_bytes(enc, enc->packet_buf, (giff_u32)enc->packet_size);
    if (rc != GIFF_OK) {
        return rc;
    }

    enc->packet_size = 0u;
    return GIFF_OK;
}

static giff_result giff_lzw_push_byte(giff_encoder* enc, giff_u8 byte)
{
    giff_result rc;

    if (enc->packet_size == (giff_u8)GIFF_DATA_SUBBLOCK_MAX) {
        rc = giff_lzw_flush_packet(enc);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    enc->packet_buf[enc->packet_size++] = byte;
    return GIFF_OK;
}

static giff_result giff_lzw_emit_code(giff_encoder* enc, giff_u16 code)
{
    giff_result rc;

    enc->lzw_bitbuf |= ((giff_u32)code << enc->lzw_bits_in_buf);
    enc->lzw_bits_in_buf = (giff_u8)(enc->lzw_bits_in_buf + enc->lzw_code_size);

    while (enc->lzw_bits_in_buf >= 8u) {
        rc = giff_lzw_push_byte(enc, (giff_u8)(enc->lzw_bitbuf & 0xFFu));
        if (rc != GIFF_OK) {
            return rc;
        }
        enc->lzw_bitbuf >>= 8u;
        enc->lzw_bits_in_buf = (giff_u8)(enc->lzw_bits_in_buf - 8u);
    }

    if (code != enc->lzw_clear_code && code != enc->lzw_end_code) {
        enc->lzw_codes_since_clear = (giff_u16)(enc->lzw_codes_since_clear + 1u);
    }

    return GIFF_OK;
}

static giff_u8 giff_lzw_should_clear(giff_encoder* enc)
{
    if (enc == 0 || enc->lzw_next_code <= enc->lzw_end_code + 16u) {
        return 0u;
    }

    if (enc->lzw_strategy == (giff_u8)GIFF_LZW_STRATEGY_AGGRESSIVE) {
        if (enc->frame_periodic_clear != 0u && enc->lzw_inputs_since_clear >= enc->frame_periodic_clear &&
            enc->lzw_next_code > enc->lzw_end_code + 64u) {
            return 1u;
        }
        if (enc->lzw_next_code >= enc->frame_reset_threshold && enc->lzw_stall_run >= enc->frame_stall_threshold) {
            return 1u;
        }
        if (enc->lzw_next_code >= (giff_u16)(GIFF_LZW_TABLE_SIZE - 64u)) {
            return 1u;
        }
        return 0u;
    }

    if (enc->lzw_strategy == (giff_u8)GIFF_LZW_STRATEGY_BALANCED) {
        if (enc->lzw_next_code >= enc->frame_reset_threshold && enc->lzw_stall_run >= enc->frame_stall_threshold) {
            return 1u;
        }
        if (enc->lzw_next_code >= (giff_u16)(GIFF_LZW_TABLE_SIZE - 16u) &&
            enc->lzw_stall_run >= (giff_u16)(enc->frame_stall_threshold / 2u)) {
            return 1u;
        }
        return 0u;
    }

    if (enc->lzw_next_code >= enc->frame_reset_threshold && enc->lzw_stall_run >= enc->frame_stall_threshold) {
        return 1u;
    }

    return 0u;
}

void giff_lzw_enc_reset(giff_encoder* enc)
{
    if (enc == 0) {
        return;
    }

    enc->lzw_bitbuf = 0u;
    enc->lzw_bits_in_buf = 0u;
    enc->packet_size = 0u;
    enc->lzw_have_prefix = 0u;
    enc->lzw_prefix_code = GIFF_LZW_NO_PREFIX;
    enc->lzw_min_code_size = 0u;
    enc->lzw_code_size = 0u;
    enc->lzw_clear_code = 0u;
    enc->lzw_end_code = 0u;
    enc->lzw_next_code = 0u;
    enc->lzw_inputs_since_clear = 0u;
    enc->lzw_codes_since_clear = 0u;
    enc->lzw_stall_run = 0u;
    enc->lzw_clear_count = 0u;
    enc->lzw_prefix_len = 0u;

    giff_lzw_enc_clear_dict(enc);

    if (enc->packet_buf != 0) {
        giff_mem_zero(enc->packet_buf, (giff_u32)GIFF_DATA_SUBBLOCK_MAX);
    }
}

giff_result giff_lzw_enc_begin_image(giff_encoder* enc, giff_u8 min_code_size)
{
    giff_result rc;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (min_code_size < 2u || min_code_size > 8u) {
        return GIFF_E_BAD_BLOCK;
    }

    enc->lzw_bitbuf = 0u;
    enc->lzw_bits_in_buf = 0u;
    enc->packet_size = 0u;
    enc->lzw_have_prefix = 0u;
    enc->lzw_prefix_code = GIFF_LZW_NO_PREFIX;
    enc->lzw_min_code_size = min_code_size;
    enc->lzw_clear_code = (giff_u16)(1u << min_code_size);
    enc->lzw_end_code = (giff_u16)(enc->lzw_clear_code + 1u);
    enc->lzw_clear_count = 0u;
    enc->lzw_prefix_len = 0u;

    giff_lzw_enc_clear_dict(enc);
    rc = giff_lzw_emit_code(enc, enc->lzw_clear_code);
    if (rc == GIFF_OK) {
        enc->lzw_clear_count = 1u;
        enc->lzw_codes_since_clear = 0u;
    }
    return rc;
}

giff_result giff_lzw_enc_force_clear(giff_encoder* enc)
{
    giff_result rc;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    rc = giff_lzw_emit_code(enc, enc->lzw_clear_code);
    if (rc != GIFF_OK) {
        return rc;
    }
    enc->lzw_clear_count = (giff_u16)(enc->lzw_clear_count + 1u);
    giff_lzw_enc_clear_dict(enc);
    return GIFF_OK;
}

giff_result giff_lzw_enc_soft_break(giff_encoder* enc)
{
    giff_result rc;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (enc->lzw_have_prefix) {
        rc = giff_lzw_emit_code(enc, enc->lzw_prefix_code);
        if (rc != GIFF_OK) {
            return rc;
        }
        enc->lzw_have_prefix = 0u;
        enc->lzw_prefix_code = GIFF_LZW_NO_PREFIX;
        enc->lzw_prefix_len = 0u;
    }

    rc = giff_lzw_enc_force_clear(enc);
    return rc;
}

giff_result giff_lzw_enc_emit_index(giff_encoder* enc, giff_u8 index)
{
    giff_result rc;
    giff_u16 match_code;
    giff_u32 slot;
    giff_u8 prefix_len;
    giff_u8 new_len;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (!enc->lzw_have_prefix) {
        enc->lzw_prefix_code = (giff_u16)index;
        enc->lzw_have_prefix = 1u;
        enc->lzw_prefix_len = 1u;
        enc->lzw_inputs_since_clear = (giff_u16)(enc->lzw_inputs_since_clear + 1u);
        return GIFF_OK;
    }

    enc->lzw_inputs_since_clear = (giff_u16)(enc->lzw_inputs_since_clear + 1u);
    slot = 0u;
    if (giff_lzw_find(enc, enc->lzw_prefix_code, index, &match_code, &slot)) {
        enc->lzw_prefix_code = match_code;
        if (enc->lzw_code_len != 0 && enc->lzw_code_len[match_code] != 0u) {
            enc->lzw_prefix_len = enc->lzw_code_len[match_code];
        } else if (enc->lzw_prefix_len < 255u) {
            enc->lzw_prefix_len = (giff_u8)(enc->lzw_prefix_len + 1u);
        }
        return GIFF_OK;
    }

    rc = giff_lzw_emit_code(enc, enc->lzw_prefix_code);
    if (rc != GIFF_OK) {
        return rc;
    }

    prefix_len = enc->lzw_prefix_len;
    if (prefix_len <= 2u) {
        enc->lzw_stall_run = (giff_u16)(enc->lzw_stall_run + 1u);
    } else if (enc->lzw_stall_run > 0u) {
        enc->lzw_stall_run = (giff_u16)(enc->lzw_stall_run - 1u);
    }

    if (enc->lzw_next_code < (giff_u16)GIFF_LZW_TABLE_SIZE) {
        enc->hash_used[slot] = 1u;
        enc->hash_prefix[slot] = enc->lzw_prefix_code;
        enc->hash_suffix[slot] = index;
        enc->hash_code[slot] = enc->lzw_next_code;
        if (enc->lzw_code_len != 0) {
            new_len = (giff_u8)(prefix_len + 1u);
            if (new_len < prefix_len) {
                new_len = 255u;
            }
            enc->lzw_code_len[enc->lzw_next_code] = new_len;
        }
        enc->lzw_next_code = (giff_u16)(enc->lzw_next_code + 1u);

        if (enc->lzw_next_code > (giff_u16)(1u << enc->lzw_code_size) &&
            enc->lzw_code_size < (giff_u8)GIFF_LZW_MAX_BITS) {
            enc->lzw_code_size = (giff_u8)(enc->lzw_code_size + 1u);
        }
    }

    if (giff_lzw_should_clear(enc)) {
        rc = giff_lzw_enc_force_clear(enc);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    enc->lzw_prefix_code = (giff_u16)index;
    enc->lzw_have_prefix = 1u;
    enc->lzw_prefix_len = 1u;
    return GIFF_OK;
}

giff_result giff_lzw_enc_end_image(giff_encoder* enc)
{
    giff_result rc;
    giff_u8 terminator;

    if (enc == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    if (enc->lzw_have_prefix) {
        rc = giff_lzw_emit_code(enc, enc->lzw_prefix_code);
        if (rc != GIFF_OK) {
            return rc;
        }
    }

    rc = giff_lzw_emit_code(enc, enc->lzw_end_code);
    if (rc != GIFF_OK) {
        return rc;
    }

    while (enc->lzw_bits_in_buf != 0u) {
        rc = giff_lzw_push_byte(enc, (giff_u8)(enc->lzw_bitbuf & 0xFFu));
        if (rc != GIFF_OK) {
            return rc;
        }

        if (enc->lzw_bits_in_buf > 8u) {
            enc->lzw_bits_in_buf = (giff_u8)(enc->lzw_bits_in_buf - 8u);
        } else {
            enc->lzw_bits_in_buf = 0u;
        }
        enc->lzw_bitbuf >>= 8u;
    }

    rc = giff_lzw_flush_packet(enc);
    if (rc != GIFF_OK) {
        return rc;
    }

    terminator = 0u;
    rc = giff_encoder_write_bytes(enc, &terminator, 1u);
    if (rc != GIFF_OK) {
        return rc;
    }

    enc->lzw_have_prefix = 0u;
    enc->lzw_prefix_code = GIFF_LZW_NO_PREFIX;
    enc->lzw_prefix_len = 0u;
    return GIFF_OK;
}
