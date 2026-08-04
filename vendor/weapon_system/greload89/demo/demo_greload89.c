#include "greload89.h"

#include <stdio.h>

int main(void)
{
    GREL89_State reload;
    int result;

    grel89_init(&reload);
    result = grel89_begin_full(&reload, 700ul, 3, 15);
    printf("begin=%d requested=%d\n", result, grel89_rounds_requested(&reload));

    while (grel89_is_active(&reload)) {
        result = grel89_update(&reload, 100ul);
        printf("remaining=%lu progress=%d/10000 result=%d\n",
               grel89_remaining_ms(&reload),
               grel89_progress_permyriad(&reload),
               result);
    }

    printf("load this many rounds: %d\n", grel89_take_completed_rounds(&reload));
    return 0;
}
