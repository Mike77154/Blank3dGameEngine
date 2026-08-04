#include <stdio.h>
#include "wsound_ggrenadeblast89.h"

static ws_ggb89 g_test;

int main(void)
{
    ws_gu32 i;
    ws_gs32 delta;
    ws_gs32 max_delta;
    ws_gs16 previous;
    ws_gs16 sample;
    int p;

    for (p = 0; p < WS_GGB89_PRESET_COUNT; ++p) {
        ws_ggb89_init(&g_test, 44100UL, 0x19890101UL + (ws_gu32)p * 313UL);
        ws_ggb89_trigger(&g_test, p, 32767);
        previous = 0;
        max_delta = 0;
        for (i = 0; i < 44100UL; ++i) {
            sample = ws_ggb89_process(&g_test);
            delta = (ws_gs32)sample - (ws_gs32)previous;
            if (delta < 0) {
                delta = -delta;
            }
            if (delta > max_delta) {
                max_delta = delta;
            }
            previous = sample;
        }
        printf("preset %d max sample jump: %ld\n", p, (long)max_delta);
        if (max_delta > 18000L) {
            return 1;
        }
    }
    return 0;
}
