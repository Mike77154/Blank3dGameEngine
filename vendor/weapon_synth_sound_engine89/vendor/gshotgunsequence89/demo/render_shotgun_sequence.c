#include <stdio.h>
#include "gshotgunsequence89.h"

#define DEMO_FRAMES 270004UL
#define PUMP_FRAMES 35280UL
#define AB_GAP 11025UL
#define AB_FRAMES 81600UL
#define STEMS_FRAMES 132300UL

static gss89_context seq;
static gss89_s16 texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 mono[DEMO_FRAMES];
static gss89_s16 stereo[DEMO_FRAMES * 2UL];
static gss89_s16 old_pair[PUMP_FRAMES];
static gss89_s16 full_pump[PUMP_FRAMES];
static gss89_s16 klek_stem[PUMP_FRAMES];
static gss89_s16 tik_stem[PUMP_FRAMES];
static gss89_s16 ab[AB_FRAMES];
static gss89_s16 stems[STEMS_FRAMES];

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

static int write_wav(const char *path, const gss89_s16 *pcm,
                     unsigned long frames, int channels)
{
    FILE *f;
    unsigned long i;
    unsigned long count;
    unsigned long bytes;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    count = frames * (unsigned long)channels;
    bytes = count * 2UL;
    fwrite("RIFF", 1U, 4U, f); put_u32(f, 36UL + bytes);
    fwrite("WAVEfmt ", 1U, 8U, f); put_u32(f, 16UL);
    put_u16(f, 1UL); put_u16(f, (unsigned long)channels);
    put_u32(f, GSS89_SAMPLE_RATE);
    put_u32(f, GSS89_SAMPLE_RATE * 2UL * (unsigned long)channels);
    put_u16(f, 2UL * (unsigned long)channels); put_u16(f, 16UL);
    fwrite("data", 1U, 4U, f); put_u32(f, bytes);
    for (i = 0UL; i < count; ++i) put_u16(f, (unsigned short)pcm[i]);
    fclose(f);
    return 1;
}

static unsigned long at_ms(unsigned long ms)
{
    return (GSS89_SAMPLE_RATE * ms) / 1000UL;
}

