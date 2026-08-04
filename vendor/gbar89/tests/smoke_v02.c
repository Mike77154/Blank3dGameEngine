#include <assert.h>
#include "gbar89.h"

int main(void)
{
    GBar89_Meter m;
    GBar89_CommandBuffer cb;
    GBar89_RenderOps ops;

    gbar89_init(&m);
    gbar89_set_range(&m, 0, 3000);
    gbar89_set_layers(&m, 3, 1000);
    m.flags |= GBAR89_FLAG_LAYERED;
    gbar89_set_value(&m, 2450);
    gbar89_tick(&m, 0);

    assert(gbar89_layer_index_from_value(&m, 2450) == 2);
    assert(gbar89_layer_value_from_value(&m, 2450) == 450);
    assert(gbar89_layer_count_filled(&m, 2450) == 3);
    assert(gbar89_current_ratio(&m) > 29000L);
    assert(gbar89_current_ratio(&m) < 30000L);

    assert(gbar89_ease(GBAR89_EASE_LINEAR, GBAR89_FIX_HALF) == GBAR89_FIX_HALF);
    assert(gbar89_ease(GBAR89_EASE_IN_QUAD, GBAR89_FIX_HALF) == (GBAR89_FIX_ONE / 4));

    gbar89_command_buffer_init(&cb);
    ops = gbar89_command_buffer_make_ops(&cb);
    gbar89_draw(&m, &ops);
    assert(cb.command_count > 0);
    assert(gbar89_command_buffer_overflowed(&cb) == 0);

    return 0;
}
