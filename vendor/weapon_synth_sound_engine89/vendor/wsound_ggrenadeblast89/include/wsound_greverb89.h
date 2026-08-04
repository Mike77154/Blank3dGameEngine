#ifndef WSOUND_GREVERB89_H
#define WSOUND_GREVERB89_H

#include "wsound_gtypes89.h"

#define WS_GREVERB89_COMBS 3
#define WS_GREVERB89_COMB_MAX 4096
#define WS_GREVERB89_AP_MAX 2048

typedef struct ws_greverb89_s {
    ws_gs16 comb[WS_GREVERB89_COMBS][WS_GREVERB89_COMB_MAX];
    ws_gs16 allpass[WS_GREVERB89_AP_MAX];
    ws_gu16 comb_pos[WS_GREVERB89_COMBS];
    ws_gu16 comb_len[WS_GREVERB89_COMBS];
    ws_gu16 ap_pos;
    ws_gu16 ap_len;
    ws_gs16 feedback_q15;
    ws_gs16 damp_q15;
    ws_gs16 wet_q15;
    ws_gs32 damp_state[WS_GREVERB89_COMBS];
} ws_greverb89;

void ws_greverb_init(ws_greverb89 *reverb, ws_gu32 sample_rate);
void ws_greverb_reset(ws_greverb89 *reverb);
void ws_greverb_set(ws_greverb89 *reverb,
                    ws_gs16 feedback_q15,
                    ws_gs16 damp_q15,
                    ws_gs16 wet_q15);
ws_gs16 ws_greverb_process(ws_greverb89 *reverb, ws_gs16 input);

#endif
