#ifndef WSOUND_GCHORUS89_H
#define WSOUND_GCHORUS89_H

#include "wsound_gtypes89.h"

#define WS_GCHORUS89_MAX_DELAY 2048

typedef struct ws_gchorus89_s {
    ws_gs16 delay[WS_GCHORUS89_MAX_DELAY];
    ws_gu16 write_pos;
    ws_gu16 base_delay;
    ws_gu16 depth;
    ws_gu32 lfo_phase;
    ws_gu32 lfo_step;
    ws_gs16 wet_q15;
    ws_gs16 feedback_q15;
} ws_gchorus89;

void ws_gchorus_init(ws_gchorus89 *chorus, ws_gu32 sample_rate);
void ws_gchorus_reset(ws_gchorus89 *chorus);
void ws_gchorus_set(ws_gchorus89 *chorus,
                    ws_gu16 base_delay,
                    ws_gu16 depth,
                    ws_gu16 rate_millihz,
                    ws_gs16 wet_q15,
                    ws_gs16 feedback_q15,
                    ws_gu32 sample_rate);
ws_gs16 ws_gchorus_process(ws_gchorus89 *chorus, ws_gs16 input);

#endif
