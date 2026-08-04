#include "gpump89.h"

int main(void)
{
    gpump89_context ctx;
    gpump89_params p;
    gpump89_i16 block[257];
    gpump89_u32 total;
    gpump89_u32 n;
    gpump89_u32 guard;
    int preset;

    for (preset = GPUMP89_PRESET_TIGHT_DRY;
         preset <= GPUMP89_PRESET_CINEMATIC_DRY;
         ++preset) {
        if (!gpump89_preset(&p, preset, 44100UL)) return 10 + preset;
        gpump89_init(&ctx, p.sample_rate, p.seed);
        gpump89_start_cycle(&ctx, &p);
        total = 0UL;
        guard = 0UL;
        while (gpump89_is_active(&ctx) && guard < 10000UL) {
            n = gpump89_render_i16(&ctx, block, 257UL);
            total += n;
            if (n == 0UL && gpump89_is_active(&ctx)) return 30 + preset;
            guard++;
        }
        if (total == 0UL || gpump89_is_active(&ctx)) return 50 + preset;
    }
    return 0;
}
