#include "gcinemathump89.h"

/* One-sample pull example. The host owns all storage. */
static gct89_context ctx;

int example_start(void)
{
    gct89_preset preset;
    if (!gct89_get_preset(GCT89_PRESET_SHOTGUN, &preset)) return 0;
    if (!gct89_init(&ctx, 44100U, &preset, 1U)) return 0;
    gct89_trigger(&ctx, 28000, 2U);
    return 1;
}

signed short example_pull(signed short input_sample)
{
    (void)input_sample;
    return gct89_process_sample(&ctx);
}
