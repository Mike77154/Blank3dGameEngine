#include <stdio.h>
#include "wmagazine89.h"

#define DEMO_RATE 44100
#define DEMO_SECONDS_PER_PRESET 2
#define DEMO_PRESETS WMAG89_PRESET_COUNT
#define DEMO_TOTAL_FRAMES (DEMO_RATE * DEMO_SECONDS_PER_PRESET * DEMO_PRESETS)

static void write_u16_le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void write_u32_le(FILE *file, unsigned long value)
{
    fputc((int)(value & 255UL), file);
    fputc((int)((value >> 8) & 255UL), file);
    fputc((int)((value >> 16) & 255UL), file);
    fputc((int)((value >> 24) & 255UL), file);
}

static int write_wav_header(FILE *file, unsigned long frames)
{
    unsigned long data_bytes;
    unsigned long riff_bytes;

    data_bytes = frames * 2UL;
    riff_bytes = 36UL + data_bytes;

    if (fwrite("RIFF", 1U, 4U, file) != 4U) {
        return 0;
    }
    write_u32_le(file, riff_bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, file) != 8U) {
        return 0;
    }
    write_u32_le(file, 16UL);
    write_u16_le(file, 1U);
    write_u16_le(file, 1U);
    write_u32_le(file, (unsigned long)DEMO_RATE);
    write_u32_le(file, (unsigned long)DEMO_RATE * 2UL);
    write_u16_le(file, 2U);
    write_u16_le(file, 16U);
    if (fwrite("data", 1U, 4U, file) != 4U) {
        return 0;
    }
    write_u32_le(file, data_bytes);
    return 1;
}

static void write_sample(FILE *file, int sample)
{
    unsigned int value;
    value = (unsigned int)((unsigned short)((short)sample));
    write_u16_le(file, value);
}

static int render_combined_preview(const char *path)
{
    WMag89State state;
    FILE *file;
    int preset;
    int frame;
    int local;
    int action;
    int sample;

    file = fopen(path, "wb");
    if (file == (FILE *)0) {
        return 0;
    }
    if (!write_wav_header(file, (unsigned long)DEMO_TOTAL_FRAMES)) {
        fclose(file);
        return 0;
    }
    if (wmag89_init(&state, DEMO_RATE, 0x1234ABCDU) != WMAG89_OK) {
        fclose(file);
        return 0;
    }

    for (preset = 0; preset < DEMO_PRESETS; ++preset) {
        wmag89_set_preset(&state, preset);
        wmag89_reset(&state);
        for (local = 0; local < DEMO_RATE * DEMO_SECONDS_PER_PRESET; ++local) {
            if (local == 2205) {
                wmag89_trigger(&state, WMAG89_ACTION_INSERT, 31000);
            }
            if (local == 28665) {
                wmag89_trigger(&state, WMAG89_ACTION_SEAT_TAP, 28500);
            }
            if (local == 50715) {
                wmag89_trigger(&state, WMAG89_ACTION_TUG_CHECK, 25000);
            }
            if (local == 66150) {
                wmag89_trigger(&state, WMAG89_ACTION_REMOVE, 30000);
            }
            sample = wmag89_process_sample(&state);
            write_sample(file, sample);
        }
    }

    frame = ferror(file);
    fclose(file);
    action = frame;
    return action == 0;
}

static int render_single_preview(const char *path, int preset)
{
    WMag89State state;
    FILE *file;
    int frame;
    int sample;
    int total_frames;

    total_frames = DEMO_RATE * 2;
    file = fopen(path, "wb");
    if (file == (FILE *)0) {
        return 0;
    }
    if (!write_wav_header(file, (unsigned long)total_frames)) {
        fclose(file);
        return 0;
    }
    if (wmag89_init(&state, DEMO_RATE,
                    0xBEEF0000U + (unsigned int)preset) != WMAG89_OK) {
        fclose(file);
        return 0;
    }
    wmag89_set_preset(&state, preset);
    wmag89_reset(&state);

    for (frame = 0; frame < total_frames; ++frame) {
        if (frame == 2205) {
            wmag89_trigger(&state, WMAG89_ACTION_INSERT, 31500);
        }
        if (frame == 33075) {
            wmag89_trigger(&state, WMAG89_ACTION_RATTLE, 24000);
        }
        if (frame == 55125) {
            wmag89_trigger(&state, WMAG89_ACTION_REMOVE, 30000);
        }
        sample = wmag89_process_sample(&state);
        write_sample(file, sample);
    }

    frame = ferror(file);
    fclose(file);
    return frame == 0;
}

int main(void)
{
    static const char *paths[WMAG89_PRESET_COUNT] = {
        "previews/pistol_metal.wav",
        "previews/pistol_polymer.wav",
        "previews/smg_steel.wav",
        "previews/rifle_aluminum.wav",
        "previews/rifle_polymer.wav",
        "previews/sniper_box.wav",
        "previews/drum_heavy.wav"
    };
    int preset;

    if (!render_combined_preview("previews/wmagazine89_all_presets.wav")) {
        fprintf(stderr, "Could not write combined preview.\n");
        return 1;
    }

    for (preset = 0; preset < WMAG89_PRESET_COUNT; ++preset) {
        if (!render_single_preview(paths[preset], preset)) {
            fprintf(stderr, "Could not write preview: %s\n", paths[preset]);
            return 1;
        }
        printf("Rendered %s\n", paths[preset]);
    }

    printf("Rendered previews/wmagazine89_all_presets.wav\n");
    return 0;
}
