#include "bulletcircle89.h"
#include <stdio.h>

static long abs_l(long v) { return v < 0L ? -v : v; }

int main(void)
{
    bc89_request request;
    bc89_result result;
    int i;
    request.center.x = 0L; request.center.y = 0L; request.center.z = 0L;
    request.right.x = BC89_ONE; request.right.y = 0L; request.right.z = 0L;
    request.up.x = 0L; request.up.y = BC89_ONE; request.up.z = 0L;
    request.radius_fx = BC89_ONE;
    request.slot_count = 4;
    request.phase_turn_q16 = 0L;
    for (i = 0; i < 4; ++i) {
        request.slot_index = i;
        if (!bulletcircle89_resolve(&request, &result)) return 1;
        if (!result.valid) return 2;
        if (i == 0 && (result.origin.x < 4000L || abs_l(result.origin.y) > 32L)) return 3;
        if (i == 1 && (result.origin.y < 4000L || abs_l(result.origin.x) > 32L)) return 4;
        if (i == 2 && (result.origin.x > -4000L || abs_l(result.origin.y) > 32L)) return 5;
        if (i == 3 && (result.origin.y > -4000L || abs_l(result.origin.x) > 32L)) return 6;
    }
    printf("bulletcircle89: OK\n");
    return 0;
}
