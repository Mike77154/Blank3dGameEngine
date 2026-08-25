/*
 * gscopevector89.h
 * Vector reticle construction and emission.
 * C89, fixed-point only, no dynamic allocation.
 */
#ifndef GSCOPEVECTOR89_H
#define GSCOPEVECTOR89_H

#include "gscopepaint89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEVECTOR89_API
#define GSCOPEVECTOR89_API
#endif

#define GSV89_MAX_PARTS 32
#define GSV89_MAX_REGULAR_SIDES 32

#define GSV89_PART_PRIMARY       0
#define GSV89_PART_SECONDARY     1
#define GSV89_PART_CENTER        2
#define GSV89_PART_POSTS         3
#define GSV89_PART_TICKS         4
#define GSV89_PART_RANGE         5
#define GSV89_PART_BDC           6
#define GSV89_PART_WIND          7
#define GSV89_PART_LEAD          8
#define GSV89_PART_STADIA        9
#define GSV89_PART_LABELS        10
#define GSV89_PART_ILLUMINATION  11
#define GSV89_PART_FRAME         12
#define GSV89_PART_DECORATION    13
#define GSV89_PART_WARNING       14
#define GSV89_PART_CUSTOM0       16

#define GSV89_SHAPE_NONE             0
#define GSV89_SHAPE_LINE             1
#define GSV89_SHAPE_HLINE            2
#define GSV89_SHAPE_VLINE            3
#define GSV89_SHAPE_RECT             4
#define GSV89_SHAPE_SQUARE           5
#define GSV89_SHAPE_TRIANGLE         6
#define GSV89_SHAPE_CIRCLE           7
#define GSV89_SHAPE_ELLIPSE          8
#define GSV89_SHAPE_ARC              9
#define GSV89_SHAPE_DOT              10
#define GSV89_SHAPE_CROSS            11
#define GSV89_SHAPE_XCROSS           12
#define GSV89_SHAPE_CHEVRON          13
#define GSV89_SHAPE_DIAMOND          14
#define GSV89_SHAPE_PARENTHESIS      15
#define GSV89_SHAPE_BRACKET          16
#define GSV89_SHAPE_HORSESHOE        17
#define GSV89_SHAPE_REGULAR_POLYGON  18
#define GSV89_SHAPE_PATH             19
#define GSV89_SHAPE_GRID             20
#define GSV89_SHAPE_TICK_STRIP       21
#define GSV89_SHAPE_BEZIER_QUAD      22
#define GSV89_SHAPE_GLYPH            23
#define GSV89_SHAPE_SCOPE_MASK       24
#define GSV89_SHAPE_VIGNETTE         25

#define GSV89_DIR_UP       0
#define GSV89_DIR_DOWN     1
#define GSV89_DIR_LEFT     2
#define GSV89_DIR_RIGHT    3

#define GSV89_AXIS_HORIZONTAL 0
#define GSV89_AXIS_VERTICAL   1

#define GSV89_SIDE_LEFT  -1
#define GSV89_SIDE_RIGHT  1

#define GSV89_FLAG_FILLED       GSP89_FLAG_FILLED
#define GSV89_FLAG_CLOSED       GSP89_FLAG_CLOSED
#define GSV89_FLAG_OUTLINE      GSP89_FLAG_OUTLINE
#define GSV89_FLAG_VISIBLE      GSP89_FLAG_VISIBLE
#define GSV89_FLAG_MAJOR_ALT    64
#define GSV89_FLAG_FLIP         128

typedef struct gsv89_point {
    gsp89_fx x;
    gsp89_fx y;
} gsv89_point;

typedef struct gsv89_part_style {
    gsp89_color color;
    gsp89_color outline_color;
    short thickness_px;
    short outline_px;
    short layer;
    short blend_mode;
    short flags;
    short visible;
} gsv89_part_style;

typedef struct gsv89_palette {
    gsv89_part_style part[GSV89_MAX_PARTS];
} gsv89_palette;

typedef struct gsv89_shape {
    short kind;
    short part_id;
    short flags;
    short layer;
    short thickness_px;
    short outline_px;

    gsp89_fx x0;
    gsp89_fx y0;
    gsp89_fx x1;
    gsp89_fx y1;
    gsp89_fx x2;
    gsp89_fx y2;
    gsp89_fx x3;
    gsp89_fx y3;

    gsp89_fx a;
    gsp89_fx b;
    gsp89_fx c;
    gsp89_fx d;

    short i0;
    short i1;
    short i2;
    short i3;

    const gsv89_point *points;
    short point_count;
} gsv89_shape;

typedef struct gsv89_builder {
    gsv89_shape *items;
    short capacity;
    short count;
    short overflowed;
} gsv89_builder;

GSCOPEVECTOR89_API void gsv89_palette_init(gsv89_palette *palette);
GSCOPEVECTOR89_API void gsv89_palette_set_part(gsv89_palette *palette,
                                               short part_id,
                                               gsp89_color color,
                                               gsp89_color outline_color,
                                               short thickness_px,
                                               short outline_px,
                                               short layer,
                                               short blend_mode,
                                               short flags,
                                               short visible);
GSCOPEVECTOR89_API gsp89_style gsv89_style_for(const gsv89_palette *palette,
                                               const gsv89_shape *shape,
                                               short global_alpha);

GSCOPEVECTOR89_API gsv89_shape gsv89_shape_empty(void);
GSCOPEVECTOR89_API gsv89_shape gsv89_line(short part_id,
                                          gsp89_fx x0,
                                          gsp89_fx y0,
                                          gsp89_fx x1,
                                          gsp89_fx y1);
GSCOPEVECTOR89_API gsv89_shape gsv89_hline(short part_id,
                                           gsp89_fx x0,
                                           gsp89_fx x1,
                                           gsp89_fx y);
