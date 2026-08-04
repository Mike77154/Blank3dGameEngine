#include <string.h>
#include "gcrosshair_params89.h"

static GC89_Fixed gcp89_fx_mul(GC89_Fixed a, GC89_Fixed b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long out;
    int negative;

    negative = 0;
    if (a < 0) {
        ua = (unsigned long)(-a);
        negative = !negative;
    } else {
        ua = (unsigned long)a;
    }
    if (b < 0) {
        ub = (unsigned long)(-b);
        negative = !negative;
    } else {
        ub = (unsigned long)b;
    }
    ah = ua >> 16;
    al = ua & 65535UL;
    bh = ub >> 16;
    bl = ub & 65535UL;
    out = (ah * bh << 16) + (ah * bl) + (al * bh) + ((al * bl) >> 16);
    if (negative) return -(GC89_Fixed)out;
    return (GC89_Fixed)out;
}

static int gcp89_clamp_shape_type(int shape_type)
{
    if (shape_type < 0 || shape_type >= GC89_SHAPE_COUNT) {
        return GC89_SHAPE_CROSS;
    }
    return shape_type;
}

static int gcp89_clamp_spread_mode(int spread_mode)
{
    return spread_mode & GC89_SPREAD_BOTH;
}

static int gcp89_default_segment_mask(int shape_type)
{
    switch (shape_type) {
    case GC89_SHAPE_CIRCLE:
        return GC89_SEGMENT_ALL;
    case GC89_SHAPE_SQUARE:
    case GC89_SHAPE_DIAMOND:
        return GC89_SEGMENT_0 | GC89_SEGMENT_1 |
               GC89_SEGMENT_2 | GC89_SEGMENT_3;
    case GC89_SHAPE_HEXAGON:
        return GC89_SEGMENT_0 | GC89_SEGMENT_1 |
               GC89_SEGMENT_2 | GC89_SEGMENT_3 |
               GC89_SEGMENT_4 | GC89_SEGMENT_5;
    case GC89_SHAPE_OPEN_TRIANGLE:
        return GC89_SEGMENT_0 | GC89_SEGMENT_1 | GC89_SEGMENT_2;
    default:
        return GC89_SEGMENT_ALL;
    }
}

void gcp89_variant_clear(GC89_Variant *variant)
{
    if (!variant) return;
    memset(variant, 0, sizeof(*variant));
    variant->draw_mode = GC89_DRAW_VECTOR;
    variant->arm_mask = GC89_ARM_ALL;
    variant->image_tint_rgba = GC89_RGBA(255, 255, 255, 255);
    variant->shape_type = GC89_SHAPE_CROSS;
    variant->shape_segment_mask = GC89_SEGMENT_ALL;
    variant->shape_direction_mask = GC89_DIRECTION_ALL;
    variant->spread_mode = GC89_SPREAD_GAP;
    variant->outline_enabled = 0;
    variant->outline_width_fx = 0;
    variant->outline_color_rgba = GC89_RGBA(0, 0, 0, 255);
}

void gcp89_style_clear(GC89_Style *style)
{
    if (!style) return;
    memset(style, 0, sizeof(*style));
    gcp89_variant_clear(&style->normal);
    gcp89_variant_clear(&style->aim);
    gcp89_variant_clear(&style->fire);
    gcp89_variant_clear(&style->hit);
    style->spread_multiplier_fx = GC89_FX_ONE;
}

void gcp89_variant_set_vector_px(GC89_Variant *variant,
                                 int gap_px,
                                 int arm_length_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 int arm_mask,
                                 unsigned long color_rgba)
{
    if (!variant) return;
    if (gap_px < 0) gap_px = 0;
    if (arm_length_px < 0) arm_length_px = 0;
    if (thickness_px < 1) thickness_px = 1;
    if (dot_size_px < 1) dot_size_px = 1;
    variant->draw_mode |= GC89_DRAW_VECTOR;
    variant->gap_fx = GC89_FX_FROM_INT(gap_px);
    variant->arm_length_fx = GC89_FX_FROM_INT(arm_length_px);
    variant->thickness_fx = GC89_FX_FROM_INT(thickness_px);
    variant->dot_enabled = dot_enabled ? 1 : 0;
    variant->dot_size_fx = GC89_FX_FROM_INT(dot_size_px);
    variant->arm_mask = arm_mask & GC89_ARM_ALL;
    variant->color_rgba = color_rgba;
    variant->shape_type = GC89_SHAPE_CROSS;
    variant->shape_segment_mask = GC89_SEGMENT_ALL;
    variant->shape_direction_mask = variant->arm_mask;
    variant->shape_radius_x_fx = 0;
    variant->shape_radius_y_fx = 0;
    variant->shape_depth_fx = 0;
    variant->shape_rotation_deg_fx = 0;
    variant->shape_break_fx = 0;
    variant->spread_mode = GC89_SPREAD_GAP;
}

