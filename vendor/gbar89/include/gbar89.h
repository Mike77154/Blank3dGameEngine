#ifndef GBAR89_H
#define GBAR89_H

/*
    GBAR89 - General Bar / Meter Renderer for C89

    Constraints:
    - C89 compatible.
    - No malloc, no free, no realloc.
    - No float or double in the library API or implementation.
    - Fixed-point helpers only.
    - Backend-agnostic: the user provides drawing callbacks.

    v0.2 additions:
    - Fixed command buffer render backend.
    - Real layered/boss bar value mapping.
    - Numeric overlay/shield bar.
    - Visual states and automatic low/critical thresholds.
    - Accessible rect patterns.
    - Masked fills for slanted/hex/diamond/custom-slice bars.
    - Fixed-point easing helpers and integrated value tweening.

    v0.3 additions:
    - Composable 2D effects: outline, shadow, extrusion, gloss, inner shadow.
    - Multiple procedural frames and procedural backgrounds.
    - Pixel-quantized fills and pixel-cell overlays.
    - Independent middle/ghost value band.
    - User-owned normalized vector glyphs repeated as meter units.

    v0.4 additions:
    - Full radial parity: outlines, extrusion, bevel frames, backgrounds and fill FX.
    - Segmented/dashed radial arcs, round/square caps and animated phase.
    - Radial middle band, markers, vector units, grids, ticks and sweep highlights.
    - Vertical rendering parity fixes, including orientation-aware gloss and segments.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GBAR89_VERSION_MAJOR 0
#define GBAR89_VERSION_MINOR 4
#define GBAR89_VERSION_PATCH 0

#define GBAR89_FIX_SHIFT 16
#define GBAR89_FIX_ONE   (1L << GBAR89_FIX_SHIFT)
#define GBAR89_FIX_HALF  (1L << (GBAR89_FIX_SHIFT - 1))

#define GBAR89_TRIG_SHIFT 10
#define GBAR89_TRIG_ONE   (1 << GBAR89_TRIG_SHIFT)

#ifndef GBAR89_MAX_RADIAL_STEPS
#define GBAR89_MAX_RADIAL_STEPS 64
#endif

#ifndef GBAR89_MAX_MARKERS
#define GBAR89_MAX_MARKERS 16
#endif

#ifndef GBAR89_MAX_LAYERS
#define GBAR89_MAX_LAYERS 8
#endif

#ifndef GBAR89_MAX_MASK_SLICES
#define GBAR89_MAX_MASK_SLICES 16
#endif

#ifndef GBAR89_MAX_VECTOR_POINTS
#define GBAR89_MAX_VECTOR_POINTS 32
#endif

#ifndef GBAR89_MAX_COMMANDS
#define GBAR89_MAX_COMMANDS 1024
#endif

#ifndef GBAR89_MAX_COMMAND_VERTICES
#define GBAR89_MAX_COMMAND_VERTICES 8192
#endif

#ifndef GBAR89_MAX_COMMAND_INDICES
#define GBAR89_MAX_COMMAND_INDICES 32768
#endif

#define GBAR89_INVALID_SPRITE (-1)

#define GBAR89_FLAG_NONE                0x00000000L
#define GBAR89_FLAG_CLAMP_VALUE         0x00000001L
#define GBAR89_FLAG_DRAW_BG             0x00000002L
#define GBAR89_FLAG_DRAW_BORDER         0x00000004L
#define GBAR89_FLAG_DAMAGE_LAG          0x00000008L
#define GBAR89_FLAG_SMOOTH_VALUE        0x00000010L
#define GBAR89_FLAG_DRAW_MARKERS        0x00000020L
#define GBAR89_FLAG_DRAW_OVERLAY        0x00000040L
#define GBAR89_FLAG_USE_VISUAL          0x00000080L
#define GBAR89_FLAG_INVERT_RATIO        0x00000100L
#define GBAR89_FLAG_LAYERED             0x00000200L
#define GBAR89_FLAG_DRAW_VALUE_OVERLAY  0x00000400L
#define GBAR89_FLAG_DRAW_PATTERN        0x00000800L
#define GBAR89_FLAG_USE_MASK            0x00001000L
#define GBAR89_FLAG_AUTO_STATE          0x00002000L
#define GBAR89_FLAG_STATE_BLINK         0x00004000L
#define GBAR89_FLAG_EASE_VALUE          0x00008000L
#define GBAR89_FLAG_DRAW_LAYER_PIPS     0x00010000L
#define GBAR89_FLAG_DRAW_MID_VALUE      0x00020000L
#define GBAR89_FLAG_DRAW_VECTOR_UNITS   0x00040000L
#define GBAR89_FLAG_PIXEL_QUANTIZE      0x00080000L

typedef long GBar89_Fix;

typedef enum GBar89_Result {
    GBAR89_OK = 0,
    GBAR89_ERR_NULL = -1,
    GBAR89_ERR_BAD_RANGE = -2,
    GBAR89_ERR_UNSUPPORTED = -3,
    GBAR89_ERR_FULL = -4,
    GBAR89_ERR_OVERFLOW = -5
} GBar89_Result;

typedef enum GBar89_Kind {
    GBAR89_KIND_LINEAR = 0,
    GBAR89_KIND_SEGMENTED = 1,
    GBAR89_KIND_SPRITE_CLIP = 2,
    GBAR89_KIND_NINESLICE = 3,
    GBAR89_KIND_RADIAL_PIE = 4,
    GBAR89_KIND_RADIAL_RING = 5
} GBar89_Kind;

typedef enum GBar89_Direction {
    GBAR89_DIR_LEFT_TO_RIGHT = 0,
    GBAR89_DIR_RIGHT_TO_LEFT = 1,
    GBAR89_DIR_TOP_TO_BOTTOM = 2,
    GBAR89_DIR_BOTTOM_TO_TOP = 3,
    GBAR89_DIR_CENTER_HORIZONTAL = 4,
    GBAR89_DIR_CENTER_VERTICAL = 5
} GBar89_Direction;

typedef enum GBar89_State {
    GBAR89_STATE_NORMAL = 0,
    GBAR89_STATE_LOW = 1,
    GBAR89_STATE_CRITICAL = 2,
    GBAR89_STATE_POISONED = 3,
    GBAR89_STATE_REGENERATING = 4,
    GBAR89_STATE_SHIELDED = 5,
    GBAR89_STATE_OVERHEAT = 6,
    GBAR89_STATE_LOCKED = 7,
    GBAR89_STATE_BROKEN = 8,
    GBAR89_STATE_CUSTOM = 9,
    GBAR89_STATE_COUNT = 10
} GBar89_State;

typedef enum GBar89_PatternKind {
    GBAR89_PATTERN_NONE = 0,
    GBAR89_PATTERN_VERTICAL_STRIPES = 1,
    GBAR89_PATTERN_HORIZONTAL_STRIPES = 2,
    GBAR89_PATTERN_CROSSHATCH = 3,
    GBAR89_PATTERN_DOTS = 4,
    GBAR89_PATTERN_TICKS = 5
} GBar89_PatternKind;

typedef enum GBar89_MaskKind {
    GBAR89_MASK_NONE = 0,
    GBAR89_MASK_SLANT_RIGHT = 1,
    GBAR89_MASK_SLANT_LEFT = 2,
    GBAR89_MASK_HEXAGON = 3,
    GBAR89_MASK_DIAMOND = 4,
    GBAR89_MASK_CUSTOM_SLICES = 5
} GBar89_MaskKind;

typedef enum GBar89_FrameKind {
    GBAR89_FRAME_SIMPLE = 0,
    GBAR89_FRAME_DOUBLE = 1,
    GBAR89_FRAME_BEVEL_OUT = 2,
    GBAR89_FRAME_BEVEL_IN = 3,
    GBAR89_FRAME_BRACKETS = 4,
    GBAR89_FRAME_RAIL = 5,
    GBAR89_FRAME_PIXEL = 6,
    GBAR89_FRAME_NONE = 7
} GBar89_FrameKind;

typedef enum GBar89_BackgroundKind {
    GBAR89_BG_SOLID = 0,
    GBAR89_BG_GRID = 1,
    GBAR89_BG_CHECKER = 2,
    GBAR89_BG_SCANLINES = 3,
    GBAR89_BG_DIAGONAL = 4,
    GBAR89_BG_DITHER = 5
} GBar89_BackgroundKind;

typedef enum GBar89_FxFlag {
    GBAR89_FX_NONE = 0x0000,
    GBAR89_FX_OUTER_OUTLINE = 0x0001,
    GBAR89_FX_DROP_SHADOW = 0x0002,
    GBAR89_FX_EXTRUDE = 0x0004,
    GBAR89_FX_INNER_SHADOW = 0x0008,
    GBAR89_FX_GLOSS = 0x0010,
    GBAR89_FX_FILL_GRID = 0x0020,
    GBAR89_FX_FILL_SCANLINES = 0x0040,
    GBAR89_FX_PIXEL_CELLS = 0x0080,
    GBAR89_FX_RADIAL_SPOKES = 0x0100,
    GBAR89_FX_RADIAL_RINGS = 0x0200,
    GBAR89_FX_RADIAL_TICKS = 0x0400,
    GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT = 0x0800
} GBar89_FxFlag;

typedef enum GBar89_RadialCapKind {
    GBAR89_RADIAL_CAP_BUTT = 0,
    GBAR89_RADIAL_CAP_ROUND = 1,
    GBAR89_RADIAL_CAP_SQUARE = 2
} GBar89_RadialCapKind;

typedef enum GBar89_EaseKind {
    GBAR89_EASE_LINEAR = 0,
    GBAR89_EASE_IN_QUAD = 1,
    GBAR89_EASE_OUT_QUAD = 2,
    GBAR89_EASE_IN_OUT_QUAD = 3,
    GBAR89_EASE_SMOOTHSTEP = 4,
    GBAR89_EASE_OUT_CUBIC = 5
} GBar89_EaseKind;

typedef enum GBar89_CommandType {
    GBAR89_CMD_NONE = 0,
    GBAR89_CMD_RECT = 1,
    GBAR89_CMD_LINE = 2,
    GBAR89_CMD_SPRITE = 3,
    GBAR89_CMD_TRIANGLES = 4,
    GBAR89_CMD_PUSH_CLIP = 5,
    GBAR89_CMD_POP_CLIP = 6
} GBar89_CommandType;

typedef struct GBar89_Rect {
    int x;
    int y;
    int w;
    int h;
} GBar89_Rect;

typedef struct GBar89_VectorPoint {
    int x;
    int y;
} GBar89_VectorPoint;

typedef struct GBar89_Vertex {
    int x;
    int y;
    GBar89_Fix u;
    GBar89_Fix v;
    unsigned long color;
} GBar89_Vertex;

typedef struct GBar89_RenderOps {
    void *user;

    void (*draw_rect)(void *user, int x, int y, int w, int h,
                      unsigned long rgba);

    void (*draw_line)(void *user, int x1, int y1, int x2, int y2,
                      unsigned long rgba);

    void (*draw_sprite)(void *user, int sprite_id,
                        int sx, int sy, int sw, int sh,
                        int dx, int dy, int dw, int dh,
                        unsigned long tint_rgba);

    void (*draw_triangles)(void *user,
                           const GBar89_Vertex *vertices, int vertex_count,
                           const int *indices, int index_count,
                           int sprite_id);

    void (*push_clip)(void *user, int x, int y, int w, int h);
    void (*pop_clip)(void *user);
} GBar89_RenderOps;

typedef struct GBar89_Command {
    int type;
    int sprite_id;
    unsigned long color;
    GBar89_Rect src;
    GBar89_Rect dst;
    int x1;
    int y1;
    int x2;
    int y2;
    int vertex_first;
    int vertex_count;
    int index_first;
    int index_count;
} GBar89_Command;

typedef struct GBar89_CommandBuffer {
    GBar89_Command commands[GBAR89_MAX_COMMANDS];
    int command_count;

    GBar89_Vertex vertices[GBAR89_MAX_COMMAND_VERTICES];
    int vertex_count;

    int indices[GBAR89_MAX_COMMAND_INDICES];
    int index_count;

    int overflow;
} GBar89_CommandBuffer;

typedef struct GBar89_MaskSlice {
    int y_percent;
    int h_percent;
    int inset_left;
    int inset_right;
} GBar89_MaskSlice;

typedef struct GBar89_Style {
    unsigned long color_bg;
    unsigned long color_fill;
    unsigned long color_lag;
    unsigned long color_border;
    unsigned long color_overlay;
    unsigned long color_empty;
    unsigned long color_marker;
    unsigned long color_pattern;
    unsigned long color_mid;
    unsigned long color_outline;
    unsigned long color_highlight;
    unsigned long color_shadow;
    unsigned long color_extrude;
    unsigned long color_gloss;
    unsigned long color_bg_detail;
    unsigned long color_unit_fill;
    unsigned long color_unit_empty;
    unsigned long color_unit_outline;

    unsigned long color_state_fill[GBAR89_STATE_COUNT];
    unsigned long color_state_border[GBAR89_STATE_COUNT];
    unsigned long color_layer_fill[GBAR89_MAX_LAYERS];
    unsigned long color_layer_lag[GBAR89_MAX_LAYERS];

    int sprite_bg;
    int sprite_fill;
    int sprite_lag;
    int sprite_overlay;
    int sprite_empty;
    int sprite_full;
    int sprite_partial;

    GBar89_Rect src_bg;
    GBar89_Rect src_fill;
    GBar89_Rect src_lag;
    GBar89_Rect src_overlay;
    GBar89_Rect src_empty;
    GBar89_Rect src_full;
    GBar89_Rect src_partial;

    int margin_left;
    int margin_top;
    int margin_right;
    int margin_bottom;

    int padding_left;
    int padding_top;
    int padding_right;
    int padding_bottom;

    int border_size;
    int segment_gap;
    int marker_size;

    int pattern_kind;
    int pattern_step;
    int pattern_size;

    int frame_kind;
    int frame_depth;
    int frame_gap;
    int frame_corner;
    int outline_size;
    int shadow_offset_x;
    int shadow_offset_y;
    int extrude_depth;
    int bg_kind;
    int bg_step;
    int bg_size;
    int pixel_size;
    int gloss_percent;

    int radial_segments;
    int radial_gap_deg;
    int radial_cap_kind;
    int radial_detail_rings;
    int radial_unit_radius_percent;
    int radial_marker_length_percent;
    int radial_phase_deg;

    unsigned long fx_flags;
} GBar89_Style;

typedef struct GBar89_Meter {
    int kind;
    int direction;
    long flags;

    long min_value;
    long max_value;
    long value;
    long visual_value;
    long lag_value;

    long smooth_speed;
    long lag_speed;

    long overlay_min_value;
    long overlay_max_value;
    long overlay_value;
    long overlay_visual_value;
    long overlay_speed;
    int overlay_direction;

    long mid_value;
    long mid_visual_value;
    long mid_speed;
    int mid_direction;

    GBar89_Rect rect;
    GBar89_Style style;

    int segments;

    int radial_start_deg;
    int radial_sweep_deg;
    int radial_inner_percent;
    int radial_steps;

    int layer_count;
    long layer_size;

    int marker_count;
    long markers[GBAR89_MAX_MARKERS];

    int state;
    GBar89_Fix low_ratio;
    GBar89_Fix critical_ratio;
    GBar89_Fix state_timer;
    GBar89_Fix blink_period;

    int mask_kind;
    int mask_amount;
    int mask_steps;
    int mask_slice_count;
    GBar89_MaskSlice mask_slices[GBAR89_MAX_MASK_SLICES];

    int ease_kind;
    GBar89_Fix ease_duration;
    GBar89_Fix ease_elapsed;
    long ease_start_value;
    long ease_target_value;
    int ease_active;

    const GBar89_VectorPoint *unit_points;
    int unit_point_count;
    int unit_closed;
    int unit_filled;
    int unit_count;
    int unit_padding;
    int unit_scale_percent;
} GBar89_Meter;

/* Fixed-point helpers. */
GBar89_Fix gbar89_fix_from_int(int v);
int gbar89_fix_to_int(GBar89_Fix v);
GBar89_Fix gbar89_fix_mul(GBar89_Fix a, GBar89_Fix b);
GBar89_Fix gbar89_fix_div(GBar89_Fix a, GBar89_Fix b);
GBar89_Fix gbar89_fix_clamp01(GBar89_Fix v);
long gbar89_lerp_long(long a, long b, GBar89_Fix t);
GBar89_Fix gbar89_ease(int ease_kind, GBar89_Fix t);

