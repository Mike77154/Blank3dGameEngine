#include "gshotgunsequence89.h"

#include <stdio.h>
#include <string.h>

#define PLS_RATE 44100U
#define PLS_AB_SECONDS 8U
#define PLS_TRIPLE_SECONDS 4U
#define PLS_AB_FRAMES (PLS_RATE * PLS_AB_SECONDS)
#define PLS_TRIPLE_FRAMES (PLS_RATE * PLS_TRIPLE_SECONDS)
#define PLS_RENDER_FRAMES 39690U

static gss89_s16 pls_ab[PLS_AB_FRAMES * 2U];
static gss89_s16 pls_triple[PLS_TRIPLE_FRAMES * 2U];
static gss89_s16 pls_mono[PLS_RENDER_FRAMES];
static gss89_s16 pls_texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 pls_texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_context pls_ctx;

static gss89_s16 pls_sat16(gss89_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (gss89_s16)value;
}

static void pls_put_u16le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void pls_put_u32le(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
    fputc((int)((value >> 16) & 255U), file);
    fputc((int)((value >> 24) & 255U), file);
}

static int pls_write_wav(const char *path, const gss89_s16 *stereo,
                         unsigned int frames)
{
    FILE *file;
    unsigned int i;
    unsigned int samples;
    unsigned int data_bytes;

    file = fopen(path, "wb");
    if (file == 0) return 0;
    samples = frames * 2U;
    data_bytes = samples * 2U;

    fwrite("RIFF", 1U, 4U, file);
    pls_put_u32le(file, 36U + data_bytes);
    fwrite("WAVE", 1U, 4U, file);
    fwrite("fmt ", 1U, 4U, file);
    pls_put_u32le(file, 16U);
    pls_put_u16le(file, 1U);
    pls_put_u16le(file, 2U);
    pls_put_u32le(file, PLS_RATE);
    pls_put_u32le(file, PLS_RATE * 4U);
    pls_put_u16le(file, 4U);
    pls_put_u16le(file, 16U);
    fwrite("data", 1U, 4U, file);
    pls_put_u32le(file, data_bytes);

    for (i = 0U; i < samples; ++i) {
        pls_put_u16le(file, (unsigned int)(unsigned short)stereo[i]);
    }
    fclose(file);
    return 1;
}

