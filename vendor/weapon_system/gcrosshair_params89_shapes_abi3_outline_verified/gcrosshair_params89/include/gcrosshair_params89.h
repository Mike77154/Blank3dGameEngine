#ifndef GCROSSHAIR_PARAMS89_H
#define GCROSSHAIR_PARAMS89_H

#include "gcrosshair89_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void gcp89_variant_clear(GC89_Variant *variant);
void gcp89_style_clear(GC89_Style *style);

/* Original cross setter. It now also selects GC89_SHAPE_CROSS. */
void gcp89_variant_set_vector_px(GC89_Variant *variant,
                                 int gap_px,
                                 int arm_length_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 int arm_mask,
                                 unsigned long color_rgba);

/* Generic setter for all ABI 2 vector families. Sizes are half extents. */
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
                                unsigned long color_rgba);

void gcp89_variant_set_circle_px(GC89_Variant *variant,
                                 int radius_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 unsigned long color_rgba);

void gcp89_variant_set_square_px(GC89_Variant *variant,
                                 int half_size_px,
                                 int thickness_px,
                                 int dot_enabled,
                                 int dot_size_px,
                                 unsigned long color_rgba);

void gcp89_variant_set_diamond_px(GC89_Variant *variant,
                                  int radius_x_px,
                                  int radius_y_px,
                                  int thickness_px,
                                  int dot_enabled,
                                  int dot_size_px,
                                  unsigned long color_rgba);

void gcp89_variant_set_chevrons_px(GC89_Variant *variant,
                                   int radius_x_px,
                                   int radius_y_px,
                                   int depth_px,
                                   int thickness_px,
                                   int direction_mask,
                                   int dot_enabled,
                                   int dot_size_px,
                                   unsigned long color_rgba);

void gcp89_variant_set_hexagon_px(GC89_Variant *variant,
                                  int radius_x_px,
                                  int radius_y_px,
                                  int thickness_px,
                                  int dot_enabled,
                                  int dot_size_px,
                                  unsigned long color_rgba);

void gcp89_variant_set_brackets_px(GC89_Variant *variant,
                                   int radius_x_px,
                                   int radius_y_px,
                                   int hook_depth_px,
                                   int thickness_px,
                                   int direction_mask,
                                   int dot_enabled,
                                   int dot_size_px,
                                   unsigned long color_rgba);

void gcp89_variant_set_open_triangle_px(GC89_Variant *variant,
                                        int radius_x_px,
                                        int radius_y_px,
                                        int thickness_px,
                                        int side_mask,
                                        int dot_enabled,
                                        int dot_size_px,
                                        unsigned long color_rgba);

void gcp89_variant_set_shape_rotation_deg(GC89_Variant *variant,
                                          int degrees);
void gcp89_variant_set_shape_segments(GC89_Variant *variant,
                                      int segment_mask);
void gcp89_variant_set_shape_break_px(GC89_Variant *variant,
                                      int break_px);
void gcp89_variant_set_shape_directions(GC89_Variant *variant,
                                        int direction_mask);
void gcp89_variant_set_spread_mode(GC89_Variant *variant,
                                   int spread_mode);

/* Vector outline. Width is the visible border on each side in pixels. */
void gcp89_variant_set_outline_px(GC89_Variant *variant,
                                  int enabled,
                                  int width_px,
                                  unsigned long color_rgba);

void gcp89_variant_set_image_px(GC89_Variant *variant,
                                int image_id,
                                int width_px,
                                int height_px,
                                unsigned long tint_rgba,
                                int keep_vector);

void gcp89_style_set_center_offset_px(GC89_Style *style,
                                      int offset_x_px,
                                      int offset_y_px);
void gcp89_style_enable_color_change(GC89_Style *style, int enabled);
void gcp89_style_set_spread_multiplier(GC89_Style *style,
                                       GC89_Fixed multiplier_fx);

void gcp89_resolve(const GC89_Style *style,
                   const GC89_InputState *state,
                   GC89_DrawSpec *out_spec);

#ifdef __cplusplus
}
#endif

#endif
