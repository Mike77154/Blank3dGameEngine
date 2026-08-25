/*
 * gscoperaster89.c
 * Raster image layer emission. No image loading and no allocation.
 */
#include "../include/gscoperaster89.h"

#define GSR89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

gsr89_layer gsr89_layer_make(short asset_id,
                              short part_id,
                              short layer,
                              gsp89_fx x,
                              gsp89_fx y,
                              gsp89_fx width,
                              gsp89_fx height)
{
    gsr89_layer r;
    r.asset_id = asset_id;
    r.part_id = part_id;
    r.layer = layer;
    r.blend_mode = GSP89_BLEND_ALPHA;
    r.fit_mode = GSR89_FIT_STRETCH;
    r.anchor = GSR89_ANCHOR_CENTER;
    r.flags = GSR89_LAYER_VISIBLE;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    r.pivot_x = 0;
    r.pivot_y = 0;
    r.scale_x = GSP89_FX_ONE;
    r.scale_y = GSP89_FX_ONE;
    r.uv_x0 = 0;
    r.uv_y0 = 0;
    r.uv_x1 = 10000;
    r.uv_y1 = 10000;
    r.tint = gsp89_rgba(255, 255, 255, 255);
    r.outline_color = gsp89_rgba(0, 0, 0, 255);
    r.outline_px = 0;
    return r;
}

void gsr89_layer_set_uv(gsr89_layer *layer,
                        short x0,
                        short y0,
                        short x1,
                        short y1)
{
    if (!layer) return;
    layer->uv_x0 = (short)GSR89_CLAMP(x0, 0, 10000);
    layer->uv_y0 = (short)GSR89_CLAMP(y0, 0, 10000);
    layer->uv_x1 = (short)GSR89_CLAMP(x1, 0, 10000);
    layer->uv_y1 = (short)GSR89_CLAMP(y1, 0, 10000);
}

void gsr89_layer_set_tint(gsr89_layer *layer,
                          gsp89_color tint,
                          gsp89_color outline_color,
                          short outline_px)
{
    if (!layer) return;
    layer->tint = tint;
    layer->outline_color = outline_color;
    layer->outline_px = outline_px < 0 ? 0 : outline_px;
    if (layer->outline_px > 0) layer->flags = (short)(layer->flags | GSR89_LAYER_OUTLINE);
}

void gsr89_layer_set_transform(gsr89_layer *layer,
                               gsp89_fx pivot_x,
                               gsp89_fx pivot_y,
                               gsp89_fx scale_x,
                               gsp89_fx scale_y,
                               short anchor,
                               short fit_mode)
{
    if (!layer) return;
    layer->pivot_x = pivot_x;
    layer->pivot_y = pivot_y;
    layer->scale_x = scale_x <= 0 ? GSP89_FX_ONE : scale_x;
    layer->scale_y = scale_y <= 0 ? GSP89_FX_ONE : scale_y;
    layer->anchor = anchor;
    layer->fit_mode = fit_mode;
}

void gsr89_builder_init(gsr89_builder *builder, gsr89_layer *storage, short capacity)
{
    if (!builder) return;
    builder->layers = storage;
    builder->capacity = capacity < 0 ? 0 : capacity;
    builder->count = 0;
    builder->overflowed = 0;
}

int gsr89_builder_push(gsr89_builder *builder, const gsr89_layer *layer)
{
    if (!builder || !builder->layers || !layer) return 0;
    if (builder->count >= builder->capacity) {
        builder->overflowed = 1;
        return 0;
    }
    builder->layers[builder->count] = *layer;
    builder->count = (short)(builder->count + 1);
    return 1;
}

