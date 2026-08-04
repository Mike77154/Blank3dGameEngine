#include "gklek89.h"

static gkl89_context ctx;
static gkl89_s16 out[8192];

int main(void)
{
    unsigned long i;
    unsigned long nonzero;
    long peak;
    gkl89_init(&ctx, 1234UL);
    if (gkl89_is_active(&ctx)) return 1;
    if (!gkl89_trigger(&ctx, GKL89_PRESET_CARRIER_KLEK, 5678UL, 12U)) return 2;
    gkl89_render_mono(&ctx, out, 8192UL);
    nonzero = 0UL;
    peak = 0L;
    for (i = 0UL; i < 8192UL; ++i) {
        long a;
        a = out[i] < 0 ? -(long)out[i] : (long)out[i];
        if (a != 0L) nonzero++;
        if (a > peak) peak = a;
    }
    if (nonzero < 100UL) return 3;
    if (peak < 8000L || peak > 32767L) return 4;
    return 0;
}
