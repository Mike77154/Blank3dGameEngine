#include "mpcm89.h"

static void mpcm89_clear_clip(mpcm89_clip *clip)
{
    clip->format.channels = 0;
    clip->format.sample_rate = 0;
    clip->format.byte_rate = 0;
    clip->format.block_align = 0;
    clip->format.bits_per_sample = 0;
    clip->bytes = (const mpcm89_u8 *)0;
    clip->byte_count = 0;
    clip->frame_count = 0;
}

int mpcm89_clip_init(
    mpcm89_clip *clip,
    const mpcm89_u8 *bytes,
    mpcm89_u32 byte_count,
    const mpcm89_format *format)
{
    mpcm89_u32 bytes_per_sample;
    mpcm89_u32 expected_align;
    mpcm89_u32 expected_byte_rate;
    mpcm89_u32 max_u32;

    if (clip == (mpcm89_clip *)0 ||
        bytes == (const mpcm89_u8 *)0 ||
        format == (const mpcm89_format *)0) {
        return MPCM89_ERR_ARGUMENT;
    }

    mpcm89_clear_clip(clip);

    if (format->channels < 1U || format->channels > 2U) {
        return MPCM89_ERR_CHANNELS;
    }
    if (format->sample_rate < 1000U || format->sample_rate > 768000U) {
        return MPCM89_ERR_SAMPLE_RATE;
    }
    if (format->bits_per_sample != 8U && format->bits_per_sample != 16U) {
        return MPCM89_ERR_BITS;
    }

    bytes_per_sample = (mpcm89_u32)format->bits_per_sample / 8U;
    expected_align = (mpcm89_u32)format->channels * bytes_per_sample;
    if ((mpcm89_u32)format->block_align != expected_align) {
        return MPCM89_ERR_BLOCK_ALIGN;
    }

    max_u32 = ~(mpcm89_u32)0;
    if (format->sample_rate > max_u32 / expected_align) {
        return MPCM89_ERR_BYTE_RATE;
    }
    expected_byte_rate = format->sample_rate * expected_align;
    if (format->byte_rate != expected_byte_rate) {
        return MPCM89_ERR_BYTE_RATE;
    }

    if (byte_count == 0U || (byte_count % expected_align) != 0U) {
        return MPCM89_ERR_DATA_SIZE;
    }

    clip->format = *format;
    clip->bytes = bytes;
    clip->byte_count = byte_count;
    clip->frame_count = byte_count / expected_align;
    return MPCM89_OK;
}

mpcm89_u32 mpcm89_duration_ms(const mpcm89_clip *clip)
{
    mpcm89_u32 whole_seconds;
    mpcm89_u32 remaining_frames;
    mpcm89_u32 milliseconds;
    mpcm89_u32 max_u32;

    if (clip == (const mpcm89_clip *)0 || clip->format.sample_rate == 0U) {
        return 0U;
    }

    whole_seconds = clip->frame_count / clip->format.sample_rate;
    remaining_frames = clip->frame_count % clip->format.sample_rate;
    max_u32 = ~(mpcm89_u32)0;
    if (whole_seconds > max_u32 / 1000U) {
        return max_u32;
    }
    milliseconds = whole_seconds * 1000U;
    milliseconds += (remaining_frames * 1000U) / clip->format.sample_rate;
    return milliseconds;
}

const char *mpcm89_result_string(int result)
{
    switch (result) {
    case MPCM89_OK: return "ok";
    case MPCM89_ERR_ARGUMENT: return "bad argument";
    case MPCM89_ERR_CHANNELS: return "unsupported channel count";
    case MPCM89_ERR_SAMPLE_RATE: return "invalid sample rate";
    case MPCM89_ERR_BITS: return "only integer PCM 8/16-bit is supported";
    case MPCM89_ERR_BLOCK_ALIGN: return "invalid block alignment";
    case MPCM89_ERR_BYTE_RATE: return "invalid byte rate";
    case MPCM89_ERR_DATA_SIZE: return "invalid PCM data size";
    default: return "unknown mpcm89 result";
    }
}
