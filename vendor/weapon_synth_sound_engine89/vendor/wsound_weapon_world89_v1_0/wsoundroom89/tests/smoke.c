#include "wsoundroom89.h"
#define N 12000U
static wsound89_i16 mem[N];
int main(void)
{
    wsoundroom89_context ctx;
    wsound89_i16 l;
    wsound89_i16 r;
    wsound89_u32 i;
    wsound89_i32 sum;
    if (wsoundroom89_init(&ctx, 44100U, WSOUNDROOM89_SMALL, mem, N) != WSOUND89_OK) return 1;
    sum = 0;
    for (i = 0U; i < 10000U; ++i) { wsoundroom89_process_sample(&ctx, i == 0U ? 30000 : 0, &l, &r); if (l < 0) sum -= l; else sum += l; }
    if (sum == 0) return 2;
    return 0;
}