void gcp89_variant_set_shape_px(GC89_Variant *variant,
                                int shape_type,
                                int radius_x_px,
                                int radius_y_px,
                                int depth_px,
                                int thickness_px,
                                int dot_enabled,
                                int dot_size_px,
                                int segment_mask,
                                int direction_mask,
                                unsigned long color_rgba)
{
    if (!variant) return;
    shape_type = gcp89_clamp_shape_type(shape_type);
    if (radius_x_px < 0) radius_x_px = 0;
    if (radius_y_px < 0) radius_y_px = 0;
    if (depth_px < 0) depth_px = 0;
    if (thickness_px < 1) thickness_px = 1;
    if (dot_size_px < 1) dot_size_px = 1;

    variant->draw_mode |= GC89_DRAW_VECTOR;
    variant->gap_fx = 0;
    variant->arm_length_fx = 0;
    variant->shape_type = shape_type;
    variant->shape_radius_x_fx = GC89_FX_FROM_INT(radius_x_px);
    variant->shape_radius_y_fx = GC89_FX_FROM_INT(radius_y_px);
    variant->shape_depth_fx = GC89_FX_FROM_INT(depth_px);
    variant->thickness_fx = GC89_FX_FROM_INT(thickness_px);
    variant->dot_enabled = dot_enabled ? 1 : 0;
    variant->dot_size_fx = GC89_FX_FROM_INT(dot_size_px);
    variant->shape_segment_mask = segment_mask & GC89_SEGMENT_ALL;
    variant->shape_direction_mask = direction_mask & GC89_DIRECTION_ALL;
    variant->arm_mask = variant->shape_direction_mask;
    variant->color_rgba = color_rgba;
    variant->shape_rotation_deg_fx = 0;
    variant->shape_break_fx = 0;
    variant->spread_mode = (shape_type == GC89_SHAPE_CROSS) ?
                           GC89_SPREAD_GAP : GC89_SPREAD_SHAPE_SIZE;
}

void gcp89_variant_set_circle_px(GC89_Variant *variant,
                                 int radius_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_CIRCLE,
                              radius_px, radius_px, 0,
                              thickness_px, dot_enabled, dot_size_px,
                              gcp89_default_segment_mask(GC89_SHAPE_CIRCLE),
                              GC89_DIRECTION_ALL, color_rgba);
}

void gcp89_variant_set_square_px(GC89_Variant *variant,
                                 int half_size_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_SQUARE,
                              half_size_px, half_size_px, 0,
                              thickness_px, dot_enabled, dot_size_px,
                              gcp89_default_segment_mask(GC89_SHAPE_SQUARE),
                              GC89_DIRECTION_ALL, color_rgba);
}

void gcp89_variant_set_diamond_px(GC89_Variant *variant,
                                  int radius_x_px,
                                  int radius_y_px,
                                  int thickness_px,
                                  int dot_enabled,
                                  int dot_size_px,
                                  unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_DIAMOND,
                              radius_x_px, radius_y_px, 0,
                              thickness_px, dot_enabled, dot_size_px,
                              gcp89_default_segment_mask(GC89_SHAPE_DIAMOND),
                              GC89_DIRECTION_ALL, color_rgba);
}

void gcp89_variant_set_chevrons_px(GC89_Variant *variant,
                                   int radius_x_px,
                                   int radius_y_px,
                                   int depth_px,
                                   int thickness_px,
                                   int direction_mask,
                                   int dot_enabled,
                                   int dot_size_px,
                                   unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_CHEVRONS,
                              radius_x_px, radius_y_px, depth_px,
                              thickness_px, dot_enabled, dot_size_px,
                              GC89_SEGMENT_ALL, direction_mask,
                              color_rgba);
}