/* Initialization and configuration. */
void gbar89_style_default(GBar89_Style *style);
void gbar89_init(GBar89_Meter *meter);
void gbar89_set_rect(GBar89_Meter *meter, int x, int y, int w, int h);
void gbar89_set_range(GBar89_Meter *meter, long min_value, long max_value);
void gbar89_set_value(GBar89_Meter *meter, long value);
void gbar89_set_kind(GBar89_Meter *meter, int kind);
void gbar89_set_direction(GBar89_Meter *meter, int direction);
void gbar89_set_segments(GBar89_Meter *meter, int segments, int gap);
void gbar89_set_speeds(GBar89_Meter *meter, long smooth_speed, long lag_speed);
void gbar89_set_radial(GBar89_Meter *meter, int start_deg, int sweep_deg,
                       int inner_percent, int steps);
void gbar89_set_radial_style(GBar89_Meter *meter, int segments,
                             int gap_deg, int cap_kind,
                             int detail_rings, int unit_radius_percent);
void gbar89_set_radial_phase(GBar89_Meter *meter, int phase_deg);
void gbar89_set_nineslice(GBar89_Meter *meter,
                          int left, int top, int right, int bottom);
int gbar89_add_marker(GBar89_Meter *meter, long value);

/* Command buffer backend: fixed storage, no heap. */
void gbar89_command_buffer_init(GBar89_CommandBuffer *buffer);
GBar89_RenderOps gbar89_command_buffer_make_ops(GBar89_CommandBuffer *buffer);
void gbar89_command_buffer_replay(const GBar89_CommandBuffer *buffer,
                                  const GBar89_RenderOps *ops);
