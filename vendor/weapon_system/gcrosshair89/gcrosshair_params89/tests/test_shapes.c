#include <stdio.h>
#include "gcrosshair_params89.h"

static int check_shape(int shape_type)
{
    GC89_Style style;
    GC89_InputState input;
    GC89_DrawSpec out;

    gcp89_style_clear(&style);
    gcp89_variant_set_shape_px(&style.normal,
                               shape_type,
                               20, 15, 5, 2,
                               1, 3,
                               GC89_SEGMENT_0 | GC89_SEGMENT_2,
                               GC89_DIRECTION_LEFT | GC89_DIRECTION_RIGHT,
                               GC89_RGBA(10, 20, 30, 255));
    gcp89_variant_set_shape_rotation_deg(&style.normal, 405);
    gcp89_variant_set_shape_break_px(&style.normal, 6);
    input.flags = 0;
    input.spread_fx = GC89_FX_FROM_INT(4);
    gcp89_resolve(&style, &input, &out);

    if (!out.visible) return 1;
    if (out.shape_type != shape_type) return 2;
    if (out.shape_radius_x_fx != GC89_FX_FROM_INT(24)) return 3;
    if (out.shape_radius_y_fx != GC89_FX_FROM_INT(19)) return 4;
    if (out.shape_depth_fx != GC89_FX_FROM_INT(5)) return 5;
    if (out.shape_rotation_deg_fx != GC89_FX_FROM_INT(45)) return 6;
    if (out.shape_segment_mask != (GC89_SEGMENT_0 | GC89_SEGMENT_2)) return 7;
    if (out.shape_direction_mask !=
        (GC89_DIRECTION_LEFT | GC89_DIRECTION_RIGHT)) return 8;
    if (out.thickness_fx != GC89_FX_FROM_INT(2)) return 9;
    if (out.dot_size_fx != GC89_FX_FROM_INT(3)) return 10;
    if (out.spread_mode != GC89_SPREAD_SHAPE_SIZE) return 11;
    if (out.shape_break_fx != GC89_FX_FROM_INT(6)) return 12;
    return 0;
}

int main(void)
{
    GC89_Style style;
    GC89_InputState input;
    GC89_DrawSpec out;
    int shape;
    int result;

    for (shape = GC89_SHAPE_CIRCLE;
         shape <= GC89_SHAPE_OPEN_TRIANGLE;
         ++shape) {
        result = check_shape(shape);
        if (result != 0) return shape * 20 + result;
    }

    gcp89_style_clear(&style);
    gcp89_variant_set_open_triangle_px(&style.normal,
                                       18, 16, 2,
                                       GC89_SEGMENT_0 | GC89_SEGMENT_2,
                                       0, 1,
                                       GC89_RGBA(255, 255, 255, 255));
    gcp89_variant_set_shape_break_px(&style.normal, 5);
    gcp89_variant_set_spread_mode(&style.normal, GC89_SPREAD_BOTH);
    style.normal.gap_fx = GC89_FX_FROM_INT(6);
    input.flags = 0;
    input.spread_fx = GC89_FX_FROM_INT(3);
    gcp89_resolve(&style, &input, &out);

    if (out.shape_type != GC89_SHAPE_OPEN_TRIANGLE) return 181;
    if (out.shape_segment_mask !=
        (GC89_SEGMENT_0 | GC89_SEGMENT_2)) return 182;
    if (out.gap_fx != GC89_FX_FROM_INT(9)) return 183;
    if (out.shape_radius_x_fx != GC89_FX_FROM_INT(21)) return 184;
    if (out.shape_radius_y_fx != GC89_FX_FROM_INT(19)) return 185;
    if (out.shape_break_fx != GC89_FX_FROM_INT(5)) return 190;

    gcp89_variant_set_circle_px(&style.aim, 11, 3, 1, 2,
                                GC89_RGBA(0, 255, 0, 255));
    style.use_aim_variant = 1;
    style.color_change_enabled = 1;
    input.flags = GC89_STATE_AIM;
    input.spread_fx = 0;
    gcp89_resolve(&style, &input, &out);
    if (out.shape_type != GC89_SHAPE_CIRCLE) return 186;
    if (out.shape_radius_x_fx != GC89_FX_FROM_INT(11)) return 187;
    if (out.shape_radius_y_fx != GC89_FX_FROM_INT(11)) return 188;
    if (out.color_rgba != GC89_RGBA(0, 255, 0, 255)) return 189;

    printf("gcrosshair_params89 shapes: OK\n");
    return 0;
}
