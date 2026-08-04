#include <stdio.h>
#include "gklek89.h"

#define CLIP_FRAMES 13230UL
#define ALL_FRAMES 61740UL

static gkl89_context ctx;
static gkl89_s16 clip[CLIP_FRAMES];
static gkl89_s16 all[ALL_FRAMES];

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

static int write_wav(const char *path, const gkl89_s16 *pcm,
                     unsigned long frames)
{
    FILE *f;
    unsigned long i;
    unsigned long bytes;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    bytes = frames * 2UL;
    fwrite("RIFF", 1U, 4U, f);
    put_u32(f, 36UL + bytes);
    fwrite("WAVEfmt ", 1U, 8U, f);
    put_u32(f, 16UL);
    put_u16(f, 1UL);
    put_u16(f, 1UL);
    put_u32(f, GKL89_SAMPLE_RATE);
    put_u32(f, GKL89_SAMPLE_RATE * 2UL);
    put_u16(f, 2UL);
    put_u16(f, 16UL);
    fwrite("data", 1U, 4U, f);
    put_u32(f, bytes);
    for (i = 0UL; i < frames; ++i) put_u16(f, (unsigned short)pcm[i]);
    fclose(f);
    return 1;
}

static int render_one(gkl89_preset_id id, const char *path,
                      unsigned long seed)
{
    gkl89_init(&ctx, seed);
    if (!gkl89_trigger(&ctx, id, seed + 1UL, 20U)) return 0;
    gkl89_render_mono(&ctx, clip, CLIP_FRAMES);
    return write_wav(path, clip, CLIP_FRAMES);
}

int main(void)
{
    unsigned long i;
    unsigned long t[4];
    gkl89_preset_id p[4];
    int n;

    if (!render_one(GKL89_PRESET_DRY_CLICK, "audio/00_dry_click.wav", 101UL)) return 1;
    if (!render_one(GKL89_PRESET_STEEL_KLEK, "audio/01_steel_klek.wav", 102UL)) return 2;
    if (!render_one(GKL89_PRESET_CARRIER_KLEK, "audio/02_carrier_klek.wav", 103UL)) return 3;
    if (!render_one(GKL89_PRESET_REAR_STOP, "audio/03_rear_stop.wav", 104UL)) return 4;

    for (i = 0UL; i < ALL_FRAMES; ++i) all[i] = 0;
    t[0] = 4410UL; t[1] = 17640UL; t[2] = 30870UL; t[3] = 44100UL;
    p[0] = GKL89_PRESET_DRY_CLICK;
    p[1] = GKL89_PRESET_STEEL_KLEK;
    p[2] = GKL89_PRESET_CARRIER_KLEK;
    p[3] = GKL89_PRESET_REAR_STOP;
    gkl89_init(&ctx, 500UL);
    n = 0;
    for (i = 0UL; i < ALL_FRAMES; ++i) {
        if (n < 4 && i == t[n]) {
            gkl89_trigger(&ctx, p[n], 600UL + (unsigned long)n, 0U);
            n++;
        }
        all[i] = gkl89_process_sample(&ctx);
    }
    if (!write_wav("audio/04_all_presets.wav", all, ALL_FRAMES)) return 5;
    printf("gklek89 rendered; context=%lu bytes\n",
           (unsigned long)gkl89_context_bytes());
    return 0;
}
