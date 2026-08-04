#include "wsoundcombatbus89.h"
int main(void)
{
    wsoundcombatbus89_context ctx;
    wsoundcombatbus89_frame f;
    wsound89_i16 l;
    wsound89_i16 r;
    wsound89_u16 i;
    if (wsoundcombatbus89_init(&ctx) != WSOUND89_OK) return 1;
    for (i = 0U; i < WSOUNDCOMBATBUS89_BUSES; ++i) { f.left[i] = 10000; f.right[i] = 10000; }
    wsoundcombatbus89_process_sample(&ctx, &f, &l, &r);
    if (l > 30000 || r > 30000) return 2;
    return 0;
}
