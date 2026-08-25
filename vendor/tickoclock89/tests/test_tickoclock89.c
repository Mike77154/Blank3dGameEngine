#include "tickoclock89.h"
#include <assert.h>

int main(void)
{
    TickOClock89 c;
    TickOClock89HMS h;
    tickoclock89_init(&c, 60U);
    tickoclock89_set_hms(&c, 23, 59, 59, 900);
    tickoclock89_update(&c, 100U, 1U);
    tickoclock89_get_hms(&c, &h);
    assert(h.days == 1U);
    assert(h.hours == 0 && h.minutes == 0 && h.seconds == 0);
    assert(h.milliseconds == 0);
    assert(tickoclock89_delta_ticks(&c) == 6U);
    assert(tickoclock89_global_frames(&c) == 1U);
    assert(tickoclock89_frametime_q16(&c) > 6500);
    tickoclock89_update(&c, 900U, 1U);
    assert(tickoclock89_every_seconds(&c, 1U));
    assert(tickoclock89_global_ticks(&c) == 60U);
    return 0;
}
