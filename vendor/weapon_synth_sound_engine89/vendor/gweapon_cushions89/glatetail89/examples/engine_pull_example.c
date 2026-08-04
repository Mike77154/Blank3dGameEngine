#include "glatetail89.h"

/* One-sample pull example. The host owns all storage. */
static glt89_context ctx;

int example_start(void)
{
    glt89_preset preset;
    if (!glt89_get_preset(GLT89_PRESET_WAREHOUSE, &preset)) return 0;
    if (!glt89_init(&ctx, 44100U, &preset)) return 0;
    /* tail starts when nonzero input enters */;
    return 1;
}

signed short example_pull(signed short input_sample)
{
    return glt89_process_sample(&ctx, input_sample);
}
