#include "wsoundimpact89.h"
int main(void)
{
    wsoundimpact89_context ctx;
    wsound89_i32 sum;
    wsound89_i16 s;
    wsound89_u16 i;
    if (wsoundimpact89_init(&ctx, 3U) != WSOUND89_OK) return 1;
    if (wsoundimpact89_trigger(&ctx, WSOUNDIMPACT89_METAL, 30000U, 16000U) != WSOUND89_OK) return 2;
    sum = 0;
    for (i = 0U; i < 4000U; ++i) { s = wsoundimpact89_process_sample(&ctx); if (s < 0) sum -= s; else sum += s; }
    if (sum == 0) return 3;
    return 0;
}
