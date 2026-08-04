#include "wsound_greverb89.h"

static ws_gu16 ws_greverb_scaled_len(ws_gu32 sample_rate, ws_gu16 base)
{
    ws_gu32 value;

    value = ((ws_gu32)base * sample_rate) / 44100UL;
    if (value < 17UL) {
        value = 17UL;
    }
    if (value >= WS_GREVERB89_COMB_MAX) {
        value = WS_GREVERB89_COMB_MAX - 1;
    }
    return (ws_gu16)value;
}

void ws_greverb_reset(ws_greverb89 *reverb)
{
    int c;
    int i;

    if (reverb == 0) {
        return;
    }

    for (c = 0; c < WS_GREVERB89_COMBS; ++c) {
        for (i = 0; i < WS_GREVERB89_COMB_MAX; ++i) {
            reverb->comb[c][i] = 0;
        }
        reverb->comb_pos[c] = 0;
        reverb->damp_state[c] = 0;
    }
    for (i = 0; i < WS_GREVERB89_AP_MAX; ++i) {
        reverb->allpass[i] = 0;
    }
    reverb->ap_pos = 0;
}

void ws_greverb_init(ws_greverb89 *reverb, ws_gu32 sample_rate)
{
    if (reverb == 0) {
        return;
    }

    reverb->comb_len[0] = ws_greverb_scaled_len(sample_rate, 1559);
    reverb->comb_len[1] = ws_greverb_scaled_len(sample_rate, 1949);
    reverb->comb_len[2] = ws_greverb_scaled_len(sample_rate, 2381);
    reverb->ap_len = ws_greverb_scaled_len(sample_rate, 613);
    if (reverb->ap_len >= WS_GREVERB89_AP_MAX) {
        reverb->ap_len = WS_GREVERB89_AP_MAX - 1;
    }

    reverb->feedback_q15 = 24500;
    reverb->damp_q15 = 12000;
    reverb->wet_q15 = 5500;
    ws_greverb_reset(reverb);
}

void ws_greverb_set(ws_greverb89 *reverb,
                    ws_gs16 feedback_q15,
                    ws_gs16 damp_q15,
                    ws_gs16 wet_q15)
{
    if (reverb == 0) {
        return;
    }
    if (feedback_q15 < 0) {
        feedback_q15 = 0;
    }
    if (feedback_q15 > 31500) {
        feedback_q15 = 31500;
    }
    if (damp_q15 < 0) {
        damp_q15 = 0;
    }
    if (wet_q15 < 0) {
        wet_q15 = 0;
    }
    reverb->feedback_q15 = feedback_q15;
    reverb->damp_q15 = damp_q15;
    reverb->wet_q15 = wet_q15;
}

ws_gs16 ws_greverb_process(ws_greverb89 *reverb, ws_gs16 input)
{
    ws_gs32 comb_sum;
    ws_gs32 delayed;
    ws_gs32 filtered;
    ws_gs32 write_value;
    ws_gs32 ap_delayed;
    ws_gs32 ap_out;
    ws_gs32 out;
    int c;

    if (reverb == 0 || reverb->wet_q15 <= 0) {
        return input;
    }

    comb_sum = 0;
    for (c = 0; c < WS_GREVERB89_COMBS; ++c) {
        delayed = reverb->comb[c][reverb->comb_pos[c]];
        reverb->damp_state[c] += ws_gmul_q15(delayed - reverb->damp_state[c],
                                             reverb->damp_q15);
        filtered = reverb->damp_state[c];
        write_value = (ws_gs32)input +
                      ws_gmul_q15(filtered, reverb->feedback_q15);
        reverb->comb[c][reverb->comb_pos[c]] = ws_gclip16(write_value);
        reverb->comb_pos[c] = (ws_gu16)(reverb->comb_pos[c] + 1);
        if (reverb->comb_pos[c] >= reverb->comb_len[c]) {
            reverb->comb_pos[c] = 0;
        }
        comb_sum += delayed;
    }
    comb_sum /= WS_GREVERB89_COMBS;

    ap_delayed = reverb->allpass[reverb->ap_pos];
    ap_out = ap_delayed - comb_sum;
    reverb->allpass[reverb->ap_pos] =
        ws_gclip16(comb_sum + ws_gmul_q15(ap_delayed, 16384));
    reverb->ap_pos = (ws_gu16)(reverb->ap_pos + 1);
    if (reverb->ap_pos >= reverb->ap_len) {
        reverb->ap_pos = 0;
    }

    out = ws_gmul_q15(input, 32767 - reverb->wet_q15);
    out += ws_gmul_q15(ap_out, reverb->wet_q15);
    return ws_gclip16(out);
}
