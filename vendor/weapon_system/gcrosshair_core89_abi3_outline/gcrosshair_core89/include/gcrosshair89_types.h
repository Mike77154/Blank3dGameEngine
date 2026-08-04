#ifndef GCROSSHAIR89_TYPES_H
#define GCROSSHAIR89_TYPES_H

/*
 * Shared ABI for the three gcrosshair89 libraries.
 * C89, fixed-point Q16.16, no allocation and no renderer dependency.
 * Color format: 0xRRGGBBAA.
 *
 * ABI 2 appends vector-shape fields to GC89_Variant and GC89_DrawSpec.
 * ABI 3 appends vector-outline fields to both structures.
 * Rebuild every consumer that shares this header.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GC89_TYPES_ABI_VERSION 3

typedef long GC89_Fixed;

#define GC89_FX_SHIFT 16
#define GC89_FX_ONE   65536L
#define GC89_FX_HALF  32768L
#define GC89_FX_FROM_INT(v) ((GC89_Fixed)((long)(v) * GC89_FX_ONE))

#define GC89_RGBA(r,g,b,a) \
    ((((unsigned long)(r) & 255UL) << 24) | \
     (((unsigned long)(g) & 255UL) << 16) | \
     (((unsigned long)(b) & 255UL) << 8)  | \
      ((unsigned long)(a) & 255UL))

#define GC89_RGBA_R(c) ((int)(((unsigned long)(c) >> 24) & 255UL))
#define GC89_RGBA_G(c) ((int)(((unsigned long)(c) >> 16) & 255UL))
#define GC89_RGBA_B(c) ((int)(((unsigned long)(c) >> 8) & 255UL))
#define GC89_RGBA_A(c) ((int)((unsigned long)(c) & 255UL))

#define GC89_DRAW_VECTOR 1
#define GC89_DRAW_IMAGE  2
#define GC89_DRAW_HYBRID (GC89_DRAW_VECTOR | GC89_DRAW_IMAGE)

#define GC89_ARM_LEFT  1
#define GC89_ARM_RIGHT 2
#define GC89_ARM_UP    4
#define GC89_ARM_DOWN  8
#define GC89_ARM_ALL   15

#define GC89_DIRECTION_LEFT  GC89_ARM_LEFT
#define GC89_DIRECTION_RIGHT GC89_ARM_RIGHT
#define GC89_DIRECTION_UP    GC89_ARM_UP
#define GC89_DIRECTION_DOWN  GC89_ARM_DOWN
#define GC89_DIRECTION_ALL   GC89_ARM_ALL

#define GC89_STATE_AIM      1
#define GC89_STATE_FIRE     2
#define GC89_STATE_HIT      4
#define GC89_STATE_DISABLED 8

/* Vector families. CROSS preserves the original four-arm behavior. */
#define GC89_SHAPE_CROSS          0
#define GC89_SHAPE_CIRCLE         1
#define GC89_SHAPE_SQUARE         2
#define GC89_SHAPE_DIAMOND        3
#define GC89_SHAPE_CHEVRONS       4
#define GC89_SHAPE_HEXAGON        5
#define GC89_SHAPE_BRACKETS       6
#define GC89_SHAPE_OPEN_TRIANGLE  7
#define GC89_SHAPE_COUNT          8

/* Up to eight independently enabled sides, arcs or pieces. */
#define GC89_SEGMENT_0   1
#define GC89_SEGMENT_1   2
#define GC89_SEGMENT_2   4
#define GC89_SEGMENT_3   8
#define GC89_SEGMENT_4   16
#define GC89_SEGMENT_5   32
#define GC89_SEGMENT_6   64
#define GC89_SEGMENT_7   128
#define GC89_SEGMENT_ALL 255

/* Where dynamic spread is applied. */
#define GC89_SPREAD_NONE       0
#define GC89_SPREAD_GAP        1
#define GC89_SPREAD_SHAPE_SIZE 2
#define GC89_SPREAD_BOTH       3

typedef struct GC89_Variant {
    int draw_mode;
    int arm_mask;
    int dot_enabled;
    GC89_Fixed gap_fx;
    GC89_Fixed arm_length_fx;
    GC89_Fixed thickness_fx;
    GC89_Fixed dot_size_fx;
    int image_id;
    GC89_Fixed image_width_fx;
    GC89_Fixed image_height_fx;
    unsigned long color_rgba;
    unsigned long image_tint_rgba;

    /* ABI 2 vector-shape extension. */
    int shape_type;
    int shape_segment_mask;
    int shape_direction_mask;
    int spread_mode;
    GC89_Fixed shape_radius_x_fx;
    GC89_Fixed shape_radius_y_fx;
    GC89_Fixed shape_depth_fx;
    GC89_Fixed shape_rotation_deg_fx;
    GC89_Fixed shape_break_fx;

    /* ABI 3 vector-outline extension. */
    int outline_enabled;
    GC89_Fixed outline_width_fx;
    unsigned long outline_color_rgba;
} GC89_Variant;

typedef struct GC89_Style {
    GC89_Variant normal;
    GC89_Variant aim;
    GC89_Variant fire;
    GC89_Variant hit;
    int use_aim_variant;
    int use_fire_variant;
    int use_hit_variant;
    int color_change_enabled;
    GC89_Fixed spread_multiplier_fx;
    GC89_Fixed center_offset_x_fx;
    GC89_Fixed center_offset_y_fx;
} GC89_Style;

typedef struct GC89_InputState {
    int flags;
    GC89_Fixed spread_fx;
} GC89_InputState;

typedef struct GC89_DrawSpec {
    int visible;
    int draw_mode;
    int arm_mask;
    int dot_enabled;
    GC89_Fixed gap_fx;
    GC89_Fixed arm_length_fx;
    GC89_Fixed thickness_fx;
    GC89_Fixed dot_size_fx;
    int image_id;
    GC89_Fixed image_width_fx;
    GC89_Fixed image_height_fx;
    unsigned long color_rgba;
    unsigned long image_tint_rgba;
    GC89_Fixed center_offset_x_fx;
    GC89_Fixed center_offset_y_fx;

    /* ABI 2 vector-shape extension. */
    int shape_type;
    int shape_segment_mask;
    int shape_direction_mask;
    int spread_mode;
    GC89_Fixed shape_radius_x_fx;
    GC89_Fixed shape_radius_y_fx;
    GC89_Fixed shape_depth_fx;
    GC89_Fixed shape_rotation_deg_fx;
    GC89_Fixed shape_break_fx;

    /* ABI 3 vector-outline extension. */
    int outline_enabled;
    GC89_Fixed outline_width_fx;
    unsigned long outline_color_rgba;
} GC89_DrawSpec;

#ifdef __cplusplus
}
#endif

#endif
