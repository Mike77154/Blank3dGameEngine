#include "gtrigger89.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    gtrigger89_state s;
    gtrigger89_config c;
    gtrigger89_output o;
    gtrigger89_init(&s);
    c.model = GTRIGGER89_MODEL_NORMAL; c.spinup_ms = 0U; c.charge_max_ms = 0U;
    gtrigger89_update(&s,&c,1,16,&o); assert(o.trigger_pressed && o.trigger_down);
    gtrigger89_update(&s,&c,1,16,&o); assert(!o.trigger_pressed && o.trigger_down);
    gtrigger89_update(&s,&c,0,16,&o); assert(o.trigger_released);
    c.model = GTRIGGER89_MODEL_SPINUP; c.spinup_ms = 32U;
    gtrigger89_update(&s,&c,1,16,&o); assert(o.spin_begin && !o.trigger_down);
    gtrigger89_update(&s,&c,1,16,&o); assert(o.trigger_pressed && o.trigger_down);
    gtrigger89_update(&s,&c,0,16,&o); assert(o.spin_end && o.trigger_released);
    c.model = GTRIGGER89_MODEL_CHARGE_RELEASE; c.charge_max_ms = 100U;
    gtrigger89_update(&s,&c,1,25,&o); assert(o.charge_begin);
    gtrigger89_update(&s,&c,1,25,&o); assert(!o.trigger_down);
    gtrigger89_update(&s,&c,0,25,&o); assert(o.charge_release && o.trigger_pressed);
    assert(o.charge_elapsed_ms == 50U);
    puts("gtrigger89: OK");
        /* Buster model: press fires immediately, release only reports charge. */
    gtrigger89_reset(&s);
    c.model = GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE;
    c.charge_max_ms = 1200U;
    gtrigger89_update(&s,&c,1,25,&o); assert(o.trigger_pressed && o.charge_begin);
    gtrigger89_update(&s,&c,1,300,&o); assert(!o.trigger_pressed && !o.charge_release);
    gtrigger89_update(&s,&c,0,25,&o); assert(o.charge_release && !o.trigger_pressed);
    assert(o.charge_elapsed_ms == 325U);
return 0;
}
