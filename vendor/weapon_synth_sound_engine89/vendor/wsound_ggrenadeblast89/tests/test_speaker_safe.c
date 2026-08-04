#include <stdio.h>
#include "wsound_ggrenadeblast89.h"

static ws_ggb89 g_test;

static ws_gu32 ws_abs32(ws_gs32 x)
{
    if (x < 0) {
        return (ws_gu32)(-x);
    }
    return (ws_gu32)x;
}

static int check_preset(int preset, ws_gu32 max_mean_delta,
                        ws_gu32 max_mean_second)
{
    ws_gu32 i;
    ws_gu32 count;
    ws_gu32 sum_delta;
    ws_gu32 sum_second;
    ws_gs32 delta;
    ws_gs32 previous_delta;
    ws_gs16 previous;
    ws_gs16 sample;
    ws_gu32 mean_delta;
    ws_gu32 mean_second;

    ws_ggb89_init(&g_test, 44100UL,
                  0x19890101UL + (ws_gu32)preset * 313UL);
    ws_ggb89_trigger(&g_test, preset, 32767);

    previous = 0;
    previous_delta = 0;
    sum_delta = 0UL;
    sum_second = 0UL;
    count = 0UL;

    for (i = 0; i < 22050UL; ++i) {
        sample = ws_ggb89_process(&g_test);
        delta = (ws_gs32)sample - (ws_gs32)previous;
        sum_delta += ws_abs32(delta);
        if (i > 0UL) {
            sum_second += ws_abs32(delta - previous_delta);
            count += 1UL;
        }
        previous_delta = delta;
        previous = sample;
    }

    if (count == 0UL) {
        return 0;
    }
    mean_delta = sum_delta / 22050UL;
    mean_second = sum_second / count;

    printf("preset %d mean |dx|: %lu, mean |d2x|: %lu\n",
           preset, (unsigned long)mean_delta,
           (unsigned long)mean_second);

    if (mean_delta > max_mean_delta || mean_second > max_mean_second) {
        return 0;
    }
    if (mean_delta < 20UL || mean_second < 20UL) {
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!check_preset(WS_GGB89_40MM_HEDP_HARD, 220UL, 220UL)) {
        return 1;
    }
    if (!check_preset(WS_GGB89_CONCRETE_IMPACT, 260UL, 260UL)) {
        return 1;
    }
    return 0;
}