static int setup(unsigned long seed)
{
    if (gss89_init(&seq, GSS89_SAMPLE_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&seq, 0U, texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&seq, 1U, texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    return 1;
}

static int render_full_sequence(void)
{
    unsigned long i;
    if (!setup(0x53455132UL)) return 0;
    for (i = 0UL; i < DEMO_FRAMES; ++i) {
        if (i == at_ms(320UL) && gss89_trigger_chuecka(&seq,
            CH89_PRESET_SHOTGUN_MULTI_INSERT, 6U, 32767U, 1001U) != GSS89_OK) return 0;
        if (i == at_ms(1100UL) && gss89_trigger_chuecka(&seq,
            CH89_PRESET_SHOTGUN_INSERT, 1U, 32767U, 1002U) != GSS89_OK) return 0;
        if (i == at_ms(1420UL) && gss89_trigger_pump(&seq,
            GPUMP89_PRESET_HEAVY, 1003U) != GSS89_OK) return 0;
        if (i == at_ms(2030UL)) gss89_trigger_report(&seq, 2001U);
        if (i == at_ms(3080UL) && gss89_trigger_pump(&seq,
            GPUMP89_PRESET_WITH_SHELL, 2002U) != GSS89_OK) return 0;
        if (i == at_ms(3650UL)) gss89_trigger_report(&seq, 3001U);
        if (i == at_ms(4780UL) && gss89_trigger_pump(&seq,
            GPUMP89_PRESET_WITH_SHELL, 3002U) != GSS89_OK) return 0;
        if (i == at_ms(5220UL)) gss89_trigger_foley(&seq, GWF89_SHOTGUN_EMPTY, 4001U);
        if (i == at_ms(5620UL)) gss89_trigger_foley(&seq, GWF89_SHOTGUN_SAFETY, 4002U);
        mono[i] = gss89_process_sample(&seq);
        stereo[i * 2UL] = mono[i];
        stereo[i * 2UL + 1UL] = mono[i];
    }
    return write_wav("audio/00_shotgun_sequence_with_mid_klek_tik.wav",
                     stereo, DEMO_FRAMES, 2);
}

static unsigned long render_variant(gss89_s16 *dst, int primary,
                                    int chuecka, int klek, int tik,
                                    unsigned long seed)
{
    gss89_mix mix;
    unsigned long i;
    if (!setup(seed)) return 0UL;
    gss89_mix_default(&mix);
    mix.report_q15 = 0; mix.report_body_q15 = 0; mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0; mix.report_thump_q15 = 0; mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0; mix.foley_q15 = 0; mix.master_q15 = 32767;
    if (!primary) mix.pump_primary_q15 = 0;
    if (!chuecka) mix.pump_chuecka_q15 = 0;
    if (!klek) mix.pump_klek_q15 = 0;
    if (!tik) mix.pump_tik_q15 = 0;
    gss89_set_mix(&seq, &mix);
    if (gss89_trigger_pump(&seq, GPUMP89_PRESET_WITH_SHELL,
                            seed + 1UL) != GSS89_OK) return 0UL;
    for (i = 0UL; i < PUMP_FRAMES; ++i) dst[i] = gss89_process_sample(&seq);
    return PUMP_FRAMES;
}

static int render_pump_auditions(void)
{
    unsigned long i;
    unsigned long start;
    if (!render_variant(old_pair, 1, 1, 0, 0, 5000UL)) return 0;
    if (!render_variant(full_pump, 1, 1, 1, 1, 5000UL)) return 0;
    if (!render_variant(klek_stem, 0, 0, 1, 0, 5000UL)) return 0;
    if (!render_variant(tik_stem, 0, 0, 0, 1, 5000UL)) return 0;

    if (!write_wav("audio/01_pump_full_chic_klek_tik_chuk.wav", full_pump, PUMP_FRAMES, 1)) return 0;
    if (!write_wav("audio/02_pump_v1_1_without_midpoint.wav", old_pair, PUMP_FRAMES, 1)) return 0;
    if (!write_wav("audio/03_gklek89_midpoint_stem.wav", klek_stem, PUMP_FRAMES, 1)) return 0;
    if (!write_wav("audio/04_foley_tik_midpoint_stem.wav", tik_stem, PUMP_FRAMES, 1)) return 0;

    for (i = 0UL; i < AB_FRAMES; ++i) ab[i] = 0;
    for (i = 0UL; i < PUMP_FRAMES; ++i) ab[i] = old_pair[i];
    start = PUMP_FRAMES + AB_GAP;
    for (i = 0UL; i < PUMP_FRAMES && start + i < AB_FRAMES; ++i) ab[start + i] = full_pump[i];
    if (!write_wav("audio/05_AB_without_midpoint_then_klek_plus_tik.wav", ab, AB_FRAMES, 1)) return 0;

    for (i = 0UL; i < STEMS_FRAMES; ++i) stems[i] = 0;
    start = 4410UL;
    for (i = 0UL; i < PUMP_FRAMES; ++i) stems[start + i] = klek_stem[i];
    start = 44100UL;
    for (i = 0UL; i < PUMP_FRAMES; ++i) stems[start + i] = tik_stem[i];
    start = 83790UL;
    for (i = 0UL; i < PUMP_FRAMES && start + i < STEMS_FRAMES; ++i) stems[start + i] = full_pump[i];
    return write_wav("audio/06_klek_then_tik_then_full_pump.wav", stems, STEMS_FRAMES, 1);
}

int main(void)
{
    if (!render_full_sequence()) return 1;
    if (!render_pump_auditions()) return 2;
    printf("gshotgunsequence89 v1.2 rendered; context=%lu bytes\n",
           (unsigned long)gss89_context_bytes());
    return 0;
}