int gbar89_command_buffer_overflowed(const GBar89_CommandBuffer *buffer);

/* Layers / boss bars. */
void gbar89_set_layers(GBar89_Meter *meter, int layer_count, long layer_size);
void gbar89_set_layer_color(GBar89_Meter *meter, int layer_index,
                            unsigned long fill_rgba,
                            unsigned long lag_rgba);
int gbar89_layer_index_from_value(const GBar89_Meter *meter, long value);
long gbar89_layer_value_from_value(const GBar89_Meter *meter, long value);
GBar89_Fix gbar89_layer_ratio_from_value(const GBar89_Meter *meter,
                                         long value);
int gbar89_layer_count_filled(const GBar89_Meter *meter, long value);

/* Numeric overlay / shield bar. */
void gbar89_set_overlay_range(GBar89_Meter *meter,
                              long min_value, long max_value);
void gbar89_set_overlay_value(GBar89_Meter *meter, long value);
void gbar89_set_overlay_direction(GBar89_Meter *meter, int direction);
GBar89_Fix gbar89_overlay_ratio(const GBar89_Meter *meter);

/* Independent middle / ghost value band. */
void gbar89_set_mid_value(GBar89_Meter *meter, long value);
void gbar89_set_mid_speed(GBar89_Meter *meter, long speed);
void gbar89_set_mid_direction(GBar89_Meter *meter, int direction);
GBar89_Fix gbar89_mid_ratio(const GBar89_Meter *meter);

