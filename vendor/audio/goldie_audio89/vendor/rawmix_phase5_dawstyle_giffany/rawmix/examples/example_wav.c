#include "example_wav.h"

static int example_write_u16_le(FILE *f, rm_u16 v)
{
    unsigned char b[2];
    b[0] = (unsigned char)(v & 0xFFU);
    b[1] = (unsigned char)((v >> 8) & 0xFFU);
    return fwrite(b, 1U, 2U, f) == 2U ? 1 : 0;
}

static int example_write_u32_le(FILE *f, rm_u32 v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFFU);
    b[1] = (unsigned char)((v >> 8) & 0xFFU);
    b[2] = (unsigned char)((v >> 16) & 0xFFU);
    b[3] = (unsigned char)((v >> 24) & 0xFFU);
    return fwrite(b, 1U, 4U, f) == 4U ? 1 : 0;
}

int example_wav_write_s16(const char *path,
                          const rm_s16 *samples,
                          rm_u32 frames,
                          rm_u16 channels,
                          rm_u32 sample_rate)
{
    FILE *f;
    rm_u32 data_bytes;
    rm_u32 riff_size;
    rm_u32 byte_rate;
    rm_u16 block_align;

    if (path == NULL || samples == NULL) {
        return 0;
    }
    if (channels != 1U && channels != 2U) {
        return 0;
    }

    data_bytes = frames * (rm_u32)channels * 2U;
    riff_size = 36U + data_bytes;
    byte_rate = sample_rate * (rm_u32)channels * 2U;
    block_align = (rm_u16)(channels * 2U);

    f = fopen(path, "wb");
    if (f == NULL) {
        return 0;
    }

    if (fwrite("RIFF", 1U, 4U, f) != 4U ||
        !example_write_u32_le(f, riff_size) ||
        fwrite("WAVE", 1U, 4U, f) != 4U ||
        fwrite("fmt ", 1U, 4U, f) != 4U ||
        !example_write_u32_le(f, 16U) ||
        !example_write_u16_le(f, 1U) ||
        !example_write_u16_le(f, channels) ||
        !example_write_u32_le(f, sample_rate) ||
        !example_write_u32_le(f, byte_rate) ||
        !example_write_u16_le(f, block_align) ||
        !example_write_u16_le(f, 16U) ||
        fwrite("data", 1U, 4U, f) != 4U ||
        !example_write_u32_le(f, data_bytes) ||
        fwrite(samples, 1U, (size_t)data_bytes, f) != (size_t)data_bytes) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}
