#include "bulletspin89.h"
#include <stdio.h>

int main(void)
{
    bs89_config cfg;
    bs89_state state;
    int expected[7];
    int i;
    int slot;
    expected[0] = 0; expected[1] = 1; expected[2] = 2;
    expected[3] = 3; expected[4] = 4; expected[5] = 5;
    expected[6] = 0;
    cfg.slot_count = 6;
    cfg.start_slot = 0;
    cfg.step = 1;
    cfg.direction = BS89_DIRECTION_FORWARD;
    bulletspin89_init(&state, &cfg);
    for (i = 0; i < 7; ++i) {
        if (!bulletspin89_next(&state, &slot)) return 1;
        if (slot != expected[i]) return 2;
    }
    cfg.start_slot = 5;
    cfg.direction = BS89_DIRECTION_REVERSE;
    bulletspin89_reset(&state, &cfg);
    if (!bulletspin89_next(&state, &slot) || slot != 5) return 3;
    if (!bulletspin89_next(&state, &slot) || slot != 4) return 4;
    printf("bulletspin89: OK\n");
    return 0;
}
