#include "greload89.h"

#include <stdio.h>

static int failures = 0;

static void check(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        failures++;
    }
}

int main(void)
{
    GREL89_State reload;
    int amount;

    grel89_init(&reload);
    check(grel89_begin_full(&reload, 700ul, 5, 15) == GREL89_STARTED,
          "reload starts");
    check(grel89_rounds_requested(&reload) == 10, "requests missing rounds");
    check(grel89_update(&reload, 300ul) == GREL89_OK, "still reloading");
    check(grel89_remaining_ms(&reload) == 400ul, "remaining time");
    check(grel89_update(&reload, 400ul) == GREL89_COMPLETED, "reload completes");
    check(grel89_has_completion(&reload), "completion pending");
    amount = grel89_take_completed_rounds(&reload);
    check(amount == 10, "completion returns requested rounds");
    check(!grel89_has_completion(&reload), "completion consumed");

    check(grel89_begin_full(&reload, 500ul, 15, 15) == GREL89_NOT_NEEDED,
          "full magazine does not reload");

    if (failures) {
        printf("greload89: %d failure(s)\n", failures);
        return 1;
    }
    printf("greload89: all tests passed\n");
    return 0;
}
