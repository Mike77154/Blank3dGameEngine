#include "wsoundprop89.h"
#define N 5000U
static wsound89_i16 mem[N];
int main(void)
{
    wsoundprop89_context ctx;
    wsound89_u32 i;
    wsound89_i16 s;
    int found;
    if (wsoundprop89_init(&ctx, 44100U, mem, N) != WSOUND89_OK) return 1;
    if (wsoundprop89_set_path(&ctx, 1000U, 34300U, 32767, 32767U) != WSOUND89_OK) return 2;
    found = 0;
    for (i = 0U; i < 3000U; ++i) { s = wsoundprop89_process_sample(&ctx, i == 0U ? 30000 : 0); if (s != 0) found = 1; }
    if (!found) return 3;
    return 0;
}
