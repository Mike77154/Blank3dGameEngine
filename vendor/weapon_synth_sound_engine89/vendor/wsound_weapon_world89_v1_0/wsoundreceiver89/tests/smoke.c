#include "wsoundreceiver89.h"
int main(void)
{
    wsoundreceiver89_context ctx;
    wsound89_i16 s;
    wsound89_u16 i;
    wsound89_i32 sum;
    if (wsoundreceiver89_init(&ctx, WSOUNDRECEIVER89_RIFLE_STEEL) != WSOUND89_OK) return 1;
    wsoundreceiver89_excite(&ctx, 30000);
    sum = 0;
    for (i = 0U; i < 300U; ++i) { s = wsoundreceiver89_process_sample(&ctx, 0); if (s < 0) sum -= s; else sum += s; }
    if (sum == 0) return 2;
    return 0;
}
