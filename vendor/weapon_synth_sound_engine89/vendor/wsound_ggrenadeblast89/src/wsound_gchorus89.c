#include "wsound_gchorus89.h"

void ws_gchorus_reset(ws_gchorus89 *chorus)
{
    int i;

    if (chorus == 0) {
        return;
    }
    for (i = 0; i < WS_GCHORUS89_MAX_DELAY; ++i) {
        chorus->delay[i] = 0;
    }
    chorus->write_pos = 0;
    chorus->lfo_phase = 0;
}

void ws_gchorus_init(ws_gchorus89 *chorus, ws_gu32 sample_rate)
{
    if (chorus == 0) {
        return;
    }
    ws_gchorus_reset(chorus);
    ws_gchorus_set(chorus, 529, 44, 370, 2500, 1500, sample_rate);
}

void ws_gchorus_set(ws_gchorus89 *chorus,
                    ws_gu16 base_delay,
                    ws_gu16 depth,
                    ws_gu16 rate_millihz,
                    ws_gs16 wet_q15,
                    ws_gs16 feedback_q15,
                    ws_gu32 sample_rate)
{
    ws_gu32 step;

    if (chorus == 0) {
        return;
    }
    if (base_delay < 2) {
        base_delay = 2;
    }
    if ((ws_gu32)base_delay + (ws_gu32)depth >= WS_GCHORUS89_MAX_DELAY) {
        if (base_delay >= WS_GCHORUS89_MAX_DELAY - 2) {
            base_delay = WS_GCHORUS89_MAX_DELAY - 2;
            depth = 1;
        } else {
            depth = (ws_gu16)(WS_GCHORUS89_MAX_DELAY - base_delay - 1);
        }
    }
    if (sample_rate < 8000UL) {
        sample_rate = 8000UL;
    }
    if (rate_millihz > 1000U) {
        rate_millihz = 1000U;
    }
    if (wet_q15 < 0) {
        wet_q15 = 0;
    }
    if (feedback_q15 > 30000) {
        feedback_q15 = 30000;
    }
    if (feedback_q15 < -30000) {
        feedback_q15 = -30000;
    }

    step = ((ws_gu32)rate_millihz * 4294967UL) / sample_rate;
    chorus->base_delay = base_delay;
    chorus->depth = depth;
    chorus->lfo_step = step & WS_GU32_MASK;
    chorus->wet_q15 = wet_q15;
    chorus->feedback_q15 = feedback_q15;
}

ws_gs16 ws_gchorus_process(ws_gchorus89 *chorus, ws_gs16 input)
{
    ws_gu32 phase_hi;
    ws_gu32 tri;
    ws_gu32 mod;
    ws_gu16 delay_len;
    ws_gu16 read_pos;
    ws_gs16 delayed;
    ws_gs32 write_value;
    ws_gs32 out;

    if (chorus == 0 || chorus->wet_q15 <= 0) {
        return input;
    }

    phase_hi = (chorus->lfo_phase >> 16) & 0xFFFFUL;
    if (phase_hi < 32768UL) {
        tri = phase_hi;
    } else {
        tri = 65535UL - phase_hi;
    }
    mod = (tri * chorus->depth) >> 15;
    delay_len = (ws_gu16)(chorus->base_delay + (ws_gu16)mod);

    if (chorus->write_pos >= delay_len) {
        read_pos = (ws_gu16)(chorus->write_pos - delay_len);
    } else {
        read_pos = (ws_gu16)(WS_GCHORUS89_MAX_DELAY + chorus->write_pos - delay_len);
    }
    delayed = chorus->delay[read_pos];

    write_value = (ws_gs32)input + ws_gmul_q15(delayed, chorus->feedback_q15);
    chorus->delay[chorus->write_pos] = ws_gclip16(write_value);
    chorus->write_pos = (ws_gu16)(chorus->write_pos + 1);
    if (chorus->write_pos >= WS_GCHORUS89_MAX_DELAY) {
        chorus->write_pos = 0;
    }

    chorus->lfo_phase = (chorus->lfo_phase + chorus->lfo_step) & WS_GU32_MASK;
    out = ws_gmul_q15(input, 32767 - chorus->wet_q15);
    out += ws_gmul_q15(delayed, chorus->wet_q15);
    return ws_gclip16(out);
}
