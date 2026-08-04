/* Offline WAV generator for gpaah89.
 * The DSP library itself uses no stdio; this small tool uses stdio only
 * to write RIFF/WAVE files.
 */
#include <stdio.h>
#include <string.h>
#include "gpaah89.h"

#define TOOL_RATE 44100U
#define TOOL_CHANNELS 2U
#define TOOL_BLOCK 256U

static void put_u16le(FILE *f, gpaah89_u16 v)
{
    fputc((int)(v & 255U), f);
    fputc((int)((v >> 8) & 255U), f);
}

static void put_u32le(FILE *f, gpaah89_u32 v)
{
    fputc((int)(v & 255U), f);
    fputc((int)((v >> 8) & 255U), f);
    fputc((int)((v >> 16) & 255U), f);
    fputc((int)((v >> 24) & 255U), f);
}

static int write_header(FILE *f, gpaah89_u32 frames)
{
    gpaah89_u32 data_bytes;
    data_bytes = frames * TOOL_CHANNELS * 2U;
    if (fwrite("RIFF", 1U, 4U, f) != 4U) return 0;
    put_u32le(f, 36U + data_bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, f) != 8U) return 0;
    put_u32le(f, 16U);
    put_u16le(f, 1U);
    put_u16le(f, (gpaah89_u16)TOOL_CHANNELS);
    put_u32le(f, TOOL_RATE);
    put_u32le(f, TOOL_RATE * TOOL_CHANNELS * 2U);
    put_u16le(f, (gpaah89_u16)(TOOL_CHANNELS * 2U));
    put_u16le(f, 16U);
    if (fwrite("data", 1U, 4U, f) != 4U) return 0;
    put_u32le(f, data_bytes);
    return 1;
}

static int write_sample(FILE *f, gpaah89_s16 v)
{
    gpaah89_u16 u;
    u = (gpaah89_u16)v;
    put_u16le(f, u);
    return ferror(f) ? 0 : 1;
}

static int render_file(const char *path,
                       gpaah89_preset_id preset_id,
                       gpaah89_u32 milliseconds,
                       gpaah89_u32 interval_ms,
                       gpaah89_u32 shot_count,
                       gpaah89_u32 seed)
{
    FILE *f;
    gpaah89_state state;
    gpaah89_preset preset;
    gpaah89_s16 block[TOOL_BLOCK * 2U];
    gpaah89_u32 total_frames;
    gpaah89_u32 frame;
    gpaah89_u32 n;
    gpaah89_u32 i;
    gpaah89_u32 next_shot;
    gpaah89_u32 shots_fired;
    gpaah89_u32 interval_frames;

    if (!gpaah89_get_preset(preset_id, &preset)) return 0;
    if (!gpaah89_init(&state, TOOL_RATE, &preset, seed)) return 0;

    total_frames = (milliseconds * TOOL_RATE) / 1000U;
    interval_frames = (interval_ms * TOOL_RATE) / 1000U;
    next_shot = interval_frames;
    shots_fired = 1U;

    f = fopen(path, "wb");
    if (f == 0) return 0;
    if (!write_header(f, total_frames)) {
        fclose(f);
        return 0;
    }

    frame = 0U;
    while (frame < total_frames) {
        n = total_frames - frame;
        if (n > TOOL_BLOCK) n = TOOL_BLOCK;

        for (i = 0U; i < n; ++i) {
            if (shot_count > 1U && shots_fired < shot_count &&
                frame + i >= next_shot) {
                gpaah89_trigger(&state, seed + shots_fired * 977U);
                shots_fired++;
                next_shot += interval_frames;
            }
            gpaah89_render_stereo(&state, &block[i * 2U], 1U);
        }

        for (i = 0U; i < n * 2U; ++i) {
            if (!write_sample(f, block[i])) {
                fclose(f);
                return 0;
            }
        }
        frame += n;
    }

    if (fclose(f) != 0) return 0;
    return 1;
}

int main(void)
{
    int ok;
    ok = 1;

    ok &= render_file("samples/base_presets/pistol_base.wav", GPAAH89_PRESET_PISTOL,
                      1100U, 0U, 1U, 1001U);
    ok &= render_file("samples/base_presets/magnum_base.wav", GPAAH89_PRESET_MAGNUM,
                      1450U, 0U, 1U, 2002U);
    ok &= render_file("samples/base_presets/shotgun_base.wav", GPAAH89_PRESET_SHOTGUN,
                      1800U, 0U, 1U, 3003U);
    ok &= render_file("samples/base_presets/metralla_single_base.wav", GPAAH89_PRESET_METRALLA,
                      900U, 0U, 1U, 4004U);
    ok &= render_file("samples/base_presets/metralla_burst_base.wav", GPAAH89_PRESET_METRALLA,
                      1600U, 83U, 10U, 4104U);
    ok &= render_file("samples/base_presets/gatling_single_base.wav", GPAAH89_PRESET_GATLING,
                      850U, 0U, 1U, 5005U);
    ok &= render_file("samples/base_presets/gatling_burst_base.wav", GPAAH89_PRESET_GATLING,
                      1600U, 20U, 40U, 5105U);
    ok &= render_file("samples/base_presets/rocket_launcher_base.wav", GPAAH89_PRESET_ROCKET_LAUNCHER,
                      3400U, 0U, 1U, 6006U);
    ok &= render_file("samples/base_presets/sniper_rifle_base.wav", GPAAH89_PRESET_SNIPER_RIFLE,
                      1400U, 0U, 1U, 7007U);

    if (!ok) {
        fprintf(stderr, "gpaah_render: failed\n");
        return 1;
    }

    printf("Generated gpaah89 BASE PRESET WAV samples.\n");
    return 0;
}
