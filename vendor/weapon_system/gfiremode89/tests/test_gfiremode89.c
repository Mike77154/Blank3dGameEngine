#include "gfiremode89.h"

#include <stdio.h>

static int failures = 0;

static void check(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        failures++;
    }
}

static void test_semi(void)
{
    GFM89_State s;
    gfm89_init(&s);
    gfm89_configure(&s, GFM89_FIRE_SEMI, 3, 100u);
    check(gfm89_request(&s, GFM89_TRIGGER_PRESSED | GFM89_TRIGGER_DOWN, 0u) == GFM89_FIRE_REQUEST,
          "semi accepts press");
    check(gfm89_commit_fire(&s) == GFM89_OK, "semi commit");
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 100u) == GFM89_TRIGGER_LOCKED,
          "semi does not repeat while held");
}

static void test_auto(void)
{
    GFM89_State s;
    gfm89_init(&s);
    gfm89_configure(&s, GFM89_FIRE_AUTO, 3, 100u);
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 0u) == GFM89_FIRE_REQUEST,
          "auto first shot");
    gfm89_commit_fire(&s);
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 50u) == GFM89_COOLDOWN,
          "auto honors cooldown");
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 50u) == GFM89_FIRE_REQUEST,
          "auto repeats after cooldown");
    gfm89_commit_fire(&s);
}

static void test_hold_once(void)
{
    GFM89_State s;
    gfm89_init(&s);
    gfm89_configure(&s, GFM89_FIRE_HOLD_ONCE, 3, 0u);
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 0u) == GFM89_FIRE_REQUEST,
          "hold_once first down");
    gfm89_commit_fire(&s);
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 0u) == GFM89_TRIGGER_LOCKED,
          "hold_once stays latched");
    check(gfm89_request(&s, GFM89_TRIGGER_RELEASED, 0u) == GFM89_TRIGGER_LOCKED,
          "release only resets latch");
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 0u) == GFM89_FIRE_REQUEST,
          "hold_once accepts next hold");
    gfm89_reject_fire(&s);
}

static void test_burst(void)
{
    GFM89_State s;
    int i;
    gfm89_init(&s);
    gfm89_configure(&s, GFM89_FIRE_BURST, 3, 0u);
    for (i = 0; i < 3; i++) {
        int flags = GFM89_TRIGGER_DOWN;
        if (i == 0) flags |= GFM89_TRIGGER_PRESSED;
        check(gfm89_request(&s, flags, 0u) == GFM89_FIRE_REQUEST,
              "burst request");
        gfm89_commit_fire(&s);
    }
    check(gfm89_burst_remaining(&s) == 0, "burst exhausted");
    check(gfm89_request(&s, GFM89_TRIGGER_DOWN, 0u) == GFM89_TRIGGER_LOCKED,
          "burst does not restart without press");
}

int main(void)
{
    test_semi();
    test_auto();
    test_hold_once();
    test_burst();
    if (failures) {
        printf("gfiremode89: %d failure(s)\n", failures);
        return 1;
    }
    printf("gfiremode89: all tests passed\n");
    return 0;
}
