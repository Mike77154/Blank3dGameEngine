#include <stdio.h>
#include <string.h>
#include "wsound_ggrenadeblast89.h"

#define DEMO_RATE 44100UL

static ws_ggb89 g_synth;

static void put_u16le(FILE *fp, ws_gu16 v)
{
    fputc((int)(v & 255U), fp);
    fputc((int)((v >> 8) & 255U), fp);
}

static void put_u32le(FILE *fp, ws_gu32 v)
{
    fputc((int)(v & 255UL), fp);
    fputc((int)((v >> 8) & 255UL), fp);
    fputc((int)((v >> 16) & 255UL), fp);
    fputc((int)((v >> 24) & 255UL), fp);
}

static int write_wav_header(FILE *fp, ws_gu32 frames)
{
    ws_gu32 data_bytes;
    ws_gu32 riff_size;

    data_bytes = frames * 2UL;
    riff_size = 36UL + data_bytes;

    if (fwrite("RIFF", 1, 4, fp) != 4) return 0;
    put_u32le(fp, riff_size);
    if (fwrite("WAVE", 1, 4, fp) != 4) return 0;
    if (fwrite("fmt ", 1, 4, fp) != 4) return 0;
    put_u32le(fp, 16UL);
    put_u16le(fp, 1);
    put_u16le(fp, 1);
    put_u32le(fp, DEMO_RATE);
    put_u32le(fp, DEMO_RATE * 2UL);
    put_u16le(fp, 2);
    put_u16le(fp, 16);
    if (fwrite("data", 1, 4, fp) != 4) return 0;
    put_u32le(fp, data_bytes);
    return 1;
}

static int render_one(const char *filename, int preset, ws_gu32 seed)
{
    FILE *fp;
    ws_ggb89_params p;
    ws_gu32 frames;
    ws_gu32 i;
    ws_gs16 sample;

    if (!ws_ggb89_get_preset(preset, &p)) {
        return 0;
    }

    frames = ((ws_gu32)p.total_ms * DEMO_RATE) / 1000UL;
    fp = fopen(filename, "wb");
    if (fp == 0) {
        return 0;
    }
    if (!write_wav_header(fp, frames)) {
        fclose(fp);
        return 0;
    }

    ws_ggb89_init(&g_synth, DEMO_RATE, seed);
    ws_ggb89_trigger(&g_synth, preset, 32767);
    for (i = 0; i < frames; ++i) {
        sample = ws_ggb89_process(&g_synth);
        put_u16le(fp, (ws_gu16)sample);
    }
    fclose(fp);
    return 1;
}

static int render_montage(const char *filename)
{
    FILE *fp;
    ws_gu32 segment_frames;
    ws_gu32 gap_frames;
    ws_gu32 total_frames;
    ws_gu32 i;
    ws_gu32 j;
    ws_gs16 sample;
    int preset;

    segment_frames = DEMO_RATE * 3UL;
    gap_frames = DEMO_RATE / 2UL;
    total_frames = (segment_frames + gap_frames) * WS_GGB89_PRESET_COUNT;

    fp = fopen(filename, "wb");
    if (fp == 0) {
        return 0;
    }
    if (!write_wav_header(fp, total_frames)) {
        fclose(fp);
        return 0;
    }

    for (preset = 0; preset < WS_GGB89_PRESET_COUNT; ++preset) {
        ws_ggb89_init(&g_synth, DEMO_RATE,
                      0xA5C30000UL + (ws_gu32)preset * 977UL);
        ws_ggb89_trigger(&g_synth, preset, 32767);
        for (i = 0; i < segment_frames; ++i) {
            sample = ws_ggb89_process(&g_synth);
            put_u16le(fp, (ws_gu16)sample);
        }
        for (j = 0; j < gap_frames; ++j) {
            put_u16le(fp, 0);
        }
    }

    fclose(fp);
    return 1;
}

int main(void)
{
    static char *names[WS_GGB89_PRESET_COUNT] = {
        "previews/00_m67_open.wav",
        "previews/01_40mm_he_open.wav",
        "previews/02_40mm_hedp_hard.wav",
        "previews/03_indoor_confined.wav",
        "previews/04_concrete_impact.wav",
        "previews/05_dirt_impact.wav",
        "previews/06_distant.wav",
        "previews/07_arcade_heavy.wav"
    };
    int i;

    for (i = 0; i < WS_GGB89_PRESET_COUNT; ++i) {
        if (!render_one(names[i], i, 0x19890101UL + (ws_gu32)i * 313UL)) {
            fprintf(stderr, "failed: %s\n", names[i]);
            return 1;
        }
        printf("rendered: %s (%s)\n", names[i], ws_ggb89_preset_name(i));
    }

    if (!render_montage("previews/wsound_ggrenadeblast89_montage.wav")) {
        fprintf(stderr, "failed montage\n");
        return 1;
    }
    printf("rendered montage\n");
    printf("context bytes: %lu\n", (unsigned long)sizeof(ws_ggb89));
    return 0;
}
