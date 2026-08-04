#include "wsound_geq6_89.h"

static ws_gs16 ws_geq6_alpha(ws_gu32 sample_rate, ws_gu16 cutoff_hz)
{
    ws_gu32 x_q15;
    ws_gu32 a_q15;

    if (sample_rate < 8000UL) {
        sample_rate = 8000UL;
    }

    x_q15 = ((ws_gu32)cutoff_hz * 205887UL) / sample_rate;
    a_q15 = (x_q15 * 32768UL) / (32768UL + x_q15);

    if (a_q15 > 32767UL) {
        a_q15 = 32767UL;
    }
    return (ws_gs16)a_q15;
}

void ws_geq6_init(ws_geq6_89 *eq, ws_gu32 sample_rate)
{
    static ws_gu16 cuts[5] = { 90, 250, 700, 1800, 5000 };
    int i;

    if (eq == 0) {
        return;
    }

    for (i = 0; i < 5; ++i) {
        eq->lp[i] = 0;
        eq->alpha_q15[i] = ws_geq6_alpha(sample_rate, cuts[i]);
    }
    for (i = 0; i < WS_GEQ6_BANDS; ++i) {
        eq->gain_q12[i] = WS_GQ12_ONE;
    }
}

void ws_geq6_reset(ws_geq6_89 *eq)
{
    int i;

    if (eq == 0) {
        return;
    }
    for (i = 0; i < 5; ++i) {
        eq->lp[i] = 0;
    }
}

void ws_geq6_set_gain_q12(ws_geq6_89 *eq, int band, ws_gs16 gain_q12)
{
    if (eq == 0) {
        return;
    }
    if (band < 0 || band >= WS_GEQ6_BANDS) {
        return;
    }
    eq->gain_q12[band] = gain_q12;
}

ws_gs16 ws_geq6_process(ws_geq6_89 *eq, ws_gs16 input)
{
    ws_gs32 x;
    ws_gs32 band[6];
    ws_gs32 sum;
    int i;

    if (eq == 0) {
        return input;
    }

    x = (ws_gs32)input;
    for (i = 0; i < 5; ++i) {
        eq->lp[i] += ws_gmul_q15(x - eq->lp[i], eq->alpha_q15[i]);
    }

    band[0] = eq->lp[0];
    band[1] = eq->lp[1] - eq->lp[0];
    band[2] = eq->lp[2] - eq->lp[1];
    band[3] = eq->lp[3] - eq->lp[2];
    band[4] = eq->lp[4] - eq->lp[3];
    band[5] = x - eq->lp[4];

    sum = 0;
    for (i = 0; i < 6; ++i) {
        sum += ws_gmul_q12(band[i], eq->gain_q12[i]);
    }
    return ws_gsoftlimit16(sum);
}
