#include "gshotgunsequence89.h"

static gss89_context ctx;
static gss89_s16 texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 output[35280];

static unsigned long abs16(gss89_s16 x)
{
    return x < 0 ? (unsigned long)(-(long)x) : (unsigned long)x;
}

static int setup(unsigned long seed)
{
    if (gss89_init(&ctx, GSS89_SAMPLE_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&ctx, 0U, texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&ctx, 1U, texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    return 1;
}

static unsigned long render_sum(int primary, int harmonic, int rail, int klek, int tik,
                                unsigned long start, unsigned long end)
{
    gss89_mix mix;
    unsigned long i;
    unsigned long sum;
    if (!setup(60000UL)) return 0UL;
    gss89_mix_default(&mix);
    mix.report_q15 = 0; mix.report_body_q15 = 0; mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0; mix.report_thump_q15 = 0; mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0; mix.foley_q15 = 0; mix.master_q15 = 32767;
    if (!primary) mix.pump_primary_q15 = 0;
    if (!harmonic) mix.pump_chuecka_q15 = 0;
    if (!rail) mix.pump_shotpumpkin_q15 = 0;
    if (!klek) mix.pump_klek_q15 = 0;
    if (!tik) mix.pump_tik_q15 = 0;
    gss89_set_mix(&ctx, &mix);
    if (gss89_trigger_pump(&ctx, GPUMP89_PRESET_WITH_SHELL, 60001UL) != GSS89_OK) return 0UL;
    gss89_render_mono(&ctx, output, 35280UL);
    sum = 0UL;
    for (i = start; i < end; ++i) sum += abs16(output[i]);
    return sum;
}

int main(void)
{
    unsigned long i;
    unsigned long primary_sum;
    unsigned long harmonic_sum;
    unsigned long rail_sum;
    unsigned long valley_old;
    unsigned long valley_new;

    if (!setup(12345UL)) return 1;
    for (i = 0UL; i < 256UL; ++i) if (gss89_process_sample(&ctx) != 0) return 2;

    primary_sum = render_sum(1, 0, 0, 0, 0, 0UL, 35280UL);
    harmonic_sum = render_sum(0, 1, 0, 0, 0, 0UL, 35280UL);
    rail_sum = render_sum(0, 0, 1, 0, 0, 0UL, 35280UL);
    if (primary_sum == 0UL || harmonic_sum < primary_sum || rail_sum == 0UL) return 3;

    /* 240-340 ms is the old energy valley between the two pump contacts. */
    valley_old = render_sum(1, 1, 0, 0, 0, 10584UL, 14994UL);
    valley_new = render_sum(1, 1, 1, 1, 1, 10584UL, 14994UL);
    if (valley_new <= valley_old + valley_old / 5UL) return 4;
    return 0;
}
