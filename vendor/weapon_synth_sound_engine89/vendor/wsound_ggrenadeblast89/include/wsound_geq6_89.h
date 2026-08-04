#ifndef WSOUND_GEQ6_89_H
#define WSOUND_GEQ6_89_H

#include "wsound_gtypes89.h"

#define WS_GEQ6_BANDS 6

typedef struct ws_geq6_89_s {
    ws_gs32 lp[5];
    ws_gs16 alpha_q15[5];
    ws_gs16 gain_q12[WS_GEQ6_BANDS];
} ws_geq6_89;

void ws_geq6_init(ws_geq6_89 *eq, ws_gu32 sample_rate);
void ws_geq6_reset(ws_geq6_89 *eq);
void ws_geq6_set_gain_q12(ws_geq6_89 *eq, int band, ws_gs16 gain_q12);
ws_gs16 ws_geq6_process(ws_geq6_89 *eq, ws_gs16 input);

#endif
