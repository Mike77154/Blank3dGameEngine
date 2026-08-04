#include <stdio.h>
#include "chuecka89_wav.h"

static void ch89_put_u16(FILE *f, ch89_u16 v)
{
    fputc((int)(v & 255U), f);
    fputc((int)((v >> 8) & 255U), f);
}

static void ch89_put_u32(FILE *f, ch89_u32 v)
{
    fputc((int)(v & 255U), f);
    fputc((int)((v >> 8) & 255U), f);
    fputc((int)((v >> 16) & 255U), f);
    fputc((int)((v >> 24) & 255U), f);
}

int ch89_write_wav_mono16(
    const char *path,
    const ch89_i16 *samples,
    ch89_u32 frames,
    ch89_u32 sample_rate
)
{
    FILE *f;
    ch89_u32 i;
    ch89_u32 data_bytes;
    ch89_u16 s;
    if (path == 0 || samples == 0) return 0;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    data_bytes = frames * 2U;
    fwrite("RIFF", 1U, 4U, f);
    ch89_put_u32(f, 36U + data_bytes);
    fwrite("WAVE", 1U, 4U, f);
    fwrite("fmt ", 1U, 4U, f);
    ch89_put_u32(f, 16U);
    ch89_put_u16(f, 1U);
    ch89_put_u16(f, 1U);
    ch89_put_u32(f, sample_rate);
    ch89_put_u32(f, sample_rate * 2U);
    ch89_put_u16(f, 2U);
    ch89_put_u16(f, 16U);
    fwrite("data", 1U, 4U, f);
    ch89_put_u32(f, data_bytes);
    for (i = 0U; i < frames; ++i) {
        s = (ch89_u16)samples[i];
        ch89_put_u16(f, s);
    }
    fclose(f);
    return 1;
}
