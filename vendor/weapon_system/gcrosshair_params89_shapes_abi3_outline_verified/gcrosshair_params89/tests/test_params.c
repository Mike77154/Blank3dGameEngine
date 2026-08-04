#include <stdio.h>
#include "gcrosshair_params89.h"

int main(void)
{
    GC89_Style style;
    GC89_InputState state;
    GC89_DrawSpec out;

    gcp89_style_clear(&style);
    gcp89_variant_set_vector_px(&style.normal, 9, 13, 1, 1, 3,
                                GC89_ARM_ALL,
                                GC89_RGBA(209, 235, 255, 255));
    gcp89_variant_set_vector_px(&style.aim, 5, 9, 2, 1, 4,
                                GC89_ARM_ALL,
                                GC89_RGBA(255, 235, 89, 255));
    style.use_aim_variant = 1;
    gcp89_variant_set_outline_px(&style.aim, 1, 2,
                                 GC89_RGBA(0, 0, 0, 255));
    gcp89_style_enable_color_change(&style, 1);

    state.flags = GC89_STATE_AIM;
    state.spread_fx = GC89_FX_FROM_INT(2);
    gcp89_resolve(&style, &state, &out);

    if (!out.visible) return 1;
    if (out.gap_fx != GC89_FX_FROM_INT(7)) return 2;
    if (out.arm_length_fx != GC89_FX_FROM_INT(9)) return 3;
    if (out.thickness_fx != GC89_FX_FROM_INT(2)) return 4;
    if (out.color_rgba != GC89_RGBA(255, 235, 89, 255)) return 5;
    if (out.shape_type != GC89_SHAPE_CROSS) return 6;
    if (out.spread_mode != GC89_SPREAD_GAP) return 7;
    if (!out.outline_enabled) return 10;
    if (out.outline_width_fx != GC89_FX_FROM_INT(2)) return 11;
    if (out.outline_color_rgba != GC89_RGBA(0, 0, 0, 255)) return 12;

    gcp89_style_enable_color_change(&style, 0);
    gcp89_resolve(&style, &state, &out);
    if (out.color_rgba != style.normal.color_rgba) return 8;

    state.flags = GC89_STATE_DISABLED;
    gcp89_resolve(&style, &state, &out);
    if (out.visible) return 9;

    printf("gcrosshair_params89 ABI3 outline: OK\n");
    return 0;
}
