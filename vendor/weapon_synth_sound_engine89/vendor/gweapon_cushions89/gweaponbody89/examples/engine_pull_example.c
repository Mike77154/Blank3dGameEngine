#include "gweaponbody89.h"

/* One-sample pull example. The host owns all storage. */
static gwb89_context ctx;

int example_start(void)
{
    gwb89_preset preset;
    if (!gwb89_get_preset(GWB89_PRESET_SHOTGUN, &preset)) return 0;
    if (!gwb89_init(&ctx, 44100U, &preset, 1U)) return 0;
    gwb89_trigger(&ctx, 30000, 2U);
    return 1;
}

signed short example_pull(signed short input_sample)
{
    return gwb89_process_sample(&ctx, input_sample);
}