void gcp89_variant_set_hexagon_px(GC89_Variant *variant,
                                  int radius_x_px,
                                  int radius_y_px,
                                  int thickness_px,
                                  int dot_enabled,
                                  int dot_size_px,
                                  unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_HEXAGON,
                              radius_x_px, radius_y_px, 0,
                              thickness_px, dot_enabled, dot_size_px,
                              gcp89_default_segment_mask(GC89_SHAPE_HEXAGON),
                              GC89_DIRECTION_ALL, color_rgba);
}

void gcp89_variant_set_brackets_px(GC89_Variant *variant,
                                   int radius_x_px,
                                   int radius_y_px,
                                   int hook_depth_px,
                                   int thickness_px,
                                   int direction_mask,
                                   int dot_enabled,
                                   int dot_size_px,
                                   unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_BRACKETS,
                              radius_x_px, radius_y_px, hook_depth_px,
                              thickness_px, dot_enabled, dot_size_px,
                              GC89_SEGMENT_ALL, direction_mask,
                              color_rgba);
}

void gcp89_variant_set_open_triangle_px(GC89_Variant *variant,
                                        int radius_x_px,
                                        int radius_y_px,
                                        int thickness_px,
                                        int side_mask,
                                        int dot_enabled,
                                        int dot_size_px,
                                        unsigned long color_rgba)
{
    gcp89_variant_set_shape_px(variant, GC89_SHAPE_OPEN_TRIANGLE,
                              radius_x_px, radius_y_px, 0,
                              thickness_px, dot_enabled, dot_size_px,
                              side_mask & (GC89_SEGMENT_0 |
                                           GC89_SEGMENT_1 |
                                           GC89_SEGMENT_2),
                              GC89_DIRECTION_ALL, color_rgba);
}

void gcp89_variant_set_shape_rotation_deg(GC89_Variant *variant,
                                          int degrees)
{
    if (!variant) return;
    degrees %= 360;
    if (degrees < 0) degrees += 360;
    variant->shape_rotation_deg_fx = GC89_FX_FROM_INT(degrees);
}

void gcp89_variant_set_shape_segments(GC89_Variant *variant,
                                      int segment_mask)
{
    if (!variant) return;
    variant->shape_segment_mask = segment_mask & GC89_SEGMENT_ALL;
}

void gcp89_variant_set_shape_break_px(GC89_Variant *variant,
                                      int break_px)
{
    if (!variant) return;
    if (break_px < 0) break_px = 0;
    variant->shape_break_fx = GC89_FX_FROM_INT(break_px);
}

void gcp89_variant_set_shape_directions(GC89_Variant *variant,
                                        int direction_mask)
{
    if (!variant) return;
    variant->shape_direction_mask = direction_mask & GC89_DIRECTION_ALL;
    variant->arm_mask = variant->shape_direction_mask;
}

void gcp89_variant_set_spread_mode(GC89_Variant *variant,
                                   int spread_mode)
{
    if (!variant) return;
    variant->spread_mode = gcp89_clamp_spread_mode(spread_mode);
}

void gcp89_variant_set_outline_px(GC89_Variant *variant,
                                  int enabled,
                                  int width_px,
                                  unsigned long color_rgba)
{
    if (!variant) return;
    if (width_px < 0) width_px = 0;
    variant->outline_enabled = (enabled && width_px > 0) ? 1 : 0;
    variant->outline_width_fx = GC89_FX_FROM_INT(width_px);
    variant->outline_color_rgba = color_rgba;
}

void gcp89_variant_set_image_px(GC89_Variant *variant,
                                int image_id,
                                int width_px,
                                int height_px,
                                unsigned long tint_rgba,
                                int keep_vector)
{
    if (!variant) return;
    if (width_px < 1) width_px = 1;
    if (height_px < 1) height_px = 1;
    variant->draw_mode = keep_vector ? GC89_DRAW_HYBRID : GC89_DRAW_IMAGE;
    variant->image_id = image_id;
    variant->image_width_fx = GC89_FX_FROM_INT(width_px);
    variant->image_height_fx = GC89_FX_FROM_INT(height_px);
    variant->image_tint_rgba = tint_rgba;
}

void gcp89_style_set_center_offset_px(GC89_Style *style,
                                      int offset_x_px,
                                      int offset_y_px)
{
    if (!style) return;
    style->center_offset_x_fx = GC89_FX_FROM_INT(offset_x_px);
    style->center_offset_y_fx = GC89_FX_FROM_INT(offset_y_px);
}

