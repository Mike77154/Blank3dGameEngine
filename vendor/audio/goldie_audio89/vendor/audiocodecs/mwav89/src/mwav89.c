#include "mwav89.h"

static mwav89_u16 mwav89_read_u16_le(const mwav89_u8 *p)
{
    return (mwav89_u16)((mwav89_u16)p[0] |
        (mwav89_u16)((mwav89_u16)p[1] << 8));
}

static mwav89_u32 mwav89_read_u32_le(const mwav89_u8 *p)
{
    return (mwav89_u32)p[0] |
        ((mwav89_u32)p[1] << 8) |
        ((mwav89_u32)p[2] << 16) |
        ((mwav89_u32)p[3] << 24);
}

static int mwav89_id_is(const mwav89_u8 *p, const char *id)
{
    return p[0] == (mwav89_u8)id[0] &&
        p[1] == (mwav89_u8)id[1] &&
        p[2] == (mwav89_u8)id[2] &&
        p[3] == (mwav89_u8)id[3];
}

static void mwav89_clear_view(mwav89_view *view)
{
    view->format_tag = 0;
    view->channels = 0;
    view->sample_rate = 0;
    view->byte_rate = 0;
    view->block_align = 0;
    view->bits_per_sample = 0;
    view->pcm_data = (const mwav89_u8 *)0;
    view->pcm_bytes = 0;
}

int mwav89_parse_memory(
    const void *memory,
    mwav89_u32 memory_bytes,
    mwav89_view *out_view)
{
    const mwav89_u8 *bytes;
    mwav89_u32 riff_size;
    mwav89_u32 parse_limit;
    mwav89_u32 offset;
    mwav89_u32 chunk_size;
    mwav89_u32 data_offset;
    mwav89_u32 next_offset;
    int have_fmt;
    int have_data;

    if (memory == (const void *)0 || out_view == (mwav89_view *)0) {
        return MWAV89_ERR_ARGUMENT;
    }

    mwav89_clear_view(out_view);
    if (memory_bytes < 12U) {
        return MWAV89_ERR_TOO_SMALL;
    }

    bytes = (const mwav89_u8 *)memory;
    if (!mwav89_id_is(bytes, "RIFF")) {
        return MWAV89_ERR_NOT_RIFF;
    }
    if (!mwav89_id_is(bytes + 8, "WAVE")) {
        return MWAV89_ERR_NOT_WAVE;
    }

    riff_size = mwav89_read_u32_le(bytes + 4);
    if (riff_size < 4U) {
        return MWAV89_ERR_TRUNCATED;
    }
    if (riff_size > memory_bytes - 8U) {
        return MWAV89_ERR_TRUNCATED;
    }
    parse_limit = riff_size + 8U;

    have_fmt = 0;
    have_data = 0;
    offset = 12U;

    while (offset <= parse_limit && parse_limit - offset >= 8U) {
        chunk_size = mwav89_read_u32_le(bytes + offset + 4U);
        data_offset = offset + 8U;
        if (chunk_size > parse_limit - data_offset) {
            return MWAV89_ERR_TRUNCATED;
        }

        if (mwav89_id_is(bytes + offset, "fmt ")) {
            if (chunk_size < 16U) {
                return MWAV89_ERR_BAD_FMT;
            }
            out_view->format_tag = mwav89_read_u16_le(bytes + data_offset);
            out_view->channels = mwav89_read_u16_le(bytes + data_offset + 2U);
            out_view->sample_rate = mwav89_read_u32_le(bytes + data_offset + 4U);
            out_view->byte_rate = mwav89_read_u32_le(bytes + data_offset + 8U);
            out_view->block_align = mwav89_read_u16_le(bytes + data_offset + 12U);
            out_view->bits_per_sample = mwav89_read_u16_le(bytes + data_offset + 14U);
            have_fmt = 1;
        } else if (mwav89_id_is(bytes + offset, "data")) {
            out_view->pcm_data = bytes + data_offset;
            out_view->pcm_bytes = chunk_size;
            have_data = 1;
        }

        next_offset = data_offset + chunk_size;
        if ((chunk_size & 1U) != 0U && next_offset < parse_limit) {
            next_offset += 1U;
        }
        if (next_offset <= offset) {
            return MWAV89_ERR_TRUNCATED;
        }
        offset = next_offset;
    }

    if (!have_fmt) {
        return MWAV89_ERR_NO_FMT;
    }
    if (!have_data) {
        return MWAV89_ERR_NO_DATA;
    }
    if (out_view->format_tag != 1U) {
        return MWAV89_ERR_UNSUPPORTED_FORMAT;
    }

    return MWAV89_OK;
}

const char *mwav89_result_string(int result)
{
    switch (result) {
    case MWAV89_OK: return "ok";
    case MWAV89_ERR_ARGUMENT: return "bad argument";
    case MWAV89_ERR_TOO_SMALL: return "buffer too small";
    case MWAV89_ERR_NOT_RIFF: return "not RIFF";
    case MWAV89_ERR_NOT_WAVE: return "not WAVE";
    case MWAV89_ERR_TRUNCATED: return "truncated RIFF/WAVE";
    case MWAV89_ERR_NO_FMT: return "missing fmt chunk";
    case MWAV89_ERR_NO_DATA: return "missing data chunk";
    case MWAV89_ERR_UNSUPPORTED_FORMAT: return "unsupported WAVE encoding";
    case MWAV89_ERR_BAD_FMT: return "invalid fmt chunk";
    default: return "unknown mwav89 result";
    }
}
