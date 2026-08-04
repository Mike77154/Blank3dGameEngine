#include <stdio.h>
#include <string.h>
#include "gbulletair89.h"

#define DEMO_FRAMES 28800UL
#define SHOWCASE_SILENCE 9600UL

static const char *demo_names[GBA89_PRESET_COUNT] = {
    "01_subsonic_whiz.wav",
    "02_transonic_zip.wav",
    "03_supersonic_snap.wav",
    "04_close_rifle_pass.wav",
    "05_heavy_round_pass.wav",
    "06_distant_crack.wav",
    "07_indoor_pass.wav",
    "08_game_whizz.wav"
};

static void write_u16(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
}

static void write_u32(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
    fputc((int)((v >> 16) & 255UL), f);
    fputc((int)((v >> 24) & 255UL), f);
}

static void write_header(FILE *f, unsigned long frames)
{
    unsigned long data_bytes;

    data_bytes = frames * 4UL;
    fwrite("RIFF", 1, 4, f);
    write_u32(f, 36UL + data_bytes);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    write_u32(f, 16UL);
    write_u16(f, 1UL);
    write_u16(f, 2UL);
    write_u32(f, GBA89_SAMPLE_RATE);
    write_u32(f, GBA89_SAMPLE_RATE * 4UL);
    write_u16(f, 4UL);
    write_u16(f, 16UL);
    fwrite("data", 1, 4, f);
    write_u32(f, data_bytes);
}

static void write_sample(FILE *f, gba89_s16 sample)
{
    unsigned short u;

    u = (unsigned short)sample;
    fputc((int)(u & 255U), f);
    fputc((int)((u >> 8) & 255U), f);
}

static int write_preset_file(int preset, const char *name)
{
    FILE *f;
    gba89_state state;
    gba89_s16 frame[2];
    unsigned long i;

    f = fopen(name, "wb");
    if (f == 0) {
        return 0;
    }
    write_header(f, DEMO_FRAMES);
    gba89_init(&state, 0x12345678UL + (unsigned long)preset * 977UL);
    gba89_trigger_preset(&state, preset);
    for (i = 0UL; i < DEMO_FRAMES; ++i) {
        gba89_render_stereo(&state, frame, 1UL);
        write_sample(f, frame[0]);
        write_sample(f, frame[1]);
    }
    fclose(f);
    return 1;
}

static int write_showcase(void)
{
    FILE *f;
    gba89_state state;
    gba89_s16 frame[2];
    unsigned long total_frames;
    unsigned long i;
    int p;

    total_frames = (DEMO_FRAMES + SHOWCASE_SILENCE) * (unsigned long)GBA89_PRESET_COUNT;
    f = fopen("gbulletair89_v1_2_showcase.wav", "wb");
    if (f == 0) {
        return 0;
    }
    write_header(f, total_frames);
    for (p = 0; p < GBA89_PRESET_COUNT; ++p) {
        gba89_init(&state, 0xCAFEBABEUL + (unsigned long)p * 313UL);
        gba89_trigger_preset(&state, p);
        for (i = 0UL; i < DEMO_FRAMES; ++i) {
            gba89_render_stereo(&state, frame, 1UL);
            write_sample(f, frame[0]);
            write_sample(f, frame[1]);
        }
        for (i = 0UL; i < SHOWCASE_SILENCE; ++i) {
            write_sample(f, 0);
            write_sample(f, 0);
        }
    }
    fclose(f);
    return 1;
}

int main(void)
{
    int i;

    for (i = 0; i < GBA89_PRESET_COUNT; ++i) {
        if (!write_preset_file(i, demo_names[i])) {
            fprintf(stderr, "Could not write %s\n", demo_names[i]);
            return 1;
        }
    }
    if (!write_showcase()) {
        fprintf(stderr, "Could not write showcase WAV\n");
        return 1;
    }
    printf("Generated %d preset WAV files and gbulletair89_v1_2_showcase.wav\n", GBA89_PRESET_COUNT);
    return 0;
}
