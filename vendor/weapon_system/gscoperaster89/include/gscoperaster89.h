/*
 * gscoperaster89.h
 * Raster image layer descriptors for scopes/HUDs.
 * C89, fixed-point only, no dynamic allocation.
 */
#ifndef GSCOPERASTER89_H
#define GSCOPERASTER89_H

#include "gscopepaint89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPERASTER89_API
#define GSCOPERASTER89_API
#endif

#define GSR89_FIT_STRETCH 0
#define GSR89_FIT_CONTAIN 1
#define GSR89_FIT_COVER   2
#define GSR89_FIT_NATIVE  3

#define GSR89_ANCHOR_CENTER       0
#define GSR89_ANCHOR_TOP_LEFT     1
#define GSR89_ANCHOR_TOP          2
#define GSR89_ANCHOR_TOP_RIGHT    3
#define GSR89_ANCHOR_LEFT         4
#define GSR89_ANCHOR_RIGHT        5
#define GSR89_ANCHOR_BOTTOM_LEFT  6
#define GSR89_ANCHOR_BOTTOM       7
#define GSR89_ANCHOR_BOTTOM_RIGHT 8

#define GSR89_LAYER_VISIBLE       1
#define GSR89_LAYER_CLIP_SCOPE    2
#define GSR89_LAYER_OUTLINE       4
#define GSR89_LAYER_FLIP_X        8
#define GSR89_LAYER_FLIP_Y        16

typedef struct gsr89_layer {
    short asset_id;
    short part_id;
    short layer;
    short blend_mode;
    short fit_mode;
    short anchor;
    short flags;

    gsp89_fx x;
    gsp89_fx y;
    gsp89_fx width;
    gsp89_fx height;
    gsp89_fx pivot_x;
    gsp89_fx pivot_y;
    gsp89_fx scale_x;
    gsp89_fx scale_y;

    short uv_x0;
    short uv_y0;
    short uv_x1;
    short uv_y1;

    gsp89_color tint;
    gsp89_color outline_color;
    short outline_px;
} gsr89_layer;

typedef struct gsr89_stack {
    const gsr89_layer *layers;
    short layer_count;
    short clip_to_scope;
    gsp89_fx clip_radius;
} gsr89_stack;

typedef struct gsr89_builder {
    gsr89_layer *layers;
    short capacity;
    short count;
    short overflowed;
} gsr89_builder;

GSCOPERASTER89_API gsr89_layer gsr89_layer_make(short asset_id,
                                                 short part_id,
                                                 short layer,
                                                 gsp89_fx x,
                                                 gsp89_fx y,
                                                 gsp89_fx width,
                                                 gsp89_fx height);
GSCOPERASTER89_API void gsr89_layer_set_uv(gsr89_layer *layer,
                                           short x0,
                                           short y0,
                                           short x1,
                                           short y1);
GSCOPERASTER89_API void gsr89_layer_set_tint(gsr89_layer *layer,
                                             gsp89_color tint,
                                             gsp89_color outline_color,
                                             short outline_px);
GSCOPERASTER89_API void gsr89_layer_set_transform(gsr89_layer *layer,
                                                  gsp89_fx pivot_x,
                                                  gsp89_fx pivot_y,
                                                  gsp89_fx scale_x,
                                                  gsp89_fx scale_y,
                                                  short anchor,
                                                  short fit_mode);
GSCOPERASTER89_API void gsr89_builder_init(gsr89_builder *builder,
                                            gsr89_layer *storage,
                                            short capacity);
GSCOPERASTER89_API int gsr89_builder_push(gsr89_builder *builder,
                                          const gsr89_layer *layer);
GSCOPERASTER89_API void gsr89_emit_layer(gsp89_painter *painter,
                                         const gsr89_layer *layer,
                                         short global_alpha);
GSCOPERASTER89_API void gsr89_emit_stack(gsp89_painter *painter,
                                         const gsr89_stack *stack,
                                         short global_alpha);

#ifdef __cplusplus
}
#endif

#endif
