#include "wsounddna89.h"
int main(void)
{
    wsounddna89_context ctx;
    wsounddna89_shot shot;
    if (wsounddna89_init(&ctx, 1U) != WSOUND89_OK) return 1;
    if (wsounddna89_next(&ctx, WSOUNDDNA89_RIFLE, 26000U, &shot) != WSOUND89_OK) return 2;
    if (shot.energy_q15 == 0U || shot.pitch_q16 == 0U || shot.seed == 0U) return 3;
    return 0;
}
