#include "gshotgunsequence89.h"

#include <string.h>

#define TEST_FRAMES 35280U

static gss89_context test_ctx;
static gss89_s16 test_texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 test_texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 test_a[TEST_FRAMES];
static gss89_s16 test_b[TEST_FRAMES];

static unsigned long test_abs16(gss89_s16 value)
{
    return value < 0 ? (unsigned long)(-(long)value) : (unsigned long)value;
}

static int test_render(gss89_s16 *out, int rail_enabled,
                       unsigned long seed)
{
    gss89_mix mix;
    unsigned long i;

    if (gss89_init(&test_ctx, GSS89_SAMPLE_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&test_ctx, 0U, test_texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&test_ctx, 1U, test_texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;

    gss89_mix_default(&mix);
    mix.report_q15 = 0;
    mix.report_body_q15 = 0;
    mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0;
    mix.report_thump_q15 = 0;
    mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0;
    mix.foley_q15 = 0;
    mix.master_q15 = 30000;
    if (!rail_enabled) mix.pump_shotpumpkin_q15 = 0;
    gss89_set_mix(&test_ctx, &mix);

    if (gss89_trigger_pump(&test_ctx, GPUMP89_PRESET_WITH_SHELL,
        seed + 1UL) != GSS89_OK) return 0;
    for (i = 0UL; i < TEST_FRAMES; ++i) {
        out[i] = gss89_process_sample(&test_ctx);
    }
    return 1;
}

int main(void)
{
    unsigned long i;
    unsigned long legacy_energy;
    unsigned long triple_energy;
    unsigned long changed;

    memset(test_a, 0, sizeof(test_a));
    memset(test_b, 0, sizeof(test_b));
    if (!test_render(test_a, 0, 77154UL)) return 1;
    if (!test_render(test_b, 1, 77154UL)) return 2;

    legacy_energy = 0UL;
    triple_energy = 0UL;
    changed = 0UL;
    for (i = 0UL; i < TEST_FRAMES; ++i) {
        legacy_energy += test_abs16(test_a[i]);
        triple_energy += test_abs16(test_b[i]);
        if (test_a[i] != test_b[i]) changed++;
    }
    if (changed < 4000UL) return 3;
    if (triple_energy <= legacy_energy + legacy_energy / 25UL) return 4;

    /* Same seed and configuration must remain bit-identical. */
    if (!test_render(test_a, 1, 77154UL)) return 5;
    for (i = 0UL; i < TEST_FRAMES; ++i) {
        if (test_a[i] != test_b[i]) return 6;
    }
    return 0;
}
