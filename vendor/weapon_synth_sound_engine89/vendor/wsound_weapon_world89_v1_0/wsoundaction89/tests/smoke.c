#include "wsoundaction89.h"
int main(void)
{
    wsoundaction89_context ctx;
    wsoundaction89_event ev[WSOUNDACTION89_MAX_EVENTS];
    wsound89_u16 n;
    if (wsoundaction89_init(&ctx, 44100U) != WSOUND89_OK) return 1;
    if (wsoundaction89_trigger(&ctx, WSOUNDACTION89_PISTOL, 65536U) != WSOUND89_OK) return 2;
    if (wsoundaction89_advance(&ctx, 4410U, ev, WSOUNDACTION89_MAX_EVENTS, &n) != WSOUND89_OK) return 3;
    if (n == 0U) return 4;
    return 0;
}