void gcp89_style_enable_color_change(GC89_Style *style, int enabled)
{
    if (!style) return;
    style->color_change_enabled = enabled ? 1 : 0;
}

void gcp89_style_set_spread_multiplier(GC89_Style *style,
                                       GC89_Fixed multiplier_fx)
{
    if (!style) return;
    if (multiplier_fx < 0) multiplier_fx = 0;
    style->spread_multiplier_fx = multiplier_fx;
}

static const GC89_Variant *gcp89_choose_variant(const GC89_Style *style,
                                                int flags)
{
    if ((flags & GC89_STATE_HIT) != 0 && style->use_hit_variant) {
        return &style->hit;
    }
    if ((flags & GC89_STATE_FIRE) != 0 && style->use_fire_variant) {
        return &style->fire;
    }
    if ((flags & GC89_STATE_AIM) != 0 && style->use_aim_variant) {
        return &style->aim;
    }
    return &style->normal;
}

void gcp89_resolve(const GC89_Style *style,
                   const GC89_InputState *state,
                   GC89_DrawSpec *out_spec)
{
    const GC89_Variant *variant;
    int flags;
    GC89_Fixed spread;
    GC89_Fixed spread_delta;

    if (!out_spec) return;
    memset(out_spec, 0, sizeof(*out_spec));
    if (!style) return;

    flags = state ? state->flags : 0;
    if ((flags & GC89_STATE_DISABLED) != 0) return;
    variant = gcp89_choose_variant(style, flags);

    out_spec->visible = 1;
    out_spec->draw_mode = variant->draw_mode;
    out_spec->arm_mask = variant->arm_mask;
    out_spec->dot_enabled = variant->dot_enabled;
    out_spec->gap_fx = variant->gap_fx;
    out_spec->arm_length_fx = variant->arm_length_fx;
    out_spec->thickness_fx = variant->thickness_fx;
    out_spec->dot_size_fx = variant->dot_size_fx;
    out_spec->image_id = variant->image_id;
    out_spec->image_width_fx = variant->image_width_fx;
    out_spec->image_height_fx = variant->image_height_fx;
    out_spec->color_rgba = variant->color_rgba;
    out_spec->image_tint_rgba = variant->image_tint_rgba;
    out_spec->center_offset_x_fx = style->center_offset_x_fx;
    out_spec->center_offset_y_fx = style->center_offset_y_fx;
    out_spec->shape_type = variant->shape_type;
    out_spec->shape_segment_mask = variant->shape_segment_mask;
    out_spec->shape_direction_mask = variant->shape_direction_mask;
    out_spec->spread_mode = variant->spread_mode;
    out_spec->shape_radius_x_fx = variant->shape_radius_x_fx;
    out_spec->shape_radius_y_fx = variant->shape_radius_y_fx;
    out_spec->shape_depth_fx = variant->shape_depth_fx;
    out_spec->shape_rotation_deg_fx = variant->shape_rotation_deg_fx;
    out_spec->shape_break_fx = variant->shape_break_fx;
    out_spec->outline_enabled = variant->outline_enabled;
    out_spec->outline_width_fx = variant->outline_width_fx;
    out_spec->outline_color_rgba = variant->outline_color_rgba;

    if (!style->color_change_enabled) {
        out_spec->color_rgba = style->normal.color_rgba;
        out_spec->image_tint_rgba = style->normal.image_tint_rgba;
    }

    spread = state ? state->spread_fx : 0;
    if (spread < 0) spread = 0;
    spread_delta = gcp89_fx_mul(spread, style->spread_multiplier_fx);

    if ((out_spec->spread_mode & GC89_SPREAD_GAP) != 0) {
        out_spec->gap_fx += spread_delta;
        if (out_spec->gap_fx < 0) out_spec->gap_fx = 0;
    }
    if ((out_spec->spread_mode & GC89_SPREAD_SHAPE_SIZE) != 0) {
        out_spec->shape_radius_x_fx += spread_delta;
        out_spec->shape_radius_y_fx += spread_delta;
        if (out_spec->shape_radius_x_fx < 0) out_spec->shape_radius_x_fx = 0;
        if (out_spec->shape_radius_y_fx < 0) out_spec->shape_radius_y_fx = 0;
    }
}
