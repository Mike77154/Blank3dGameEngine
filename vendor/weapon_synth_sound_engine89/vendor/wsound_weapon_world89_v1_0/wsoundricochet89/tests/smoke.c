#include "wsoundricochet89.h"
int main(void)
{
    wsoundricochet89_context ctx;
    wsound89_i32 sum;
    wsound89_i16 s;
    wsound89_u16 i;
    if (wsoundricochet89_init(&ctx, 4U) != WSOUND89_OK) return 1;
    if (wsoundricochet89_trigger(&ctx, 28000U, 26000U, 12000U, 44100U) != WSOUND89_OK) return 2;
    sum = 0;
    for (i = 0U; i < 8000U; ++i) { s = wsoundricochet89_process_sample(&ctx); if (s < 0) sum -= s; else sum += s; }
    if (sum == 0) return 3;
    return 0;
}
