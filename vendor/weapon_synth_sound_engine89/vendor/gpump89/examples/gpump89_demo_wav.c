#include <stdio.h>
#include <stdlib.h>
#include "gpump89.h"

static void put_u16(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
}

static void put_u32(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
    fputc((int)((v >> 16) & 255UL), f);
    fputc((int)((v >> 24) & 255UL), f);
}

static int write_wav(const char *path, const gpump89_i16 *pcm,
                     gpump89_u32 frames, gpump89_u32 rate)
{
    FILE *f;
    gpump89_u32 i;
    gpump89_u32 bytes;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    bytes = frames * 2UL;
    fwrite("RIFF", 1, 4, f);
    put_u32(f, 36UL + bytes);
    fwrite("WAVEfmt ", 1, 8, f);
    put_u32(f, 16UL);
    put_u16(f, 1UL);
    put_u16(f, 1UL);
    put_u32(f, rate);
    put_u32(f, rate * 2UL);
    put_u16(f, 2UL);
    put_u16(f, 16UL);
    fwrite("data", 1, 4, f);
    put_u32(f, bytes);
    for (i = 0UL; i < frames; ++i) {
        unsigned long u;
        u = (unsigned long)((unsigned short)pcm[i]);
        put_u16(f, u);
    }
    fclose(f);
    return 1;
}

int main(int argc, char **argv)
{
    gpump89_context ctx;
    gpump89_params params;
    gpump89_i16 pcm[44100];
    gpump89_u32 total;
    gpump89_u32 at;
    gpump89_u32 n;
    int preset;
    const char *path;

    preset = GPUMP89_PRESET_TIGHT_DRY;
    path = "gpump89_preview.wav";
    if (argc > 1) preset = atoi(argv[1]);
    if (argc > 2) path = argv[2];

    if (!gpump89_preset(&params, preset, 44100UL)) {
        fprintf(stderr, "Unknown preset: %d\n", preset);
        return 2;
    }

    gpump89_init(&ctx, params.sample_rate, params.seed);
    gpump89_start_cycle(&ctx, &params);
    total = gpump89_total_frames(&ctx);
    if (total > 44100UL) {
        fprintf(stderr, "Preview buffer too small.\n");
        return 3;
    }

    at = 0UL;
    while (gpump89_is_active(&ctx) && at < total) {
        n = gpump89_render_i16(&ctx, &pcm[at], total - at);
        if (n == 0UL) break;
        at += n;
    }

    if (!write_wav(path, pcm, at, params.sample_rate)) {
        fprintf(stderr, "Could not write %s\n", path);
        return 4;
    }

    printf("wrote %s (%lu frames)\n", path, at);
    return 0;
}
