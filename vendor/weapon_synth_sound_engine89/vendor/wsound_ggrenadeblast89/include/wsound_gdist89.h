#ifndef WSOUND_GDIST89_H
#define WSOUND_GDIST89_H

#include "wsound_gtypes89.h"

typedef struct ws_gdist89_s {
    ws_gs16 drive_q8;
    ws_gs16 mix_q15;
} ws_gdist89;

void ws_gdist_init(ws_gdist89 *dist);
void ws_gdist_set(ws_gdist89 *dist, ws_gs16 drive_q8, ws_gs16 mix_q15);
ws_gs16 ws_gdist_process(ws_gdist89 *dist, ws_gs16 input);

#endif
