#include "wsound_gtypes89.h"

ws_gs16 ws_gclip16(ws_gs32 x)
{
    if (x > 32767L) {
        return (ws_gs16)32767;
    }
    if (x < -32768L) {
        return (ws_gs16)-32768;
    }
    return (ws_gs16)x;
}

ws_gs16 ws_gsoftlimit16(ws_gs32 x)
{
    ws_gs32 ax;
    ws_gs32 y;

    if (x < 0) {
        ax = -x;
    } else {
        ax = x;
    }

    if (ax <= 23500L) {
        y = ax;
    } else if (ax <= 47000L) {
        y = 23500L + ((ax - 23500L) >> 2);
    } else {
        y = 29375L + ((ax - 47000L) >> 5);
    }

    if (y > 31800L) {
        y = 31800L;
    }
    if (x < 0) {
        y = -y;
    }
    return (ws_gs16)y;
}

ws_gs32 ws_gmul_q15(ws_gs32 a, ws_gs32 b)
{
    return (a * b) >> 15;
}

ws_gs32 ws_gmul_q12(ws_gs32 a, ws_gs32 b)
{
    return (a * b) >> 12;
}
