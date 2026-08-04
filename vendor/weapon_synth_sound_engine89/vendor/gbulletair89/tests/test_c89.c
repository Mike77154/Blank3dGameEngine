#include "gbulletair89.h"

int main(void)
{
    gba89_state state;
    gba89_s16 samples[512];
    int p;

    gba89_init(&state, 1UL);
    for (p = 0; p < GBA89_PRESET_COUNT; ++p) {
        gba89_trigger_preset(&state, p);
        gba89_render_stereo(&state, samples, 256UL);
    }
    return 0;
}