static int pls_setup(unsigned int seed, const gss89_mix *mix,
                     sp89_preset shotpump_preset)
{
    if (gss89_init(&pls_ctx, PLS_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&pls_ctx, 0U, pls_texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&pls_ctx, 1U, pls_texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    gss89_set_mix(&pls_ctx, mix);
    gss89_set_shotpumpkin_preset(&pls_ctx, shotpump_preset);
    return 1;
}

static int pls_render_layer(gss89_s16 *dst, unsigned int dst_frames,
                            unsigned int start_frame, const gss89_mix *mix,
                            int gpump_preset, sp89_preset shotpump_preset,
                            unsigned int seed)
{
    unsigned int i;
    unsigned int frame;
    gss89_s32 value;

    if (!pls_setup(seed, mix, shotpump_preset)) return 0;
    if (gss89_trigger_pump(&pls_ctx, gpump_preset, seed + 1U) != GSS89_OK) {
        return 0;
    }
    memset(pls_mono, 0, sizeof(pls_mono));
    gss89_render_mono(&pls_ctx, pls_mono, PLS_RENDER_FRAMES);

    for (i = 0U; i < PLS_RENDER_FRAMES; ++i) {
        frame = start_frame + i;
        if (frame >= dst_frames) break;
        value = (gss89_s32)dst[frame * 2U] + (gss89_s32)pls_mono[i];
        dst[frame * 2U] = pls_sat16(value);
        dst[frame * 2U + 1U] = dst[frame * 2U];
    }
    return 1;
}

static void pls_silence_shot(gss89_mix *mix)
{
    mix->report_q15 = 0;
    mix->report_body_q15 = 0;
    mix->report_gas_q15 = 0;
    mix->report_crack_q15 = 0;
    mix->report_thump_q15 = 0;
    mix->report_tail_q15 = 0;
    mix->general_chuecka_q15 = 0;
    mix->foley_q15 = 0;
    mix->master_q15 = 30000;
}

static int pls_render_ab(void)
{
    gss89_mix mix;

    memset(pls_ab, 0, sizeof(pls_ab));

    /* 0.40 s: gpump89 structural stages only. */
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);
    mix.pump_primary_q15 = 32767;
    mix.pump_chuecka_q15 = 0;
    mix.pump_shotpumpkin_q15 = 0;
    mix.pump_klek_q15 = 0;
    mix.pump_tik_q15 = 0;
    if (!pls_render_layer(pls_ab, PLS_AB_FRAMES, 17640U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715401U)) return 0;

    /* 1.75 s: chuecka89 bright articulation only. */
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);
    mix.pump_primary_q15 = 0;
    mix.pump_chuecka_q15 = 30000;
    mix.pump_shotpumpkin_q15 = 0;
    mix.pump_klek_q15 = 0;
    mix.pump_tik_q15 = 0;
    if (!pls_render_layer(pls_ab, PLS_AB_FRAMES, 77175U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715402U)) return 0;

    /* 3.10 s: shotpumpkin89 rail/friction texture only. */
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);
    mix.pump_primary_q15 = 0;
    mix.pump_chuecka_q15 = 0;
    mix.pump_shotpumpkin_q15 = 18500;
    mix.pump_klek_q15 = 0;
    mix.pump_tik_q15 = 0;
    if (!pls_render_layer(pls_ab, PLS_AB_FRAMES, 136710U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715403U)) return 0;

    /* 4.45 s: legacy complete pair plus midpoint articulation. */
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);
    mix.pump_shotpumpkin_q15 = 0;
    if (!pls_render_layer(pls_ab, PLS_AB_FRAMES, 196245U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715404U)) return 0;

    /* 5.80 s: complete triple-layer pump. */
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);
    if (!pls_render_layer(pls_ab, PLS_AB_FRAMES, 255780U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715405U)) return 0;

    return pls_write_wav("audio/wsse89_v1_6_1_pump_layers_ab.wav",
                         pls_ab, PLS_AB_FRAMES);
}

static int pls_render_triple(void)
{
    gss89_mix mix;

    memset(pls_triple, 0, sizeof(pls_triple));
    gss89_mix_default(&mix);
    pls_silence_shot(&mix);

    /* Normal service pump. */
    if (!pls_render_layer(pls_triple, PLS_TRIPLE_FRAMES, 17640U, &mix,
        GPUMP89_PRESET_WITH_SHELL, SP89_PRESET_REALISTIC, 7715411U)) return 0;

    /* Heavier action with the matching shotpumpkin heavy texture. */
    mix.pump_shotpumpkin_q15 = 12000;
    mix.master_q15 = 28500;
    if (!pls_render_layer(pls_triple, PLS_TRIPLE_FRAMES, 83790U, &mix,
        GPUMP89_PRESET_HEAVY, SP89_PRESET_HEAVY, 7715412U)) return 0;

    return pls_write_wav("audio/wsse89_v1_6_1_triple_pump_complete.wav",
                         pls_triple, PLS_TRIPLE_FRAMES);
}

int main(void)
{
    if (!pls_render_ab()) {
        fprintf(stderr, "could not render pump layer A/B\n");
        return 1;
    }
    if (!pls_render_triple()) {
        fprintf(stderr, "could not render triple pump\n");
        return 2;
    }
    printf("wrote pump layer A/B and triple pump previews\n");
    printf("gss89 context bytes: %lu\n",
           (unsigned long)gss89_context_bytes());
    return 0;
}
