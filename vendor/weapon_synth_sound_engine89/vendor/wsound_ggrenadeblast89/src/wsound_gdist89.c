#include "wsound_gdist89.h"

void ws_gdist_init(ws_gdist89 *dist)
{
    if (dist == 0) {
        return;
    }
    dist->drive_q8 = 256;
    dist->mix_q15 = 0;
}

void ws_gdist_set(ws_gdist89 *dist, ws_gs16 drive_q8, ws_gs16 mix_q15)
{
    if (dist == 0) {
        return;
    }
    if (drive_q8 < 1) {
        drive_q8 = 1;
    }
    if (mix_q15 < 0) {
        mix_q15 = 0;
    }
    if (drive_q8 > 2048) {
        drive_q8 = 2048;
    }
    dist->drive_q8 = drive_q8;
    dist->mix_q15 = mix_q15;
}

static ws_gs16 ws_gdist_shape(ws_gs32 x)
{
    ws_gs32 ax;
    ws_gs32 y;

    if (x < 0) {
        ax = -x;
    } else {
        ax = x;
    }

    if (ax <= 11000L) {
        y = ax;
    } else if (ax <= 26000L) {
        y = 11000L + ((ax - 11000L) >> 1);
    } else {
        y = 18500L + ((ax - 26000L) >> 3);
    }

    if (y > 30000L) {
        y = 30000L;
    }
    if (x < 0) {
        y = -y;
    }
    return ws_gclip16(y);
}

ws_gs16 ws_gdist_process(ws_gdist89 *dist, ws_gs16 input)
{
    ws_gs32 driven;
    ws_gs16 wet;
    ws_gs32 dry_mix;
    ws_gs32 wet_mix;

    if (dist == 0 || dist->mix_q15 <= 0) {
        return input;
    }

    driven = ((ws_gs32)input * dist->drive_q8) >> 8;
    wet = ws_gdist_shape(driven);
    dry_mix = ws_gmul_q15(input, 32767 - dist->mix_q15);
    wet_mix = ws_gmul_q15(wet, dist->mix_q15);
    return ws_gclip16(dry_mix + wet_mix);
}
