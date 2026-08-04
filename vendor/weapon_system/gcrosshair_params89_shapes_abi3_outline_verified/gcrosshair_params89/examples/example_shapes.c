#include <stdio.h>
#include "gcrosshair_params89.h"

int main(void)
{
    GC89_Style style;
    GC89_InputState input;
    GC89_DrawSpec draw;

    gcp89_style_clear(&style);

    gcp89_variant_set_circle_px(&style.normal,
                                14, 2, 1, 3,
                                GC89_RGBA(255, 255, 255, 255));

    gcp89_variant_set_brackets_px(&style.aim,
                                  22, 16, 6, 2,
                                  GC89_DIRECTION_LEFT |
                                  GC89_DIRECTION_RIGHT,
                                  1, 3,
                                  GC89_RGBA(80, 255, 120, 255));
    style.use_aim_variant = 1;
    style.color_change_enabled = 1;

    gcp89_variant_set_open_triangle_px(&style.hit,
                                       18, 16, 3,
                                       GC89_SEGMENT_0 |
                                       GC89_SEGMENT_2,
                                       1, 4,
                                       GC89_RGBA(255, 80, 80, 255));
    gcp89_variant_set_shape_rotation_deg(&style.hit, 180);
    gcp89_variant_set_shape_break_px(&style.hit, 5);
    style.use_hit_variant = 1;

    input.flags = GC89_STATE_HIT;
    input.spread_fx = GC89_FX_FROM_INT(2);
    gcp89_resolve(&style, &input, &draw);

    printf("shape=%d rx=%ld ry=%ld segments=%d\n",
           draw.shape_type,
           draw.shape_radius_x_fx,
           draw.shape_radius_y_fx,
           draw.shape_segment_mask);
    return 0;
}
