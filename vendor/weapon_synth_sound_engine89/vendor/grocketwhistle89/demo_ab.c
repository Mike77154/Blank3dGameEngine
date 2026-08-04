#include <stdio.h>
#include "grocketwhistle89.h"

#define AB_RATE 44100
#define AB_CHANNELS 2
#define AB_BITS 16
#define AB_BLOCK 256

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

static void write_header(FILE *file, gwh89_u32 frames)
{
    gwh89_u32 data_bytes;
    data_bytes = frames * AB_CHANNELS * (AB_BITS / 8);
    fwrite("RIFF", 1, 4, file);
    write_u32_le(file, 36U + data_bytes);
    fwrite("WAVEfmt ", 1, 8, file);
    write_u32_le(file, 16U);
    write_u16_le(file, 1U);
    write_u16_le(file, AB_CHANNELS);
    write_u32_le(file, AB_RATE);
    write_u32_le(file, AB_RATE * AB_CHANNELS * (AB_BITS / 8));
    write_u16_le(file, AB_CHANNELS * (AB_BITS / 8));
    write_u16_le(file, AB_BITS);
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

static gwh89_s32 write_silence(FILE *file, gwh89_s32 frames)
{
    gwh89_s32 i;
    for (i = 0; i < frames; ++i) {
        write_sample(file, 0);
        write_sample(file, 0);
    }
    return frames;
}

static gwh89_s32 render_version(FILE *file, gwh89_s32 airy)
{
    gwh89_state state;
    gwh89_preset preset;
    const gwh89_preset *source_preset;
    gwh89_s32 band;
    gwh89_s16 block[AB_BLOCK * 2];
    gwh89_s32 total;
    gwh89_s32 motion_total;
    gwh89_s32 rendered;
    gwh89_s32 count;
    gwh89_s32 progress_q15;
    gwh89_s32 velocity_mps;
    gwh89_s32 gain_q15;
    gwh89_s32 i;

    gwh89_init(&state, AB_RATE, 0x89ABCDEFU);
    source_preset = gwh89_get_preset(GWH89_PRESET_RPG7_SUSTAINED);
    if (source_preset == 0) {
        return 0;
    }
    preset = *source_preset;
    if (!airy) {
        preset.harmonic2_gain_q15 = 0;
        preset.harmonic3_gain_q15 = 0;
        preset.aerated_tone_gain_q15 = 0;
        preset.pitch_roughness_q15 = 0;
        preset.amplitude_roughness_q15 = 0;
        preset.air_spatial_mix_q15 = 0;
        for (band = 0; band < GWH89_OUTPUT_EQ_BANDS; ++band) {
            preset.output_eq_gain_q15[band] = GWH89_Q15_ONE;
        }
    }
    gwh89_trigger(&state, &preset);

    total = gwh89_get_estimated_total_samples(&state);
    motion_total = total - state.reverb_tail_samples;
    if (motion_total < 1) {
        motion_total = total;
    }

    rendered = 0;
    while (gwh89_is_active(&state)) {
        count = AB_BLOCK;
        if (rendered + count > total + AB_BLOCK) {
            count = total + AB_BLOCK - rendered;
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

int main(void)
{
    FILE *file;
    gwh89_s32 frames;
    file = fopen("preview_rpg7_electronic_vs_airy.wav", "wb");
    if (file == 0) {
        return 1;
    }
    write_header(file, 0U);
    frames = render_version(file, 0);
    frames += write_silence(file, AB_RATE / 2);
    frames += render_version(file, 1);
    fseek(file, 0L, SEEK_SET);
    write_header(file, (gwh89_u32)frames);
    fclose(file);
    printf("Created electronic-ish versus airy/dual-EQ A/B preview.\n");
    return 0;
}
