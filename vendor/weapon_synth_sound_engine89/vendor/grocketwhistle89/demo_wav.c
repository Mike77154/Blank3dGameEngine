#include <stdio.h>
#include "grocketwhistle89.h"

#define DEMO_SAMPLE_RATE 44100
#define DEMO_CHANNELS 2
#define DEMO_BITS 16
#define DEMO_BLOCK 256

static void write_u16_le(FILE *file, gwh89_u32 value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void write_u32_le(FILE *file, gwh89_u32 value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
    fputc((int)((value >> 16) & 255U), file);
    fputc((int)((value >> 24) & 255U), file);
}

static void write_wav_header(FILE *file, gwh89_u32 frames)
{
    gwh89_u32 data_bytes;
    gwh89_u32 riff_bytes;
    gwh89_u32 byte_rate;
    gwh89_u32 block_align;

    data_bytes = frames * DEMO_CHANNELS * (DEMO_BITS / 8);
    riff_bytes = 36U + data_bytes;
    byte_rate = DEMO_SAMPLE_RATE * DEMO_CHANNELS * (DEMO_BITS / 8);
    block_align = DEMO_CHANNELS * (DEMO_BITS / 8);

    fwrite("RIFF", 1, 4, file);
    write_u32_le(file, riff_bytes);
    fwrite("WAVE", 1, 4, file);
    fwrite("fmt ", 1, 4, file);
    write_u32_le(file, 16U);
    write_u16_le(file, 1U);
    write_u16_le(file, DEMO_CHANNELS);
    write_u32_le(file, DEMO_SAMPLE_RATE);
    write_u32_le(file, byte_rate);
    write_u16_le(file, block_align);
    write_u16_le(file, DEMO_BITS);
    fwrite("data", 1, 4, file);
    write_u32_le(file, data_bytes);
}

static gwh89_s32 ratio_q15(gwh89_s32 numerator, gwh89_s32 denominator)
{
    gwh89_s32 result;
    gwh89_s32 remainder;
    gwh89_s32 i;
    if (denominator <= 0 || numerator <= 0) {
        return 0;
    }
    if (numerator >= denominator) {
        return GWH89_Q15_ONE;
    }
    result = 0;
    remainder = numerator;
    for (i = 0; i < 15; ++i) {
        remainder <<= 1;
        result <<= 1;
        if (remainder >= denominator) {
            remainder -= denominator;
            result |= 1;
        }
    }
    return result;
}

static void write_sample(FILE *file, gwh89_s16 sample)
{
    gwh89_u16 value;
    value = (gwh89_u16)sample;
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static gwh89_s32 render_preset_to_stream(
    FILE *file,
    gwh89_s32 preset_id,
    gwh89_u32 seed
)
{
    gwh89_state state;
    gwh89_s16 block[DEMO_BLOCK * 2];
    gwh89_s32 total;
    gwh89_s32 rendered;
    gwh89_s32 motion_total;
    gwh89_s32 count;
    gwh89_s32 i;
    gwh89_s32 progress_q15;
    gwh89_s32 velocity_mps;
    gwh89_s32 gain_q15;

    gwh89_init(&state, DEMO_SAMPLE_RATE, seed);
    gwh89_trigger_preset(&state, preset_id);
    total = gwh89_get_estimated_total_samples(&state);
    motion_total = total - state.reverb_tail_samples;
    if (motion_total < 1) {
        motion_total = total;
    }
    if (total <= 0) {
        return 0;
    }

    rendered = 0;
    while (gwh89_is_active(&state)) {
        count = DEMO_BLOCK;
        if (rendered + count > total + DEMO_BLOCK) {
            count = total + DEMO_BLOCK - rendered;
        }
        if (count <= 0) {
            break;
        }

        progress_q15 = ratio_q15(rendered, motion_total);
        if (progress_q15 > GWH89_Q15_ONE) {
            progress_q15 = GWH89_Q15_ONE;
        }
        velocity_mps = -95 + ((190 * progress_q15) >> 15);

        gain_q15 = 19000;
        if (progress_q15 < 16384) {
            gain_q15 += (progress_q15 * 12000) >> 14;
        } else {
            gain_q15 += ((GWH89_Q15_ONE - progress_q15) * 12000) >> 14;
        }
        if (gain_q15 > GWH89_Q15_ONE) {
            gain_q15 = GWH89_Q15_ONE;
        }

        gwh89_set_motion(&state, velocity_mps, gain_q15);
        gwh89_render_stereo(&state, block, count);

        for (i = 0; i < count; ++i) {
            write_sample(file, block[i * 2]);
            write_sample(file, block[i * 2 + 1]);
        }
        rendered += count;
    }
    return rendered;
}

static gwh89_s32 write_silence(FILE *file, gwh89_s32 frames)
{
    gwh89_s32 i;
    for (i = 0; i < frames; ++i) {
        write_sample(file, 0);
        write_sample(file, 0);
    }
    return frames;
}

static int make_single_preset_file(
    const char *filename,
    gwh89_s32 preset_id,
    gwh89_u32 seed
)
{
    FILE *file;
    gwh89_s32 frames;

    file = fopen(filename, "wb");
    if (file == 0) {
        return 0;
    }
    write_wav_header(file, 0U);
    frames = render_preset_to_stream(file, preset_id, seed);
    fseek(file, 0L, SEEK_SET);
    write_wav_header(file, (gwh89_u32)frames);
    fclose(file);
    return 1;
}

static int make_showcase(void)
{
    static const char *names[GWH89_PRESET_COUNT] = {
        "preview_rpg7_sustained.wav",
        "preview_smaw_short.wav",
        "preview_javelin_two_stage.wav",
        "preview_at4_fast.wav",
        "preview_guided_flyby.wav",
        "preview_heavy_rocket.wav",
        "preview_chifladora_air_reference.wav"
    };
    FILE *file;
    gwh89_s32 total_frames;
    gwh89_s32 preset_id;
    gwh89_s32 silence_frames;
    gwh89_s32 frames;

    total_frames = 0;
    silence_frames = DEMO_SAMPLE_RATE / 4;

    file = fopen("grocketwhistle89_showcase.wav", "wb");
    if (file == 0) {
        return 0;
    }
    write_wav_header(file, 0U);

    for (preset_id = 0; preset_id < GWH89_PRESET_COUNT; ++preset_id) {
        frames = render_preset_to_stream(
            file,
            preset_id,
            0x12345678U + (gwh89_u32)(preset_id * 977)
        );
        total_frames += frames;
        if (preset_id + 1 < GWH89_PRESET_COUNT) {
            total_frames += write_silence(file, silence_frames);
        }
    }

    fseek(file, 0L, SEEK_SET);
    write_wav_header(file, (gwh89_u32)total_frames);
    fclose(file);

    for (preset_id = 0; preset_id < GWH89_PRESET_COUNT; ++preset_id) {
        if (!make_single_preset_file(
                names[preset_id],
                preset_id,
                0xA5A50000U + (gwh89_u32)(preset_id * 1237))) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    if (!make_showcase()) {
        fprintf(stderr, "Could not create WAV previews.\n");
        return 1;
    }

    printf("Created grocketwhistle89_showcase.wav and seven preset previews.\n");
    printf("Core state size: %lu bytes\n", (unsigned long)sizeof(gwh89_state));
    return 0;
}
