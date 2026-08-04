#include <string.h>
#include "gcrosshair_base89.h"

#define GCB89_BH_AIM    1
#define GCB89_BH_FIRE   2
#define GCB89_BH_HIT    4
#define GCB89_BH_COLOR  8
#define GCB89_BH_SPREAD 16

typedef struct GCB89_PresetDef {
    const char *name;
    const char *category;
    int draw_mode;
    int arm_mask;
    int dot_enabled;
    int gap_px;
    int arm_length_px;
    int thickness_px;
    int dot_size_px;
    int image_id;
    int image_size_px;
    unsigned long color_rgba;
    int behavior;
    int spread_percent;
    int animation_kind;
} GCB89_PresetDef;

static const GCB89_PresetDef gcb89_presets[GCB89_PRESET_COUNT] = {
    /* 00: blank3d_default */
    { "blank3d_default", "default", 1, 15, 1,
      9, 13, 1, 3,
      0, 32, GC89_RGBA(209, 235, 255, 255),
      25, 100, 0 },
    /* 01: precision_pixel */
    { "precision_pixel", "precision", 1, 15, 1,
      2, 2, 1, 1,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 1 },
    /* 02: precision_dot */
    { "precision_dot", "precision", 1, 0, 1,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 1 },
    /* 03: precision_dot_large */
    { "precision_dot_large", "precision", 1, 0, 1,
      0, 0, 1, 4,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 04: classic_compact */
    { "classic_compact", "classic", 1, 15, 0,
      3, 5, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      16, 65, 1 },
    /* 05: classic_medium */
    { "classic_medium", "classic", 1, 15, 0,
      5, 8, 1, 2,
      0, 32, GC89_RGBA(40, 255, 160, 255),
      16, 85, 4 },
    /* 06: classic_open */
    { "classic_open", "classic", 1, 15, 0,
      9, 8, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      16, 100, 2 },
    /* 07: classic_long */
    { "classic_long", "classic", 1, 15, 0,
      4, 14, 1, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      17, 70, 4 },
    /* 08: classic_thick */
    { "classic_thick", "classic", 1, 15, 0,
      5, 8, 3, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      16, 80, 4 },
    /* 09: classic_no_dot */
    { "classic_no_dot", "classic", 1, 15, 0,
      7, 10, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      17, 90, 4 },
    /* 10: classic_split_dot */
    { "classic_split_dot", "classic", 1, 15, 1,
      7, 7, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      29, 80, 4 },
    /* 11: tactical_t */
    { "tactical_t", "tactical", 1, 7, 0,
      5, 9, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 12: tactical_inverted_t */
    { "tactical_inverted_t", "tactical", 1, 11, 0,
      5, 9, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 13: tactical_left_open */
    { "tactical_left_open", "tactical", 1, 14, 1,
      5, 9, 2, 2,
      0, 32, GC89_RGBA(40, 255, 160, 255),
      13, 0, 4 },
    /* 14: tactical_right_open */
    { "tactical_right_open", "tactical", 1, 13, 1,
      5, 9, 2, 2,
      0, 32, GC89_RGBA(40, 255, 160, 255),
      13, 0, 4 },
    /* 15: horizontal_micro */
    { "horizontal_micro", "minimal", 1, 3, 1,
      3, 4, 1, 1,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 1 },
    /* 16: horizontal_wide */
    { "horizontal_wide", "minimal", 1, 3, 0,
      6, 14, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      16, 50, 4 },
    /* 17: vertical_micro */
    { "vertical_micro", "minimal", 1, 12, 1,
      3, 4, 1, 1,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 1 },
    /* 18: vertical_wide */
    { "vertical_wide", "minimal", 1, 12, 0,
      6, 14, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      16, 50, 4 },
    /* 19: top_post */
    { "top_post", "minimal", 1, 4, 1,
      4, 12, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 20: bottom_post */
    { "bottom_post", "minimal", 1, 8, 1,
      4, 12, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 21: rifle_static */
    { "rifle_static", "rifle", 1, 15, 0,
      4, 7, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 22: rifle_dynamic */
    { "rifle_dynamic", "rifle", 1, 15, 0,
      4, 7, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      31, 100, 2 },
    /* 23: smg_tracking */
    { "smg_tracking", "tracking", 1, 15, 1,
      8, 6, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      30, 140, 2 },
    /* 24: pistol_crisp */
    { "pistol_crisp", "pistol", 1, 15, 1,
      4, 5, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      13, 0, 1 },
    /* 25: pistol_dot */
    { "pistol_dot", "pistol", 1, 0, 1,
      5, 8, 1, 3,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      13, 0, 1 },
    /* 26: shotgun_vector */
    { "shotgun_vector", "shotgun", 1, 15, 1,
      14, 5, 2, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      30, 170, 3 },
    /* 27: shotgun_dynamic */
    { "shotgun_dynamic", "shotgun", 1, 15, 0,
      18, 7, 3, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 220, 3 },
    /* 28: sniper_hairline */
    { "sniper_hairline", "sniper", 1, 15, 1,
      1, 22, 1, 1,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 29: sniper_post */
    { "sniper_post", "sniper", 1, 12, 1,
      1, 24, 1, 1,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 30: hipfire_wide */
    { "hipfire_wide", "hipfire", 1, 15, 0,
      16, 8, 2, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 180, 3 },
    /* 31: accessibility_large */
    { "accessibility_large", "accessibility", 1, 15, 1,
      8, 16, 4, 5,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      29, 70, 4 },
    /* 32: accessibility_bold */
    { "accessibility_bold", "accessibility", 1, 15, 1,
      5, 12, 5, 6,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      12, 0, 4 },
    /* 33: high_contrast_white */
    { "high_contrast_white", "accessibility", 1, 15, 1,
      6, 10, 3, 3,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 4 },
    /* 34: high_contrast_yellow */
    { "high_contrast_yellow", "accessibility", 1, 15, 1,
      6, 10, 3, 3,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      13, 0, 4 },
    /* 35: cyan_tracker */
    { "cyan_tracker", "tracking", 1, 15, 1,
      7, 8, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      30, 120, 2 },
    /* 36: lime_tracker */
    { "lime_tracker", "tracking", 1, 15, 1,
      7, 8, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      30, 120, 2 },
    /* 37: red_threat */
    { "red_threat", "themed", 1, 15, 1,
      5, 9, 2, 2,
      0, 32, GC89_RGBA(255, 64, 64, 255),
      22, 110, 2 },
    /* 38: magenta_neon */
    { "magenta_neon", "themed", 1, 15, 1,
      4, 8, 2, 2,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      23, 100, 2 },
    /* 39: ring_small */
    { "ring_small", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 22, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 40: ring_medium */
    { "ring_medium", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      18, 90, 2 },
    /* 41: ring_large */
    { "ring_large", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 46, GC89_RGBA(255, 235, 89, 255),
      18, 120, 3 },
    /* 42: ring_dot */
    { "ring_dot", "circular", 1, 0, 1,
      5, 8, 1, 2,
      0, 30, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 43: ring_cross */
    { "ring_cross", "circular", 1, 15, 0,
      18, 6, 1, 2,
      0, 34, GC89_RGBA(255, 255, 255, 255),
      13, 0, 4 },
    /* 44: double_ring */
    { "double_ring", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 36, GC89_RGBA(0, 255, 255, 255),
      18, 100, 2 },
    /* 45: segmented_ring */
    { "segmented_ring", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(80, 255, 80, 255),
      30, 110, 2 },
    /* 46: broken_circle */
    { "broken_circle", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 36, GC89_RGBA(255, 235, 89, 255),
      19, 100, 2 },
    /* 47: arc_brackets */
    { "arc_brackets", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 40, GC89_RGBA(0, 255, 255, 255),
      13, 0, 4 },
    /* 48: shotgun_ring */
    { "shotgun_ring", "shotgun", 1, 0, 1,
      5, 8, 1, 2,
      0, 54, GC89_RGBA(255, 150, 40, 255),
      30, 190, 3 },
    /* 49: circle_outer_cross */
    { "circle_outer_cross", "circular", 1, 15, 0,
      20, 10, 2, 2,
      0, 34, GC89_RGBA(255, 255, 255, 255),
      17, 75, 4 },
    /* 50: circle_inner_dot */
    { "circle_inner_dot", "circular", 1, 0, 1,
      5, 8, 1, 3,
      0, 38, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 51: diamond */
    { "diamond", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 30, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 52: diamond_dot */
    { "diamond_dot", "geometric", 1, 0, 1,
      5, 8, 1, 2,
      0, 34, GC89_RGBA(255, 235, 89, 255),
      13, 0, 1 },
    /* 53: triangle */
    { "triangle", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 34, GC89_RGBA(80, 255, 80, 255),
      13, 0, 1 },
    /* 54: triangle_dot */
    { "triangle_dot", "geometric", 1, 0, 1,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(255, 235, 89, 255),
      13, 0, 1 },
    /* 55: chevron_up */
    { "chevron_up", "chevron", 1, 0, 0,
      5, 8, 1, 2,
      0, 30, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 56: chevron_down */
    { "chevron_down", "chevron", 1, 0, 0,
      5, 8, 1, 2,
      0, 30, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 57: double_chevron */
    { "double_chevron", "chevron", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(80, 255, 80, 255),
      31, 80, 2 },
    /* 58: x_cross */
    { "x_cross", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 30, GC89_RGBA(255, 64, 220, 255),
      13, 0, 1 },
    /* 59: x_dot */
    { "x_dot", "geometric", 1, 0, 1,
      5, 8, 1, 2,
      0, 34, GC89_RGBA(255, 64, 220, 255),
      13, 0, 1 },
    /* 60: hex_ring */
    { "hex_ring", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 36, GC89_RGBA(0, 255, 255, 255),
      18, 90, 2 },
    /* 61: octagon_ring */
    { "octagon_ring", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(80, 255, 80, 255),
      18, 90, 2 },
    /* 62: square_ring */
    { "square_ring", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 34, GC89_RGBA(255, 255, 255, 255),
      13, 0, 4 },
    /* 63: corner_box */
    { "corner_box", "brackets", 1, 0, 0,
      5, 8, 1, 2,
      0, 40, GC89_RGBA(0, 255, 255, 255),
      19, 100, 2 },
    /* 64: focus_brackets */
    { "focus_brackets", "brackets", 1, 0, 0,
      5, 8, 1, 2,
      0, 42, GC89_RGBA(80, 255, 80, 255),
      13, 0, 4 },
    /* 65: sci_fi_brackets */
    { "sci_fi_brackets", "brackets", 1, 0, 0,
      5, 8, 1, 2,
      0, 46, GC89_RGBA(255, 64, 220, 255),
      31, 100, 2 },
    /* 66: semicircle_top */
    { "semicircle_top", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 67: semicircle_bottom */
    { "semicircle_bottom", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 68: horseshoe */
    { "horseshoe", "circular", 1, 0, 0,
      5, 8, 1, 2,
      0, 40, GC89_RGBA(255, 235, 89, 255),
      19, 100, 2 },
    /* 69: bullseye */
    { "bullseye", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 42, GC89_RGBA(255, 64, 64, 255),
      13, 0, 1 },
    /* 70: radar_spokes */
    { "radar_spokes", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 46, GC89_RGBA(80, 255, 80, 255),
      19, 70, 2 },
    /* 71: rangefinder */
    { "rangefinder", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 54, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 72: compass */
    { "compass", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 44, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 73: three_prong */
    { "three_prong", "arena", 1, 0, 0,
      5, 8, 1, 2,
      0, 36, GC89_RGBA(80, 255, 80, 255),
      19, 100, 2 },
    /* 74: four_corner */
    { "four_corner", "arena", 1, 0, 0,
      5, 8, 1, 2,
      0, 42, GC89_RGBA(0, 255, 255, 255),
      13, 0, 4 },
    /* 75: segmented_square */
    { "segmented_square", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 40, GC89_RGBA(255, 235, 89, 255),
      18, 100, 2 },
    /* 76: capsule */
    { "capsule", "geometric", 1, 0, 0,
      5, 8, 1, 2,
      0, 40, GC89_RGBA(255, 64, 220, 255),
      31, 90, 2 },
    /* 77: ring_chevron */
    { "ring_chevron", "hybrid", 1, 8, 0,
      20, 10, 3, 2,
      0, 38, GC89_RGBA(0, 255, 255, 255),
      13, 0, 4 },
    /* 78: diamond_cross */
    { "diamond_cross", "hybrid", 1, 15, 0,
      21, 7, 1, 2,
      0, 38, GC89_RGBA(255, 255, 255, 255),
      13, 0, 4 },
    /* 79: octagon_dot */
    { "octagon_dot", "hybrid", 1, 0, 1,
      5, 8, 1, 3,
      0, 38, GC89_RGBA(80, 255, 80, 255),
      13, 0, 1 },
    /* 80: target_lock */
    { "target_lock", "brackets", 1, 0, 0,
      5, 8, 1, 2,
      0, 48, GC89_RGBA(255, 64, 64, 255),
      23, 120, 2 },
    /* 81: minimal_u */
    { "minimal_u", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 28, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 82: minimal_n */
    { "minimal_n", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 28, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 83: dual_horizontal */
    { "dual_horizontal", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(255, 235, 89, 255),
      18, 90, 2 },
    /* 84: dual_vertical */
    { "dual_vertical", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(255, 235, 89, 255),
      18, 90, 2 },
    /* 85: left_corner */
    { "left_corner", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      13, 0, 1 },
    /* 86: right_corner */
    { "right_corner", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      13, 0, 1 },
    /* 87: top_corners */
    { "top_corners", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 88: bottom_corners */
    { "bottom_corners", "minimal", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 89: scope_mil_dot */
    { "scope_mil_dot", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 58, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 90: scope_duplex */
    { "scope_duplex", "scope", 1, 0, 0,
      5, 8, 1, 2,
      0, 58, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 91: scope_circle */
    { "scope_circle", "scope", 1, 15, 0,
      29, 14, 1, 2,
      0, 56, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 92: scope_circle_dot */
    { "scope_circle_dot", "scope", 1, 15, 1,
      29, 14, 1, 2,
      0, 56, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 93: arena_brackets */
    { "arena_brackets", "arena", 1, 0, 0,
      5, 8, 1, 2,
      0, 44, GC89_RGBA(80, 255, 80, 255),
      19, 100, 2 },
    /* 94: mech_lock */
    { "mech_lock", "themed", 1, 0, 0,
      5, 8, 1, 2,
      0, 48, GC89_RGBA(255, 64, 220, 255),
      31, 110, 2 },
    /* 95: retro_arcade */
    { "retro_arcade", "themed", 1, 0, 0,
      5, 8, 1, 2,
      0, 38, GC89_RGBA(40, 255, 160, 255),
      30, 130, 2 },
    /* 96: tac_dot_micro */
    { "tac_dot_micro", "precision", 1, 0, 1,
      0, 0, 1, 1,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 1 },
    /* 97: tac_dot_ring */
    { "tac_dot_ring", "precision", 1, 0, 1,
      0, 0, 1, 2,
      0, 14, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 98: hitscan_short_cross */
    { "hitscan_short_cross", "hitscan", 1, 15, 0,
      2, 4, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      1, 0, 1 },
    /* 99: hitscan_pixel_gap */
    { "hitscan_pixel_gap", "hitscan", 1, 15, 1,
      1, 2, 1, 1,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      12, 0, 1 },
    /* 100: rail_dot */
    { "rail_dot", "arena", 1, 0, 1,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      12, 0, 1 },
    /* 101: rail_cross */
    { "rail_cross", "arena", 1, 15, 1,
      5, 6, 1, 1,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      5, 0, 1 },
    /* 102: burst_rifle_tight */
    { "burst_rifle_tight", "rifle", 1, 15, 0,
      3, 5, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      19, 55, 1 },
    /* 103: burst_rifle_dynamic */
    { "burst_rifle_dynamic", "rifle", 1, 15, 0,
      4, 6, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      19, 95, 2 },
    /* 104: recoil_compensator_t */
    { "recoil_compensator_t", "tactical", 1, 11, 1,
      4, 8, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 80, 2 },
    /* 105: headshot_tiny */
    { "headshot_tiny", "precision", 1, 15, 1,
      2, 3, 1, 1,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 106: center_post_up */
    { "center_post_up", "precision", 1, 4, 1,
      2, 9, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 107: center_post_down */
    { "center_post_down", "precision", 1, 8, 1,
      2, 9, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 108: cs_static_micro */
    { "cs_static_micro", "competitive", 1, 15, 0,
      2, 4, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      0, 0, 1 },
    /* 109: cs_static_dense */
    { "cs_static_dense", "competitive", 1, 15, 0,
      0, 5, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      0, 0, 1 },
    /* 110: cs_static_open */
    { "cs_static_open", "competitive", 1, 15, 0,
      6, 7, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      0, 0, 1 },
    /* 111: cs_dynamic_classic */
    { "cs_dynamic_classic", "competitive", 1, 15, 0,
      4, 6, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      18, 85, 2 },
    /* 112: cs_dynamic_burst */
    { "cs_dynamic_burst", "competitive", 1, 15, 1,
      3, 5, 2, 1,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      18, 65, 2 },
    /* 113: valorant_inner_short */
    { "valorant_inner_short", "competitive", 1, 15, 0,
      3, 4, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 114: valorant_inner_open */
    { "valorant_inner_open", "competitive", 1, 15, 1,
      5, 6, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      12, 0, 1 },
    /* 115: valorant_square_dot */
    { "valorant_square_dot", "competitive", 1, 15, 1,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 116: quake_plus */
    { "quake_plus", "arena", 1, 15, 0,
      0, 8, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      18, 50, 1 },
    /* 117: quake_cross_circle */
    { "quake_cross_circle", "arena", 1, 15, 0,
      3, 6, 1, 2,
      0, 24, GC89_RGBA(0, 255, 255, 255),
      18, 75, 2 },
    /* 118: arena_lightning_dot */
    { "arena_lightning_dot", "tracking", 1, 0, 1,
      0, 0, 1, 3,
      0, 32, GC89_RGBA(170, 255, 60, 255),
      14, 0, 1 },
    /* 119: arena_rail_cross */
    { "arena_rail_cross", "arena", 1, 15, 1,
      7, 9, 1, 1,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      13, 0, 1 },
    /* 120: tracker_ring_tiny */
    { "tracker_ring_tiny", "tracking", 1, 15, 0,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      0, 0, 1 },
    /* 121: tracker_ring_dot */
    { "tracker_ring_dot", "tracking", 1, 15, 1,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 122: tracking_ellipse */
    { "tracking_ellipse", "tracking", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      18, 65, 2 },
    /* 123: beam_ring */
    { "beam_ring", "tracking", 1, 15, 1,
      0, 0, 2, 3,
      0, 32, GC89_RGBA(170, 255, 60, 255),
      18, 40, 1 },
    /* 124: projectile_lead_circle */
    { "projectile_lead_circle", "projectile", 1, 15, 1,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 110, 2 },
    /* 125: projectile_lead_broken */
    { "projectile_lead_broken", "projectile", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 120, 2 },
    /* 126: melee_large_ring */
    { "melee_large_ring", "melee", 1, 15, 0,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 120, 3 },
    /* 127: aoe_wide_ring */
    { "aoe_wide_ring", "aoe", 1, 15, 1,
      0, 0, 2, 3,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      18, 140, 3 },
    /* 128: shotgun_circle */
    { "shotgun_circle", "shotgun", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 160, 3 },
    /* 129: shotgun_circle_dynamic */
    { "shotgun_circle_dynamic", "shotgun", 1, 15, 0,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      19, 220, 3 },
    /* 130: circle_top_arc */
    { "circle_top_arc", "circular", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 131: circle_bottom_arc */
    { "circle_bottom_arc", "circular", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 132: shotgun_four_posts */
    { "shotgun_four_posts", "shotgun", 1, 15, 1,
      14, 5, 3, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 180, 3 },
    /* 133: shotgun_wide_cross */
    { "shotgun_wide_cross", "shotgun", 1, 15, 0,
      11, 12, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 170, 3 },
    /* 134: shotgun_bloom_cross */
    { "shotgun_bloom_cross", "shotgun", 1, 15, 1,
      8, 8, 3, 3,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      19, 240, 3 },
    /* 135: shotgun_square */
    { "shotgun_square", "shotgun", 1, 15, 1,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 170, 3 },
    /* 136: shotgun_diamond */
    { "shotgun_diamond", "shotgun", 1, 15, 1,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 180, 3 },
    /* 137: shotgun_hex */
    { "shotgun_hex", "shotgun", 1, 15, 0,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 190, 3 },
    /* 138: buckshot_brackets */
    { "buckshot_brackets", "shotgun", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 190, 3 },
    /* 139: slug_precision */
    { "slug_precision", "shotgun", 1, 15, 1,
      2, 5, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      13, 0, 1 },
    /* 140: hipfire_heavy */
    { "hipfire_heavy", "hipfire", 1, 15, 0,
      13, 10, 4, 2,
      0, 32, GC89_RGBA(255, 64, 64, 255),
      18, 170, 3 },
    /* 141: hipfire_vehicle */
    { "hipfire_vehicle", "vehicle", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      18, 140, 2 },
    /* 142: spray_control */
    { "spray_control", "smg", 1, 11, 1,
      5, 8, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      18, 110, 2 },
    /* 143: smg_bloom */
    { "smg_bloom", "smg", 1, 15, 1,
      5, 6, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      19, 135, 2 },
    /* 144: square_micro_vector */
    { "square_micro_vector", "geometric", 1, 15, 0,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      0, 0, 1 },
    /* 145: square_dot_vector */
    { "square_dot_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 146: square_broken_vector */
    { "square_broken_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 70, 2 },
    /* 147: square_top_open */
    { "square_top_open", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 148: diamond_micro_vector */
    { "diamond_micro_vector", "geometric", 1, 15, 0,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      0, 0, 1 },
    /* 149: diamond_dot_vector */
    { "diamond_dot_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 150: diamond_broken_vector */
    { "diamond_broken_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 75, 2 },
    /* 151: diamond_horizontal */
    { "diamond_horizontal", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      1, 0, 1 },
    /* 152: hex_micro_vector */
    { "hex_micro_vector", "geometric", 1, 15, 0,
      0, 0, 1, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      0, 0, 1 },
    /* 153: hex_dot_vector */
    { "hex_dot_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 154: hex_broken_vector */
    { "hex_broken_vector", "geometric", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 85, 2 },
    /* 155: hex_wide_vector */
    { "hex_wide_vector", "geometric", 1, 15, 0,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(170, 100, 255, 255),
      18, 100, 2 },
    /* 156: chevron_up_micro_vector */
    { "chevron_up_micro_vector", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 157: chevron_down_micro_vector */
    { "chevron_down_micro_vector", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 158: chevron_left_vector */
    { "chevron_left_vector", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 159: chevron_right_vector */
    { "chevron_right_vector", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 160: chevron_double_vertical */
    { "chevron_double_vertical", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 85, 2 },
    /* 161: chevron_horizontal */
    { "chevron_horizontal", "chevron", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      18, 85, 2 },
    /* 162: bracket_lr_tight_vector */
    { "bracket_lr_tight_vector", "brackets", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 163: bracket_lr_wide_vector */
    { "bracket_lr_wide_vector", "brackets", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      18, 100, 2 },
    /* 164: bracket_ud_vector */
    { "bracket_ud_vector", "brackets", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 165: bracket_four_vector */
    { "bracket_four_vector", "brackets", 1, 15, 1,
      0, 0, 2, 3,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 110, 2 },
    /* 166: bracket_three_sided */
    { "bracket_three_sided", "brackets", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(170, 100, 255, 255),
      13, 0, 1 },
    /* 167: bracket_dynamic_lock */
    { "bracket_dynamic_lock", "brackets", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(255, 64, 64, 255),
      31, 130, 3 },
    /* 168: triangle_open_up_vector */
    { "triangle_open_up_vector", "triangle", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 169: triangle_open_down_vector */
    { "triangle_open_down_vector", "triangle", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      1, 0, 1 },
    /* 170: triangle_open_left_vector */
    { "triangle_open_left_vector", "triangle", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 171: triangle_open_right_vector */
    { "triangle_open_right_vector", "triangle", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      12, 0, 1 },
    /* 172: triangle_corner_three */
    { "triangle_corner_three", "triangle", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      18, 90, 2 },
    /* 173: triangle_dot_vector */
    { "triangle_dot_vector", "triangle", 1, 15, 1,
      0, 0, 2, 3,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      12, 0, 1 },
    /* 174: triangle_broken_vector */
    { "triangle_broken_vector", "triangle", 1, 15, 0,
      0, 0, 3, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 120, 2 },
    /* 175: triangle_scope_post */
    { "triangle_scope_post", "scope", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      1, 0, 1 },
    /* 176: scope_duplex_vector */
    { "scope_duplex_vector", "scope", 1, 15, 1,
      5, 18, 1, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      1, 0, 1 },
    /* 177: scope_mildot_vector */
    { "scope_mildot_vector", "scope", 1, 15, 1,
      3, 20, 1, 3,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      5, 0, 1 },
    /* 178: scope_range_cross */
    { "scope_range_cross", "scope", 1, 11, 1,
      4, 22, 1, 2,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      1, 0, 1 },
    /* 179: scope_ring_precision */
    { "scope_ring_precision", "scope", 1, 15, 1,
      4, 10, 1, 2,
      0, 34, GC89_RGBA(255, 255, 255, 255),
      5, 0, 1 },
    /* 180: accessibility_cyan_large */
    { "accessibility_cyan_large", "accessibility", 1, 15, 1,
      8, 14, 4, 4,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      12, 0, 4 },
    /* 181: accessibility_yellow_large */
    { "accessibility_yellow_large", "accessibility", 1, 15, 1,
      0, 0, 4, 4,
      0, 32, GC89_RGBA(255, 235, 89, 255),
      12, 0, 4 },
    /* 182: accessibility_magenta_bold */
    { "accessibility_magenta_bold", "accessibility", 1, 15, 1,
      0, 0, 5, 5,
      0, 32, GC89_RGBA(255, 64, 220, 255),
      12, 0, 4 },
    /* 183: accessibility_white_bold */
    { "accessibility_white_bold", "accessibility", 1, 15, 1,
      7, 12, 5, 5,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      12, 0, 4 },
    /* 184: color_change_threat_vector */
    { "color_change_threat_vector", "feedback", 1, 15, 1,
      0, 0, 2, 3,
      0, 32, GC89_RGBA(80, 255, 80, 255),
      15, 0, 2 },
    /* 185: color_change_confirm_vector */
    { "color_change_confirm_vector", "feedback", 1, 15, 1,
      3, 6, 2, 2,
      0, 32, GC89_RGBA(0, 255, 255, 255),
      13, 0, 1 },
    /* 186: dynamic_jump_bloom */
    { "dynamic_jump_bloom", "dynamic", 1, 15, 1,
      0, 0, 2, 2,
      0, 32, GC89_RGBA(80, 160, 255, 255),
      18, 200, 3 },
    /* 187: dynamic_fire_bloom */
    { "dynamic_fire_bloom", "dynamic", 1, 15, 1,
      4, 7, 2, 2,
      0, 32, GC89_RGBA(255, 150, 40, 255),
      18, 180, 3 },
    /* 188: dynamic_full_bloom */
    { "dynamic_full_bloom", "dynamic", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(255, 64, 64, 255),
      31, 220, 3 },
    /* 189: minimalist_no_dot_vector */
    { "minimalist_no_dot_vector", "minimal", 1, 3, 0,
      4, 5, 1, 2,
      0, 32, GC89_RGBA(255, 255, 255, 255),
      0, 0, 1 },
    /* 190: retro_hex_neon_vector */
    { "retro_hex_neon_vector", "themed", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(40, 255, 190, 255),
      30, 100, 2 },
    /* 191: sci_fi_lock_vector */
    { "sci_fi_lock_vector", "themed", 1, 15, 1,
      0, 0, 3, 3,
      0, 32, GC89_RGBA(255, 120, 210, 255),
      31, 130, 3 }
};

typedef struct GCB89_AssetDef {
    int image_id;
    const char *filename;
} GCB89_AssetDef;

static const GCB89_AssetDef gcb89_assets[] = {
    { 1000, "assets/ring.tga" },
    { 1001, "assets/double_ring.tga" },
    { 1002, "assets/segmented_ring.tga" },
    { 1003, "assets/broken_circle.tga" },
    { 1004, "assets/arc_brackets.tga" },
    { 1005, "assets/diamond.tga" },
    { 1006, "assets/triangle.tga" },
    { 1007, "assets/chevron_up.tga" },
    { 1008, "assets/chevron_down.tga" },
    { 1009, "assets/double_chevron.tga" },
    { 1010, "assets/x_cross.tga" },
    { 1011, "assets/hexagon.tga" },
    { 1012, "assets/octagon.tga" },
    { 1013, "assets/square_ring.tga" },
    { 1014, "assets/corner_box.tga" },
    { 1015, "assets/focus_brackets.tga" },
    { 1016, "assets/sci_fi_brackets.tga" },
    { 1017, "assets/semicircle_top.tga" },
    { 1018, "assets/semicircle_bottom.tga" },
    { 1019, "assets/horseshoe.tga" },
    { 1020, "assets/triple_arcs.tga" },
    { 1021, "assets/bullseye.tga" },
    { 1022, "assets/radar_spokes.tga" },
    { 1023, "assets/compass.tga" },
    { 1024, "assets/three_prong.tga" },
    { 1025, "assets/four_corner.tga" },
    { 1026, "assets/segmented_square.tga" },
    { 1027, "assets/rangefinder.tga" },
    { 1028, "assets/capsule.tga" },
    { 1029, "assets/minimal_u.tga" },
    { 1030, "assets/minimal_n.tga" },
    { 1031, "assets/dual_horizontal.tga" },
    { 1032, "assets/dual_vertical.tga" },
    { 1033, "assets/left_corner.tga" },
    { 1034, "assets/right_corner.tga" },
    { 1035, "assets/top_corners.tga" },
    { 1036, "assets/bottom_corners.tga" },
    { 1037, "assets/scope_mil_dot.tga" },
    { 1038, "assets/scope_duplex.tga" },
    { 1039, "assets/arena_brackets.tga" },
    { 1040, "assets/mech_lock.tga" },
    { 1041, "assets/retro_arcade.tga" }
};

#define GCB89_ASSET_COUNT \
    ((int)(sizeof(gcb89_assets) / sizeof(gcb89_assets[0])))


typedef struct GCB89_ShapeDef {
    int shape_type;
    int segment_mask;
    int direction_mask;
    int spread_mode;
    int radius_x_px;
    int radius_y_px;
    int depth_px;
    int rotation_deg;
    int break_px;
} GCB89_ShapeDef;

static const GCB89_ShapeDef gcb89_shape_defs[GCB89_PRESET_COUNT] = {
    /* 0: blank3d_default */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 1: precision_pixel */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 2: precision_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 3: precision_dot_large */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 4: classic_compact */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 5: classic_medium */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 6: classic_open */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 7: classic_long */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 8: classic_thick */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 9: classic_no_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 10: classic_split_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 11: tactical_t */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 12: tactical_inverted_t */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 13: tactical_left_open */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 14: tactical_right_open */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 15: horizontal_micro */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 16: horizontal_wide */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 17: vertical_micro */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 18: vertical_wide */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 19: top_post */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 20: bottom_post */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 21: rifle_static */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 22: rifle_dynamic */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 23: smg_tracking */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 24: pistol_crisp */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 25: pistol_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 26: shotgun_vector */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 27: shotgun_dynamic */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 28: sniper_hairline */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 29: sniper_post */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 30: hipfire_wide */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 31: accessibility_large */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 32: accessibility_bold */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 33: high_contrast_white */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 34: high_contrast_yellow */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 35: cyan_tracker */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 36: lime_tracker */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 37: red_threat */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 38: magenta_neon */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 39: ring_small */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 40: ring_medium */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 41: ring_large */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 42: ring_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 43: ring_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 44: double_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 45: segmented_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 46: broken_circle */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 47: arc_brackets */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 48: shotgun_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 49: circle_outer_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 50: circle_inner_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 51: diamond */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 52: diamond_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 53: triangle */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 54: triangle_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 55: chevron_up */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 56: chevron_down */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 57: double_chevron */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 58: x_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 59: x_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 60: hex_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 61: octagon_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 62: square_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 63: corner_box */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 64: focus_brackets */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 65: sci_fi_brackets */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 66: semicircle_top */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 67: semicircle_bottom */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 68: horseshoe */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 69: bullseye */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 70: radar_spokes */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 71: rangefinder */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 72: compass */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 73: three_prong */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 74: four_corner */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 75: segmented_square */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 76: capsule */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 77: ring_chevron */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 78: diamond_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 79: octagon_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 80: target_lock */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 81: minimal_u */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 82: minimal_n */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 83: dual_horizontal */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 84: dual_vertical */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 85: left_corner */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 86: right_corner */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 87: top_corners */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 88: bottom_corners */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 89: scope_mil_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 90: scope_duplex */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 91: scope_circle */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 92: scope_circle_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 93: arena_brackets */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 94: mech_lock */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 95: retro_arcade */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 96: tac_dot_micro */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 97: tac_dot_ring */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 98: hitscan_short_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 99: hitscan_pixel_gap */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 100: rail_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 101: rail_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 102: burst_rifle_tight */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 103: burst_rifle_dynamic */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 104: recoil_compensator_t */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 105: headshot_tiny */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 106: center_post_up */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 107: center_post_down */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 108: cs_static_micro */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 109: cs_static_dense */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 110: cs_static_open */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 111: cs_dynamic_classic */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 112: cs_dynamic_burst */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 113: valorant_inner_short */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 114: valorant_inner_open */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 115: valorant_square_dot */
    { GC89_SHAPE_SQUARE, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      4, 4, 0, 0, 2 },
    /* 116: quake_plus */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 117: quake_cross_circle */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 118: arena_lightning_dot */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 119: arena_rail_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 120: tracker_ring_tiny */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      6, 6, 0, 0, 0 },
    /* 121: tracker_ring_dot */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      9, 9, 0, 0, 0 },
    /* 122: tracking_ellipse */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      13, 8, 0, 0, 0 },
    /* 123: beam_ring */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      11, 11, 0, 0, 0 },
    /* 124: projectile_lead_circle */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      15, 15, 0, 0, 0 },
    /* 125: projectile_lead_broken */
    { GC89_SHAPE_CIRCLE, 85, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      16, 16, 0, 0, 2 },
    /* 126: melee_large_ring */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      24, 24, 0, 0, 0 },
    /* 127: aoe_wide_ring */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      28, 20, 0, 0, 0 },
    /* 128: shotgun_circle */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      18, 18, 0, 0, 0 },
    /* 129: shotgun_circle_dynamic */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      20, 20, 0, 0, 0 },
    /* 130: circle_top_arc */
    { GC89_SHAPE_CIRCLE, 195, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      14, 12, 0, 0, 0 },
    /* 131: circle_bottom_arc */
    { GC89_SHAPE_CIRCLE, 60, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      14, 12, 0, 0, 0 },
    /* 132: shotgun_four_posts */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 133: shotgun_wide_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 134: shotgun_bloom_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 135: shotgun_square */
    { GC89_SHAPE_SQUARE, 15, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      18, 18, 0, 0, 5 },
    /* 136: shotgun_diamond */
    { GC89_SHAPE_DIAMOND, 15, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      20, 16, 0, 0, 4 },
    /* 137: shotgun_hex */
    { GC89_SHAPE_HEXAGON, 63, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      21, 17, 0, 0, 4 },
    /* 138: buckshot_brackets */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, 3, GC89_SPREAD_SHAPE_SIZE,
      22, 15, 7, 0, 0 },
    /* 139: slug_precision */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 140: hipfire_heavy */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 141: hipfire_vehicle */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      25, 18, 8, 0, 0 },
    /* 142: spray_control */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 143: smg_bloom */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 144: square_micro_vector */
    { GC89_SHAPE_SQUARE, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      5, 5, 0, 0, 0 },
    /* 145: square_dot_vector */
    { GC89_SHAPE_SQUARE, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      9, 9, 0, 0, 0 },
    /* 146: square_broken_vector */
    { GC89_SHAPE_SQUARE, 15, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      13, 13, 0, 0, 5 },
    /* 147: square_top_open */
    { GC89_SHAPE_SQUARE, 14, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      12, 10, 0, 0, 0 },
    /* 148: diamond_micro_vector */
    { GC89_SHAPE_DIAMOND, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      6, 7, 0, 0, 0 },
    /* 149: diamond_dot_vector */
    { GC89_SHAPE_DIAMOND, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      10, 12, 0, 0, 0 },
    /* 150: diamond_broken_vector */
    { GC89_SHAPE_DIAMOND, 15, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      14, 16, 0, 0, 5 },
    /* 151: diamond_horizontal */
    { GC89_SHAPE_DIAMOND, 15, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      16, 9, 0, 0, 0 },
    /* 152: hex_micro_vector */
    { GC89_SHAPE_HEXAGON, 63, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      7, 7, 0, 0, 0 },
    /* 153: hex_dot_vector */
    { GC89_SHAPE_HEXAGON, 63, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      11, 10, 0, 0, 0 },
    /* 154: hex_broken_vector */
    { GC89_SHAPE_HEXAGON, 21, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      15, 13, 0, 0, 3 },
    /* 155: hex_wide_vector */
    { GC89_SHAPE_HEXAGON, 63, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      20, 13, 0, 0, 0 },
    /* 156: chevron_up_micro_vector */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 4, GC89_SPREAD_NONE,
      8, 8, 5, 0, 0 },
    /* 157: chevron_down_micro_vector */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 8, GC89_SPREAD_NONE,
      8, 8, 5, 0, 0 },
    /* 158: chevron_left_vector */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 1, GC89_SPREAD_NONE,
      10, 9, 6, 0, 0 },
    /* 159: chevron_right_vector */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 2, GC89_SPREAD_NONE,
      10, 9, 6, 0, 0 },
    /* 160: chevron_double_vertical */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 12, GC89_SPREAD_SHAPE_SIZE,
      13, 13, 7, 0, 0 },
    /* 161: chevron_horizontal */
    { GC89_SHAPE_CHEVRONS, GC89_SEGMENT_ALL, 3, GC89_SPREAD_SHAPE_SIZE,
      13, 11, 7, 0, 0 },
    /* 162: bracket_lr_tight_vector */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, 3, GC89_SPREAD_NONE,
      10, 8, 4, 0, 0 },
    /* 163: bracket_lr_wide_vector */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, 3, GC89_SPREAD_SHAPE_SIZE,
      18, 12, 6, 0, 0 },
    /* 164: bracket_ud_vector */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, 12, GC89_SPREAD_NONE,
      12, 16, 5, 0, 0 },
    /* 165: bracket_four_vector */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      18, 15, 6, 0, 0 },
    /* 166: bracket_three_sided */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, 11, GC89_SPREAD_NONE,
      16, 13, 6, 0, 0 },
    /* 167: bracket_dynamic_lock */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      17, 14, 7, 0, 0 },
    /* 168: triangle_open_up_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 5, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      10, 10, 0, 0, 0 },
    /* 169: triangle_open_down_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 5, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      10, 10, 0, 180, 0 },
    /* 170: triangle_open_left_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 5, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      10, 10, 0, 270, 0 },
    /* 171: triangle_open_right_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 5, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      10, 10, 0, 90, 0 },
    /* 172: triangle_corner_three */
    { GC89_SHAPE_OPEN_TRIANGLE, 7, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      14, 14, 0, 0, 6 },
    /* 173: triangle_dot_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 7, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      12, 12, 0, 0, 0 },
    /* 174: triangle_broken_vector */
    { GC89_SHAPE_OPEN_TRIANGLE, 7, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      17, 15, 0, 0, 5 },
    /* 175: triangle_scope_post */
    { GC89_SHAPE_OPEN_TRIANGLE, 5, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      9, 13, 0, 180, 0 },
    /* 176: scope_duplex_vector */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 177: scope_mildot_vector */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 178: scope_range_cross */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 179: scope_ring_precision */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 180: accessibility_cyan_large */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 181: accessibility_yellow_large */
    { GC89_SHAPE_CIRCLE, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      18, 18, 0, 0, 0 },
    /* 182: accessibility_magenta_bold */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      20, 16, 8, 0, 0 },
    /* 183: accessibility_white_bold */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 184: color_change_threat_vector */
    { GC89_SHAPE_HEXAGON, 63, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      13, 12, 0, 0, 0 },
    /* 185: color_change_confirm_vector */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 186: dynamic_jump_bloom */
    { GC89_SHAPE_CIRCLE, 85, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      14, 14, 0, 0, 0 },
    /* 187: dynamic_fire_bloom */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_GAP,
      0, 0, 0, 0, 0 },
    /* 188: dynamic_full_bloom */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      16, 14, 7, 0, 0 },
    /* 189: minimalist_no_dot_vector */
    { GC89_SHAPE_CROSS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_NONE,
      0, 0, 0, 0, 0 },
    /* 190: retro_hex_neon_vector */
    { GC89_SHAPE_HEXAGON, 42, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      16, 14, 0, 0, 2 },
    /* 191: sci_fi_lock_vector */
    { GC89_SHAPE_BRACKETS, GC89_SEGMENT_ALL, GC89_DIRECTION_ALL, GC89_SPREAD_SHAPE_SIZE,
      18, 14, 8, 0, 0 }
};

static void gcb89_clear_variant(GC89_Variant *variant)
{
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

static void gcb89_copy_variant(GC89_Variant *dst, const GC89_Variant *src)
{
    *dst = *src;
}

static GC89_Fixed gcb89_percent_fx(int percent)
{
    return (GC89_Fixed)(((long)percent * GC89_FX_ONE) / 100L);
}

static GC89_Fixed gcb89_sub_pixels_clamped(GC89_Fixed value, int pixels)
{
    GC89_Fixed delta;

    delta = GC89_FX_FROM_INT(pixels);
    if (value <= delta) return 0;
    return value - delta;
}

static void gcb89_apply_definition(const GCB89_PresetDef *def,
                                   const GCB89_ShapeDef *shape_def,
                                   GC89_Style *style)
{
    GC89_Variant *normal;
    GC89_Variant *aim;
    GC89_Variant *fire;
    GC89_Variant *hit;

    memset(style, 0, sizeof(*style));
    gcb89_clear_variant(&style->normal);
    gcb89_clear_variant(&style->aim);
    gcb89_clear_variant(&style->fire);
    gcb89_clear_variant(&style->hit);

    normal = &style->normal;
    aim = &style->aim;
    fire = &style->fire;
    hit = &style->hit;

    normal->draw_mode = def->draw_mode;
    normal->arm_mask = def->arm_mask;
    normal->dot_enabled = def->dot_enabled;
    normal->gap_fx = GC89_FX_FROM_INT(def->gap_px);
    normal->arm_length_fx = GC89_FX_FROM_INT(def->arm_length_px);
    normal->thickness_fx = GC89_FX_FROM_INT(def->thickness_px);
    normal->dot_size_fx = GC89_FX_FROM_INT(def->dot_size_px);
    normal->image_id = def->image_id;
    normal->image_width_fx = GC89_FX_FROM_INT(def->image_size_px);
    normal->image_height_fx = GC89_FX_FROM_INT(def->image_size_px);
    normal->color_rgba = def->color_rgba;
    normal->image_tint_rgba = def->color_rgba;
    normal->shape_type = shape_def->shape_type;
    normal->shape_segment_mask = shape_def->segment_mask;
    normal->shape_direction_mask = shape_def->direction_mask;
    normal->spread_mode = shape_def->spread_mode;
    normal->shape_radius_x_fx = GC89_FX_FROM_INT(shape_def->radius_x_px);
    normal->shape_radius_y_fx = GC89_FX_FROM_INT(shape_def->radius_y_px);
    normal->shape_depth_fx = GC89_FX_FROM_INT(shape_def->depth_px);
    normal->shape_rotation_deg_fx =
        GC89_FX_FROM_INT(shape_def->rotation_deg);
    normal->shape_break_fx = GC89_FX_FROM_INT(shape_def->break_px);
    if (normal->shape_type != GC89_SHAPE_CROSS) {
        normal->arm_mask = shape_def->direction_mask;
    }

    gcb89_copy_variant(aim, normal);
    gcb89_copy_variant(fire, normal);
    gcb89_copy_variant(hit, normal);

    if ((def->behavior & GCB89_BH_AIM) != 0) {
        aim->gap_fx = gcb89_sub_pixels_clamped(aim->gap_fx, 2);
        aim->arm_length_fx =
            gcb89_sub_pixels_clamped(aim->arm_length_fx, 2);
        if (aim->shape_type != GC89_SHAPE_CROSS) {
            aim->shape_radius_x_fx =
                gcb89_sub_pixels_clamped(aim->shape_radius_x_fx, 2);
            aim->shape_radius_y_fx =
                gcb89_sub_pixels_clamped(aim->shape_radius_y_fx, 2);
            aim->shape_depth_fx =
                gcb89_sub_pixels_clamped(aim->shape_depth_fx, 1);
        }
        if (aim->thickness_fx < GC89_FX_FROM_INT(2)) {
            aim->thickness_fx = GC89_FX_FROM_INT(2);
        }
        if (aim->dot_enabled != 0) {
            aim->dot_size_fx += GC89_FX_ONE;
        }
        if (aim->image_id != 0) {
            aim->image_width_fx =
                gcb89_sub_pixels_clamped(aim->image_width_fx, 6);
            aim->image_height_fx =
                gcb89_sub_pixels_clamped(aim->image_height_fx, 6);
        }
        if ((def->behavior & GCB89_BH_COLOR) != 0) {
            aim->color_rgba = GC89_RGBA(255, 235, 89, 255);
            aim->image_tint_rgba = aim->color_rgba;
        }
        style->use_aim_variant = 1;
    }

    if ((def->behavior & GCB89_BH_FIRE) != 0) {
        fire->gap_fx += GC89_FX_FROM_INT(4);
        fire->arm_length_fx += GC89_FX_FROM_INT(2);
        if (fire->shape_type != GC89_SHAPE_CROSS) {
            fire->shape_radius_x_fx += GC89_FX_FROM_INT(4);
            fire->shape_radius_y_fx += GC89_FX_FROM_INT(4);
            fire->shape_depth_fx += GC89_FX_FROM_INT(2);
        }
        if (fire->image_id != 0) {
            fire->image_width_fx += GC89_FX_FROM_INT(8);
            fire->image_height_fx += GC89_FX_FROM_INT(8);
        }
        if ((def->behavior & GCB89_BH_COLOR) != 0) {
            fire->color_rgba = GC89_RGBA(255, 150, 40, 255);
            fire->image_tint_rgba = fire->color_rgba;
        }
        style->use_fire_variant = 1;
    }

    if ((def->behavior & GCB89_BH_HIT) != 0) {
        hit->dot_enabled = 1;
        if (hit->dot_size_fx < GC89_FX_FROM_INT(3)) {
            hit->dot_size_fx = GC89_FX_FROM_INT(3);
        } else {
            hit->dot_size_fx += GC89_FX_ONE;
        }
        if (hit->shape_type != GC89_SHAPE_CROSS) {
            hit->shape_radius_x_fx += GC89_FX_FROM_INT(2);
            hit->shape_radius_y_fx += GC89_FX_FROM_INT(2);
        }
        if (hit->image_id != 0) {
            hit->image_width_fx += GC89_FX_FROM_INT(4);
            hit->image_height_fx += GC89_FX_FROM_INT(4);
        }
        if ((def->behavior & GCB89_BH_COLOR) != 0) {
            hit->color_rgba = GC89_RGBA(255, 64, 64, 255);
            hit->image_tint_rgba = hit->color_rgba;
        }
        style->use_hit_variant = 1;
    }

    style->color_change_enabled =
        (def->behavior & GCB89_BH_COLOR) != 0;
    if ((def->behavior & GCB89_BH_SPREAD) != 0) {
        style->spread_multiplier_fx =
            gcb89_percent_fx(def->spread_percent);
    } else {
        style->spread_multiplier_fx = 0;
    }
    style->center_offset_x_fx = 0;
    style->center_offset_y_fx = 0;
}

void gcb89_make_blank3d_original_style(GC89_Style *style)
{
    if (!style) return;
    memset(style, 0, sizeof(*style));
    gcb89_clear_variant(&style->normal);
    gcb89_clear_variant(&style->aim);
    gcb89_clear_variant(&style->fire);
    gcb89_clear_variant(&style->hit);

    style->normal.gap_fx = GC89_FX_FROM_INT(9);
    style->normal.arm_length_fx = GC89_FX_FROM_INT(13);
    style->normal.thickness_fx = GC89_FX_FROM_INT(1);
    style->normal.dot_enabled = 1;
    style->normal.dot_size_fx = GC89_FX_FROM_INT(3);
    style->normal.color_rgba = GC89_RGBA(209, 235, 255, 255);
    style->normal.image_tint_rgba = style->normal.color_rgba;
    style->normal.shape_type = GC89_SHAPE_CROSS;
    style->normal.shape_segment_mask = GC89_SEGMENT_ALL;
    style->normal.shape_direction_mask = GC89_DIRECTION_ALL;
    style->normal.spread_mode = GC89_SPREAD_GAP;

    style->aim.gap_fx = GC89_FX_FROM_INT(5);
    style->aim.arm_length_fx = GC89_FX_FROM_INT(9);
    style->aim.thickness_fx = GC89_FX_FROM_INT(2);
    style->aim.dot_enabled = 1;
    style->aim.dot_size_fx = GC89_FX_FROM_INT(4);
    style->aim.color_rgba = GC89_RGBA(255, 235, 89, 255);
    style->aim.image_tint_rgba = style->aim.color_rgba;
    style->aim.shape_type = GC89_SHAPE_CROSS;
    style->aim.shape_segment_mask = GC89_SEGMENT_ALL;
    style->aim.shape_direction_mask = GC89_DIRECTION_ALL;
    style->aim.spread_mode = GC89_SPREAD_GAP;

    gcb89_copy_variant(&style->fire, &style->normal);
    gcb89_copy_variant(&style->hit, &style->normal);
    style->use_aim_variant = 1;
    style->use_fire_variant = 0;
    style->use_hit_variant = 0;
    style->color_change_enabled = 1;
    style->spread_multiplier_fx = GC89_FX_ONE;
    style->center_offset_x_fx = 0;
    style->center_offset_y_fx = 0;
}

void gcb89_make_blank3d_animation(GCB89_AnimationPreset *preset)
{
    if (!preset) return;
    preset->micro_scale_fx = 57672L;
    preset->neutral_scale_fx = GC89_FX_ONE;
    preset->maxi_scale_fx = 88474L;
    preset->speed_fx_per_tick = 3932L;
}

int gcb89_preset_count(void)
{
    return GCB89_PRESET_COUNT;
}

int gcb89_preset_is_valid(int preset_id)
{
    return preset_id >= 0 && preset_id < GCB89_PRESET_COUNT;
}

const char *gcb89_preset_name(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return "invalid";
    return gcb89_presets[preset_id].name;
}

const char *gcb89_preset_category(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return "invalid";
    return gcb89_presets[preset_id].category;
}

int gcb89_preset_draw_mode(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return 0;
    return gcb89_presets[preset_id].draw_mode;
}

int gcb89_preset_asset_id(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return 0;
    return gcb89_presets[preset_id].image_id;
}

int gcb89_preset_shape_type(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return GC89_SHAPE_CROSS;
    return gcb89_shape_defs[preset_id].shape_type;
}

int gcb89_preset_spread_mode(int preset_id)
{
    if (!gcb89_preset_is_valid(preset_id)) return GC89_SPREAD_NONE;
    return gcb89_shape_defs[preset_id].spread_mode;
}

int gcb89_preset_requires_image(int preset_id)
{
    int mode;

    mode = gcb89_preset_draw_mode(preset_id);
    return (mode & GC89_DRAW_IMAGE) != 0;
}

int gcb89_make_preset(int preset_id, GC89_Style *style)
{
    if (!style || !gcb89_preset_is_valid(preset_id)) return 0;
    if (preset_id == GCB89_PRESET_BLANK3D_DEFAULT) {
        gcb89_make_blank3d_original_style(style);
        return 1;
    }
    gcb89_apply_definition(&gcb89_presets[preset_id],
                           &gcb89_shape_defs[preset_id],
                           style);
    return 1;
}

int gcb89_make_preset_animation(int preset_id,
                                GCB89_AnimationPreset *preset)
{
    int kind;

    if (!preset || !gcb89_preset_is_valid(preset_id)) return 0;
    if (preset_id == GCB89_PRESET_BLANK3D_DEFAULT) {
        gcb89_make_blank3d_animation(preset);
        return 1;
    }

    kind = gcb89_presets[preset_id].animation_kind;
    preset->neutral_scale_fx = GC89_FX_ONE;

    if (kind == 1) {
        preset->micro_scale_fx = gcb89_percent_fx(92);
        preset->maxi_scale_fx = gcb89_percent_fx(112);
        preset->speed_fx_per_tick = gcb89_percent_fx(4);
    } else if (kind == 2) {
        preset->micro_scale_fx = gcb89_percent_fx(85);
        preset->maxi_scale_fx = gcb89_percent_fx(145);
        preset->speed_fx_per_tick = gcb89_percent_fx(8);
    } else if (kind == 3) {
        preset->micro_scale_fx = gcb89_percent_fx(80);
        preset->maxi_scale_fx = gcb89_percent_fx(160);
        preset->speed_fx_per_tick = gcb89_percent_fx(10);
    } else if (kind == 4) {
        preset->micro_scale_fx = gcb89_percent_fx(90);
        preset->maxi_scale_fx = gcb89_percent_fx(125);
        preset->speed_fx_per_tick = gcb89_percent_fx(6);
    } else {
        preset->micro_scale_fx = GC89_FX_ONE;
        preset->maxi_scale_fx = GC89_FX_ONE;
        preset->speed_fx_per_tick = 0;
    }
    return 1;
}

const char *gcb89_asset_filename(int image_id)
{
    int i;

    for (i = 0; i < GCB89_ASSET_COUNT; ++i) {
        if (gcb89_assets[i].image_id == image_id) {
            return gcb89_assets[i].filename;
        }
    }
    return "";
}

GC89_Fixed gcb89_blank3d_recoil_to_spread_fx(long camera_recoil_x1000)
{
    int negative;
    unsigned long magnitude;
    unsigned long whole;
    unsigned long remainder;
    unsigned long out;

    negative = camera_recoil_x1000 < 0;
    if (negative) magnitude = (unsigned long)(-camera_recoil_x1000);
    else magnitude = (unsigned long)camera_recoil_x1000;

    /* recoil * 2.2 pixels, input expressed as recoil*1000. */
    whole = magnitude / 5000UL;
    remainder = magnitude % 5000UL;
    out = whole * 720896UL + (remainder * 720896UL) / 5000UL;
    if (negative) return -(GC89_Fixed)out;
    return (GC89_Fixed)out;
}

void gcb89_make_blank3d_input(GC89_InputState *input,
                              int aiming,
                              int firing,
                              int hit,
                              long camera_recoil_x1000)
{
    if (!input) return;
    input->flags = 0;
    if (aiming) input->flags |= GC89_STATE_AIM;
    if (firing) input->flags |= GC89_STATE_FIRE;
    if (hit) input->flags |= GC89_STATE_HIT;
    input->spread_fx =
        gcb89_blank3d_recoil_to_spread_fx(camera_recoil_x1000);
    if (input->spread_fx < 0) input->spread_fx = 0;
}