/* Composable 2D styling operations. */
void gbar89_set_frame(GBar89_Meter *meter, int frame_kind,
                      int depth, int gap, int corner_length);
void gbar89_set_outline(GBar89_Meter *meter, int size,
                        unsigned long color_rgba);
void gbar89_set_background(GBar89_Meter *meter, int bg_kind,
                           int step, int size,
                           unsigned long detail_rgba);
void gbar89_set_fx_flags(GBar89_Meter *meter, unsigned long fx_flags);
void gbar89_set_pixel_size(GBar89_Meter *meter, int pixel_size);

/* User-owned normalized vector glyph (coordinates 0..1000). */
void gbar89_set_unit_vector(GBar89_Meter *meter,
                            const GBar89_VectorPoint *points,
                            int point_count, int closed, int filled,
                            int unit_count, int padding,
                            int scale_percent);

/* States. */
void gbar89_set_state(GBar89_Meter *meter, int state);
void gbar89_set_state_thresholds(GBar89_Meter *meter,
                                 GBar89_Fix low_ratio,
                                 GBar89_Fix critical_ratio);
void gbar89_set_state_color(GBar89_Meter *meter, int state,
                            unsigned long fill_rgba,
                            unsigned long border_rgba);
int gbar89_get_state(const GBar89_Meter *meter);