static void gsr89_anchor_adjust(const gsr89_layer *layer,
                                gsp89_fx width,
                                gsp89_fx height,
                                gsp89_fx *x0,
                                gsp89_fx *y0,
                                gsp89_fx *x1,
                                gsp89_fx *y1)
{
    gsp89_fx x;
    gsp89_fx y;
    x = layer->x;
    y = layer->y;
    switch (layer->anchor) {
        case GSR89_ANCHOR_TOP_LEFT:
            *x0 = x; *y0 = y; *x1 = x + width; *y1 = y + height;
            break;
        case GSR89_ANCHOR_TOP:
            *x0 = x - width / 2; *y0 = y; *x1 = x + width / 2; *y1 = y + height;
            break;
        case GSR89_ANCHOR_TOP_RIGHT:
            *x0 = x - width; *y0 = y; *x1 = x; *y1 = y + height;
            break;
        case GSR89_ANCHOR_LEFT:
            *x0 = x; *y0 = y - height / 2; *x1 = x + width; *y1 = y + height / 2;
            break;
        case GSR89_ANCHOR_RIGHT:
            *x0 = x - width; *y0 = y - height / 2; *x1 = x; *y1 = y + height / 2;
            break;
        case GSR89_ANCHOR_BOTTOM_LEFT:
            *x0 = x; *y0 = y - height; *x1 = x + width; *y1 = y;
            break;
        case GSR89_ANCHOR_BOTTOM:
            *x0 = x - width / 2; *y0 = y - height; *x1 = x + width / 2; *y1 = y;
            break;
        case GSR89_ANCHOR_BOTTOM_RIGHT:
            *x0 = x - width; *y0 = y - height; *x1 = x; *y1 = y;
            break;
        default:
            *x0 = x - width / 2; *y0 = y - height / 2;
            *x1 = x + width / 2; *y1 = y + height / 2;
            break;
    }
}

void gsr89_emit_layer(gsp89_painter *painter,
                      const gsr89_layer *layer,
                      short global_alpha)
{
    gsp89_fx width;
    gsp89_fx height;
    gsp89_fx x0;
    gsp89_fx y0;
    gsp89_fx x1;
    gsp89_fx y1;
    gsp89_color tint;
    long alpha;
    gsp89_style style;
    short uv_x0;
    short uv_y0;
    short uv_x1;
    short uv_y1;

    if (!painter || !layer) return;
    if (!(layer->flags & GSR89_LAYER_VISIBLE)) return;

    width = gsp89_fx_mul(layer->width, layer->scale_x);
    height = gsp89_fx_mul(layer->height, layer->scale_y);
    gsr89_anchor_adjust(layer, width, height, &x0, &y0, &x1, &y1);
    x0 = x0 - layer->pivot_x;
    x1 = x1 - layer->pivot_x;
    y0 = y0 - layer->pivot_y;
    y1 = y1 - layer->pivot_y;

    tint = layer->tint;
    global_alpha = (short)GSR89_CLAMP(global_alpha, 0, 255);
    alpha = ((long)tint.a * (long)global_alpha) / 255L;
    tint.a = (unsigned char)GSR89_CLAMP(alpha, 0L, 255L);

    style = gsp89_style_make(tint, layer->outline_color,
                             1, layer->outline_px,
                             layer->layer, layer->part_id,
                             layer->blend_mode,
                             layer->outline_px > 0 ? GSP89_FLAG_OUTLINE : 0);

    uv_x0 = layer->uv_x0;
    uv_y0 = layer->uv_y0;
    uv_x1 = layer->uv_x1;
    uv_y1 = layer->uv_y1;
    if (layer->flags & GSR89_LAYER_FLIP_X) {
        short t;
        t = uv_x0; uv_x0 = uv_x1; uv_x1 = t;
    }
    if (layer->flags & GSR89_LAYER_FLIP_Y) {
        short t2;
        t2 = uv_y0; uv_y0 = uv_y1; uv_y1 = t2;
    }

    if (layer->flags & GSR89_LAYER_CLIP_SCOPE) {
        gsp89_clip_circle_begin(painter, 0, 0, GSP89_FX_ONE);
    }
    gsp89_paint_sprite(painter, layer->asset_id,
                       x0, y0, x1, y1,
                       uv_x0, uv_y0, uv_x1, uv_y1,
                       &style);
    if (layer->flags & GSR89_LAYER_CLIP_SCOPE) {
        gsp89_clip_end(painter);
    }
}

void gsr89_emit_stack(gsp89_painter *painter,
                      const gsr89_stack *stack,
                      short global_alpha)
{
    short i;
    if (!painter || !stack || !stack->layers || stack->layer_count < 1) return;
    if (stack->clip_to_scope) {
        gsp89_clip_circle_begin(painter, 0, 0,
                                stack->clip_radius <= 0 ? GSP89_FX_ONE : stack->clip_radius);
    }
    for (i = 0; i < stack->layer_count; ++i) {
        gsr89_emit_layer(painter, &stack->layers[i], global_alpha);
    }
    if (stack->clip_to_scope) gsp89_clip_end(painter);
}
