#include <stdio.h>
#include "wsound_ggrenadeblast89.h"

static ws_ggb89 g_test;

int main(void)
{
    ws_gu32 i;
    ws_gu32 hash;
    ws_gs16 sample;
    int p;

    hash = 2166136261UL;
    for (p = 0; p < WS_GGB89_PRESET_COUNT; ++p) {
        ws_ggb89_init(&g_test, 44100UL, 1234UL + (ws_gu32)p);
        ws_ggb89_trigger(&g_test, p, 32767);
        for (i = 0; i < 44100UL; ++i) {
            sample = ws_ggb89_process(&g_test);
            hash ^= (ws_gu16)sample;
            hash = (hash * 16777619UL) & WS_GU32_MASK;
        }
    }

    printf("wsound_ggrenadeblast89 test hash: %08lx\n",
           (unsigned long)(hash & WS_GU32_MASK));
    printf("context bytes: %lu\n", (unsigned long)sizeof(ws_ggb89));
    if (hash == 0UL) {
        return 1;
    }
    return 0;
}
