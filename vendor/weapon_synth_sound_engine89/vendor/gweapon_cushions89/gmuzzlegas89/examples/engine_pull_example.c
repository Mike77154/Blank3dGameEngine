#include "gmuzzlegas89.h"

/* One-sample pull example. The host owns all storage. */
static gmg89_context ctx;

int example_start(void)
{
    gmg89_preset preset;
    if (!gmg89_get_preset(GMG89_PRESET_SHOTGUN, &preset)) return 0;
    if (!gmg89_init(&ctx, 44100U, &preset, 1U)) return 0;
    gmg89_trigger(&ctx, 30000, 2U);
    return 1;
}

signed short example_pull(signed short input_sample)
{
    (void)input_sample;
    return gmg89_process_sample(&ctx);
}
