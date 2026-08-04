#include "wmagazine89.h"

int main(void)
{
    static const int rates[] = {8000, 22050, 32000, 44100, 48000};
    WMag89State state;
    short block[256];
    int rate_index;
    int preset;
    int action;
    int index;
    int nonzero;
    WMag89Event custom[3];

    nonzero = 0;
    for (rate_index = 0;
         rate_index < (int)(sizeof(rates) / sizeof(rates[0]));
         ++rate_index) {
        if (wmag89_init(&state, rates[rate_index],
                        1U + (unsigned int)rate_index) != WMAG89_OK) {
            return 1;
        }
        for (preset = 0; preset < WMAG89_PRESET_COUNT; ++preset) {
            if (wmag89_set_preset(&state, preset) != WMAG89_OK) {
                return 2;
            }
            for (action = 0; action < WMAG89_ACTION_COUNT; ++action) {
                wmag89_reset(&state);
                if (wmag89_trigger(&state, action, 32767) != WMAG89_OK) {
                    return 3;
                }
                for (index = 0; index < 100; ++index) {
                    wmag89_process_block(&state, block, 256);
                    if (block[index & 255] != 0) {
                        nonzero = 1;
                    }
                }
            }
        }
    }

    custom[0].start_ms = 0;
    custom[0].duration_ms = 8;
    custom[0].gain_q15 = 26000;
    custom[0].type = WMAG89_EVENT_BURST;
    custom[0].color = 1;
    custom[1].start_ms = 12;
    custom[1].duration_ms = 25;
    custom[1].gain_q15 = 18000;
    custom[1].type = WMAG89_EVENT_SCRAPE;
    custom[1].color = 1;
    custom[2].start_ms = 38;
    custom[2].duration_ms = 9;
    custom[2].gain_q15 = 30000;
    custom[2].type = WMAG89_EVENT_CATCH;
    custom[2].color = 2;
    if (wmag89_trigger_custom(&state, custom, 3, 32767) != WMAG89_OK) {
        return 4;
    }
    wmag89_process_block(&state, block, 256);

    if (!nonzero) {
        return 5;
    }
    return 0;
}
