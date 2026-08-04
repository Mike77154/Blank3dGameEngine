#include "gshotgunsequence89.h"

#include <string.h>

#define TSM_FRAMES 35280U

static gss89_context tsm_ctx;
static gss89_s16 tsm_texture0[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 tsm_texture1[GSS89_RECOMMENDED_TEXTURE_FRAMES];
static gss89_s16 tsm_a[TSM_FRAMES];
static gss89_s16 tsm_b[TSM_FRAMES];
static gss89_s16 tsm_c[TSM_FRAMES];

static int tsm_tek_triggered;
static unsigned int tsm_tek_left;

static void tsm_tek_trigger(void *user, gss89_u32 seed,
                            gss89_u16 delay_ms,
                            gss89_shotgun_model model)
{
    unsigned int *marker;
    marker = (unsigned int *)user;
    *marker = seed ^ (unsigned int)delay_ms ^ (unsigned int)model;
    tsm_tek_left = 96U;
    tsm_tek_triggered++;
}

static gss89_s16 tsm_tek_process(void *user)
{
    unsigned int *marker;
    marker = (unsigned int *)user;
    if (tsm_tek_left == 0U) return 0;
    tsm_tek_left--;
    return (gss89_s16)((*marker & 1U) ? 1400 : -1400);
}

static int tsm_tek_active(const void *user)
{
    (void)user;
    return tsm_tek_left != 0U;
}

static unsigned long tsm_abs16(gss89_s16 x)
{
    return x < 0 ? (unsigned long)(-(long)x) : (unsigned long)x;
}

static int tsm_render(gss89_s16 *out, gss89_shotgun_model model,
                      unsigned long seed, int master_enabled,
                      int use_tek)
{
    gss89_mix mix;
    unsigned long i;
    unsigned int marker;

    marker = 0U;
    if (gss89_init(&tsm_ctx, GSS89_SAMPLE_RATE, seed) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&tsm_ctx, 0U, tsm_texture0,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    if (gss89_bind_texture_buffer(&tsm_ctx, 1U, tsm_texture1,
        GSS89_RECOMMENDED_TEXTURE_FRAMES) != GSS89_OK) return 0;
    gss89_set_shotgun_model(&tsm_ctx, model);
    mix = tsm_ctx.mix;
    mix.report_q15 = 0;
    mix.report_body_q15 = 0;
    mix.report_gas_q15 = 0;
    mix.report_crack_q15 = 0;
    mix.report_thump_q15 = 0;
    mix.report_tail_q15 = 0;
    mix.general_chuecka_q15 = 0;
    mix.foley_q15 = 0;
    gss89_set_mix(&tsm_ctx, &mix);
    if (use_tek) {
        gss89_set_tek_provider(&tsm_ctx, tsm_tek_trigger,
            tsm_tek_process, tsm_tek_active, &marker);
    }
    if (gss89_trigger_model_pump(&tsm_ctx, model, seed + 1UL) != GSS89_OK) {
        return 0;
    }
    gss89_enable_pump_master(&tsm_ctx, master_enabled);
    for (i = 0UL; i < TSM_FRAMES; ++i) out[i] = gss89_process_sample(&tsm_ctx);
    return 1;
}

int main(void)
{
    unsigned long i;
    unsigned long changed;
    unsigned long energy_a;
    unsigned long energy_b;

    memset(tsm_a, 0, sizeof(tsm_a));
    memset(tsm_b, 0, sizeof(tsm_b));
    memset(tsm_c, 0, sizeof(tsm_c));

    if (!tsm_render(tsm_a, GSS89_SHOTGUN_MODEL_REMINGTON_870,
                    7716200UL, 1, 0)) return 1;
    if (!tsm_render(tsm_b, GSS89_SHOTGUN_MODEL_REMINGTON_870,
                    7716200UL, 1, 0)) return 2;
    for (i = 0UL; i < TSM_FRAMES; ++i) {
        if (tsm_a[i] != tsm_b[i]) return 3;
    }

    if (!tsm_render(tsm_b, GSS89_SHOTGUN_MODEL_REMINGTON_870,
                    7716200UL, 0, 0)) return 4;
    changed = 0UL;
    for (i = 0UL; i < TSM_FRAMES; ++i) {
        if (tsm_a[i] != tsm_b[i]) changed++;
    }
    if (changed < 2000UL) return 5;

    if (!tsm_render(tsm_c, GSS89_SHOTGUN_MODEL_MOSSBERG_500_590,
                    7716200UL, 1, 0)) return 6;
    changed = 0UL;
    energy_a = 0UL;
    energy_b = 0UL;
    for (i = 0UL; i < TSM_FRAMES; ++i) {
        if (tsm_a[i] != tsm_c[i]) changed++;
        energy_a += tsm_abs16(tsm_a[i]);
        energy_b += tsm_abs16(tsm_c[i]);
    }
    if (changed < 5000UL) return 7;
    if (energy_a == energy_b) return 8;

    tsm_tek_triggered = 0;
    tsm_tek_left = 0U;
    if (!tsm_render(tsm_c, GSS89_SHOTGUN_MODEL_WINCHESTER_SXP,
                    7716201UL, 1, 1)) return 9;
    if (tsm_tek_triggered != 1) return 10;
    return 0;
}