/* Patterns and masks. */
void gbar89_set_pattern(GBar89_Meter *meter, int pattern_kind,
                        int step, int size);
void gbar89_set_mask(GBar89_Meter *meter, int mask_kind,
                     int amount_px, int steps);
void gbar89_clear_mask_slices(GBar89_Meter *meter);
int gbar89_add_mask_slice(GBar89_Meter *meter, int y_percent,
                          int h_percent, int inset_left,
                          int inset_right);

/* Easing animation. */
void gbar89_set_easing(GBar89_Meter *meter, int ease_kind,
                       GBar89_Fix duration_seconds);

/* Evaluation and update. */
long gbar89_clamp_value(const GBar89_Meter *meter, long value);
GBar89_Fix gbar89_ratio_from_value(const GBar89_Meter *meter, long value);
GBar89_Fix gbar89_display_ratio_from_value(const GBar89_Meter *meter,
                                           long value);
GBar89_Fix gbar89_current_ratio(const GBar89_Meter *meter);
int gbar89_ratio_to_pixels(GBar89_Fix ratio, int pixels);
void gbar89_tick(GBar89_Meter *meter, GBar89_Fix dt_seconds);

/* Drawing. */
void gbar89_draw(const GBar89_Meter *meter, const GBar89_RenderOps *ops);

/* Small integer trigonometry for fixed radial generation. */
int gbar89_sin_deg(int deg);
int gbar89_cos_deg(int deg);

#ifdef __cplusplus
}
#endif

#endif
