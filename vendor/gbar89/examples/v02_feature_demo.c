#include <stdio.h>
#include "gbar89.h"

static void print_meter_info(const char *name, const GBar89_Meter *m)
{
    int idx;
    long layer_value;
    int filled;

    idx = gbar89_layer_index_from_value(m, m->value);
    layer_value = gbar89_layer_value_from_value(m, m->value);
    filled = gbar89_layer_count_filled(m, m->value);

    printf("%s\n", name);
    printf("  value=%ld/%ld\n", m->value, m->max_value);
    printf("  layer_index=%d layer_value=%ld filled_layers=%d\n",
           idx, layer_value, filled);
    printf("  current_ratio=%ld/65536\n", gbar89_current_ratio(m));
    printf("  overlay_ratio=%ld/65536\n", gbar89_overlay_ratio(m));
    printf("  state=%d\n", gbar89_get_state(m));
}

int main(void)
{
    GBar89_CommandBuffer cb;
    GBar89_RenderOps ops;
    GBar89_Meter boss;
    GBar89_Meter masked;
    GBar89_Meter eased;

    gbar89_command_buffer_init(&cb);
    ops = gbar89_command_buffer_make_ops(&cb);

    gbar89_init(&boss);
    gbar89_set_rect(&boss, 10, 10, 220, 18);
    gbar89_set_range(&boss, 0, 3000);
    gbar89_set_layers(&boss, 3, 1000);
    boss.flags |= GBAR89_FLAG_LAYERED |
                  GBAR89_FLAG_DAMAGE_LAG |
                  GBAR89_FLAG_DRAW_LAYER_PIPS |
                  GBAR89_FLAG_DRAW_VALUE_OVERLAY |
                  GBAR89_FLAG_DRAW_PATTERN;
    gbar89_set_pattern(&boss, GBAR89_PATTERN_TICKS, 12, 2);
    gbar89_set_overlay_range(&boss, 0, 600);
    gbar89_set_overlay_value(&boss, 250);
    boss.overlay_visual_value = 250;
    gbar89_set_value(&boss, 2450);
    boss.visual_value = 2450;
    boss.lag_value = 2600;
    gbar89_draw(&boss, &ops);

    gbar89_init(&masked);
    gbar89_set_rect(&masked, 10, 40, 160, 24);
    gbar89_set_range(&masked, 0, 100);
    gbar89_set_value(&masked, 28);
    masked.flags |= GBAR89_FLAG_USE_MASK |
                    GBAR89_FLAG_DRAW_PATTERN |
                    GBAR89_FLAG_AUTO_STATE |
                    GBAR89_FLAG_STATE_BLINK;
    gbar89_set_mask(&masked, GBAR89_MASK_HEXAGON, 12, 8);
    gbar89_set_pattern(&masked, GBAR89_PATTERN_CROSSHATCH, 8, 1);
    gbar89_tick(&masked, GBAR89_FIX_HALF);
    gbar89_draw(&masked, &ops);

    gbar89_init(&eased);
    gbar89_set_range(&eased, 0, 100);
    eased.flags |= GBAR89_FLAG_USE_VISUAL;
    gbar89_set_value(&eased, 100);
    gbar89_tick(&eased, 0);
    gbar89_set_easing(&eased, GBAR89_EASE_SMOOTHSTEP, GBAR89_FIX_ONE);
    gbar89_set_value(&eased, 20);
    gbar89_tick(&eased, GBAR89_FIX_ONE / 4);

    print_meter_info("boss layered bar", &boss);
    print_meter_info("masked low-state bar", &masked);
    printf("eased visual_value after 0.25s=%ld\n", eased.visual_value);
    printf("command_count=%d vertex_count=%d index_count=%d overflow=%d\n",
           cb.command_count, cb.vertex_count, cb.index_count,
           gbar89_command_buffer_overflowed(&cb));

    return gbar89_command_buffer_overflowed(&cb) ? 1 : 0;
}
