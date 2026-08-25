#include <stdio.h>
#include "gcrosshair_base89.h"

int main(void)
{
    GC89_Style style;
    int preset_id;

    preset_id = GCB89_PRESET_BRACKET_DYNAMIC_LOCK;
    if (!gcb89_make_preset(preset_id, &style)) return 1;

    printf("preset=%d name=%s shape=%d spread=%d\n",
           preset_id,
           gcb89_preset_name(preset_id),
           style.normal.shape_type,
           style.normal.spread_mode);
    style.normal.outline_enabled = 1;
    style.normal.outline_width_fx = GC89_FX_FROM_INT(1);
    style.normal.outline_color_rgba = GC89_RGBA(0, 0, 0, 255);

    printf("radius=(%ld,%ld) depth=%ld break=%ld\n",
           style.normal.shape_radius_x_fx,
           style.normal.shape_radius_y_fx,
           style.normal.shape_depth_fx,
           style.normal.shape_break_fx);
    printf("outline=%d width=%ld color=%08lx\n",
           style.normal.outline_enabled,
           style.normal.outline_width_fx,
           style.normal.outline_color_rgba);
    return 0;
}
