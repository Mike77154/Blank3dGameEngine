#include "gballisticcrack89.h"

/* One-sample pull example. The host owns all storage. */
static gbc89_context ctx;

int example_start(void)
{
    gbc89_preset preset;
    if (!gbc89_get_preset(GBC89_PRESET_NEAR, &preset)) return 0;
    if (!gbc89_init(&ctx, 44100U, &preset, 1U)) return 0;
    gbc89_trigger(&ctx, 18U, 30000, 2U);
    return 1;
}

signed short example_pull(signed short input_sample)
{
    (void)input_sample;
    return gbc89_process_sample(&ctx);
}