GSCOPEVECTOR89_API gsv89_shape gsv89_vline(short part_id,
                                           gsp89_fx x,
                                           gsp89_fx y0,
                                           gsp89_fx y1);
GSCOPEVECTOR89_API gsv89_shape gsv89_rect(short part_id,
                                          gsp89_fx x0,
                                          gsp89_fx y0,
                                          gsp89_fx x1,
                                          gsp89_fx y1,
                                          short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_square(short part_id,
                                            gsp89_fx cx,
                                            gsp89_fx cy,
                                            gsp89_fx half_size,
                                            short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_triangle(short part_id,
                                              gsp89_fx x0,
                                              gsp89_fx y0,
                                              gsp89_fx x1,
                                              gsp89_fx y1,
                                              gsp89_fx x2,
                                              gsp89_fx y2,
                                              short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_circle(short part_id,
                                            gsp89_fx cx,
                                            gsp89_fx cy,
                                            gsp89_fx radius,
                                            short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_ellipse(short part_id,
                                             gsp89_fx cx,
                                             gsp89_fx cy,
                                             gsp89_fx radius_x,
                                             gsp89_fx radius_y,
                                             short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_arc(short part_id,
                                         gsp89_fx cx,
                                         gsp89_fx cy,
                                         gsp89_fx radius_x,
                                         gsp89_fx radius_y,
                                         short start_deg_x100,
                                         short end_deg_x100);
GSCOPEVECTOR89_API gsv89_shape gsv89_dot(short part_id,
                                         gsp89_fx cx,
                                         gsp89_fx cy,
                                         gsp89_fx radius);
GSCOPEVECTOR89_API gsv89_shape gsv89_cross(short part_id,
                                           gsp89_fx cx,
                                           gsp89_fx cy,
                                           gsp89_fx half_w,
                                           gsp89_fx half_h,
                                           gsp89_fx gap);
GSCOPEVECTOR89_API gsv89_shape gsv89_xcross(short part_id,
                                            gsp89_fx cx,
                                            gsp89_fx cy,
                                            gsp89_fx half_w,
                                            gsp89_fx half_h,
                                            gsp89_fx gap);
GSCOPEVECTOR89_API gsv89_shape gsv89_chevron(short part_id,
                                             gsp89_fx cx,
                                             gsp89_fx cy,
                                             gsp89_fx half_w,
                                             gsp89_fx height,
                                             short direction,
                                             short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_diamond(short part_id,
                                             gsp89_fx cx,
                                             gsp89_fx cy,
                                             gsp89_fx half_w,
                                             gsp89_fx half_h,
                                             short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_parenthesis(short part_id,
                                                 gsp89_fx cx,
                                                 gsp89_fx cy,
                                                 gsp89_fx radius_x,
                                                 gsp89_fx radius_y,
                                                 short side);
GSCOPEVECTOR89_API gsv89_shape gsv89_bracket(short part_id,
                                             gsp89_fx cx,
                                             gsp89_fx cy,
                                             gsp89_fx half_h,
                                             gsp89_fx arm,
                                             short side);
GSCOPEVECTOR89_API gsv89_shape gsv89_horseshoe(short part_id,
                                               gsp89_fx cx,
                                               gsp89_fx cy,
                                               gsp89_fx radius_x,
                                               gsp89_fx radius_y,
                                               short open_deg_x100);
GSCOPEVECTOR89_API gsv89_shape gsv89_regular_polygon(short part_id,
                                                     gsp89_fx cx,
                                                     gsp89_fx cy,
                                                     gsp89_fx radius,
                                                     short sides,
                                                     short rotation_deg_x100,
                                                     short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_path(short part_id,
                                          const gsv89_point *points,
                                          short point_count,
                                          short closed,
                                          short filled);
GSCOPEVECTOR89_API gsv89_shape gsv89_grid(short part_id,
                                          gsp89_fx x0,
                                          gsp89_fx y0,
                                          gsp89_fx x1,
                                          gsp89_fx y1,
                                          short columns,
                                          short rows);
GSCOPEVECTOR89_API gsv89_shape gsv89_tick_strip(short part_id,
                                                gsp89_fx x0,
                                                gsp89_fx y0,
                                                gsp89_fx x1,
                                                gsp89_fx y1,
                                                gsp89_fx tick_half,
                                                short count,
                                                short major_every,
                                                short axis);
GSCOPEVECTOR89_API gsv89_shape gsv89_bezier_quad(short part_id,
                                                 gsp89_fx x0,
                                                 gsp89_fx y0,
                                                 gsp89_fx x1,
                                                 gsp89_fx y1,
                                                 gsp89_fx x2,
                                                 gsp89_fx y2,
                                                 short segments);
GSCOPEVECTOR89_API gsv89_shape gsv89_glyph(short part_id,
                                           short glyph_id,
                                           gsp89_fx x,
                                           gsp89_fx y,
                                           gsp89_fx size_x,
                                           gsp89_fx size_y);

GSCOPEVECTOR89_API void gsv89_builder_init(gsv89_builder *builder,
                                            gsv89_shape *storage,
                                            short capacity);
GSCOPEVECTOR89_API int gsv89_builder_push(gsv89_builder *builder,
                                          const gsv89_shape *shape);

GSCOPEVECTOR89_API void gsv89_emit_shape(gsp89_painter *painter,
                                         const gsv89_shape *shape,
                                         const gsv89_palette *palette,
                                         short global_alpha);
GSCOPEVECTOR89_API void gsv89_emit_shapes(gsp89_painter *painter,
                                          const gsv89_shape *shapes,
                                          short shape_count,
                                          const gsv89_palette *palette,
                                          short global_alpha);

#ifdef __cplusplus
}
#endif

#endif
