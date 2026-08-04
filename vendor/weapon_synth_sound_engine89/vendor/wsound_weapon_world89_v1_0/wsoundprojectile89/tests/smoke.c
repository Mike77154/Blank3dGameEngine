#include "wsoundprojectile89.h"
int main(void)
{
    wsoundprojectile89_context ctx;
    wsound89_i32 sum;
    wsound89_u16 i;
    wsound89_i16 s;
    if (wsoundprojectile89_init(&ctx, 44100U, 2U) != WSOUND89_OK) return 1;
    if (wsoundprojectile89_trigger(&ctx, WSOUNDPROJECTILE89_SUPERSONIC_NWAVE, 28000U, 30000U) != WSOUND89_OK) return 2;
    sum = 0;
    for (i = 0U; i < 2000U; ++i) { s = wsoundprojectile89_process_sample(&ctx); if (s < 0) sum -= s; else sum += s; }
    if (sum == 0) return 3;
    return 0;
}
