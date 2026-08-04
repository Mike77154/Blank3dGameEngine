/*
 * gscopepaint89.h
 * Renderer-neutral command painter for scope/HUD systems.
 * C89, integer/fixed-point only, no dynamic allocation.
 */
#ifndef GSCOPEPAINT89_H
#define GSCOPEPAINT89_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEPAINT89_API
#define GSCOPEPAINT89_API
#endif

typedef long gsp89_fx;

#define GSP89_FX_ONE          ((gsp89_fx)65536L)
#define GSP89_FX_HALF         ((gsp89_fx)32768L)
#define GSP89_FX_FROM_INT(v)  ((gsp89_fx)((long)(v) * 65536L))
#define GSP89_FX_TO_INT(v)    ((short)((v) / 65536L))
#define GSP89_NORM(v)         ((gsp89_fx)(((long)(v) * 65536L) / 10000L))
#define GSP89_UV(v)           ((short)(v))

#define GSP89_FLAG_FILLED       1
#define GSP89_FLAG_CLOSED       2
#define GSP89_FLAG_CLIPPED      4
#define GSP89_FLAG_ANTIALIAS    8
#define GSP89_FLAG_VISIBLE      16
#define GSP89_FLAG_OUTLINE      32

#define GSP89_BLEND_ALPHA       0
#define GSP89_BLEND_ADD         1
#define GSP89_BLEND_MULTIPLY    2
#define GSP89_BLEND_SCREEN      3
#define GSP89_BLEND_REPLACE     4

#define GSP89_CMD_NONE              0
#define GSP89_CMD_LINE              1
#define GSP89_CMD_RECT              2
#define GSP89_CMD_TRIANGLE          3
#define GSP89_CMD_CIRCLE            4
#define GSP89_CMD_ELLIPSE           5
#define GSP89_CMD_ARC               6
#define GSP89_CMD_SPRITE            7
#define GSP89_CMD_GLYPH             8
#define GSP89_CMD_CLIP_CIRCLE_BEGIN 9
#define GSP89_CMD_CLIP_RECT_BEGIN   10
#define GSP89_CMD_CLIP_END          11
#define GSP89_CMD_VIGNETTE          12
#define GSP89_CMD_SCOPE_MASK        13
#define GSP89_CMD_DEBUG             14

typedef struct gsp89_color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} gsp89_color;

typedef struct gsp89_style {
    gsp89_color color;
    gsp89_color outline_color;
    short thickness_px;
    short outline_px;
    short layer;
    short part_id;
    short blend_mode;
    short flags;
} gsp89_style;

typedef struct gsp89_draw_cmd {
    short kind;
    short layer;
    short part_id;
    short blend_mode;
    short flags;

    short x0;
    short y0;
    short x1;
    short y1;
    short x2;
    short y2;
    short x3;
    short y3;

    short radius_x;
    short radius_y;
    short start_deg_x100;
    short end_deg_x100;

    short thickness_px;
    short outline_px;
    short asset_id;
    short glyph_id;

    short uv_x0;
    short uv_y0;
    short uv_x1;
    short uv_y1;

    gsp89_color color;
    gsp89_color outline_color;
} gsp89_draw_cmd;

typedef void (*gsp89_emit_cb)(void *user, const gsp89_draw_cmd *cmd);

typedef struct gsp89_painter {
    short screen_w;
    short screen_h;
    short center_x;
    short center_y;
    short radius_px;
    short offset_x_px;
    short offset_y_px;
    short global_alpha;
    gsp89_fx scale_q16;
    gsp89_emit_cb emit_cb;
    void *user;
    unsigned short emitted_count;
    unsigned short dropped_count;
} gsp89_painter;

GSCOPEPAINT89_API gsp89_color gsp89_rgba(short r, short g, short b, short a);
GSCOPEPAINT89_API gsp89_style gsp89_style_make(gsp89_color color,
                                               gsp89_color outline_color,
                                               short thickness_px,
                                               short outline_px,
                                               short layer,
                                               short part_id,
                                               short blend_mode,
                                               short flags);
GSCOPEPAINT89_API gsp89_fx gsp89_fx_mul(gsp89_fx a, gsp89_fx b);
GSCOPEPAINT89_API gsp89_fx gsp89_fx_div(gsp89_fx a, gsp89_fx b);

GSCOPEPAINT89_API void gsp89_painter_init(gsp89_painter *p,
                                          short screen_w,
                                          short screen_h,
                                          gsp89_emit_cb emit_cb,
                                          void *user);
GSCOPEPAINT89_API void gsp89_painter_set_view(gsp89_painter *p,
                                              short center_x,
                                              short center_y,
                                              short radius_px,
                                              gsp89_fx scale_q16,
                                              short offset_x_px,
                                              short offset_y_px,
                                              short global_alpha);
GSCOPEPAINT89_API short gsp89_map_x(const gsp89_painter *p, gsp89_fx x);
GSCOPEPAINT89_API short gsp89_map_y(const gsp89_painter *p, gsp89_fx y);
GSCOPEPAINT89_API short gsp89_map_len(const gsp89_painter *p, gsp89_fx len);
GSCOPEPAINT89_API void gsp89_emit_cmd(gsp89_painter *p, const gsp89_draw_cmd *cmd);

GSCOPEPAINT89_API void gsp89_paint_line(gsp89_painter *p,
                                        gsp89_fx x0,
                                        gsp89_fx y0,
                                        gsp89_fx x1,
                                        gsp89_fx y1,
                                        const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_rect(gsp89_painter *p,
                                        gsp89_fx x0,
                                        gsp89_fx y0,
                                        gsp89_fx x1,
                                        gsp89_fx y1,
                                        const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_triangle(gsp89_painter *p,
                                            gsp89_fx x0,
                                            gsp89_fx y0,
                                            gsp89_fx x1,
                                            gsp89_fx y1,
                                            gsp89_fx x2,
                                            gsp89_fx y2,
                                            const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_circle(gsp89_painter *p,
                                          gsp89_fx cx,
                                          gsp89_fx cy,
                                          gsp89_fx radius,
                                          const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_ellipse(gsp89_painter *p,
                                           gsp89_fx cx,
                                           gsp89_fx cy,
                                           gsp89_fx radius_x,
                                           gsp89_fx radius_y,
                                           const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_arc(gsp89_painter *p,
                                       gsp89_fx cx,
                                       gsp89_fx cy,
                                       gsp89_fx radius_x,
                                       gsp89_fx radius_y,
                                       short start_deg_x100,
                                       short end_deg_x100,
                                       const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_sprite(gsp89_painter *p,
                                          short asset_id,
                                          gsp89_fx x0,
                                          gsp89_fx y0,
                                          gsp89_fx x1,
                                          gsp89_fx y1,
                                          short uv_x0,
                                          short uv_y0,
                                          short uv_x1,
                                          short uv_y1,
                                          const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_glyph(gsp89_painter *p,
                                         short glyph_id,
                                         gsp89_fx x,
                                         gsp89_fx y,
                                         gsp89_fx size_x,
                                         gsp89_fx size_y,
                                         const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_clip_circle_begin(gsp89_painter *p,
                                               gsp89_fx cx,
                                               gsp89_fx cy,
                                               gsp89_fx radius);
GSCOPEPAINT89_API void gsp89_clip_rect_begin(gsp89_painter *p,
                                             gsp89_fx x0,
                                             gsp89_fx y0,
                                             gsp89_fx x1,
                                             gsp89_fx y1);
GSCOPEPAINT89_API void gsp89_clip_end(gsp89_painter *p);
GSCOPEPAINT89_API void gsp89_paint_scope_mask(gsp89_painter *p,
                                              gsp89_fx cx,
                                              gsp89_fx cy,
                                              gsp89_fx radius,
                                              const gsp89_style *style);
GSCOPEPAINT89_API void gsp89_paint_vignette(gsp89_painter *p,
                                            gsp89_fx strength_q16,
                                            const gsp89_style *style);

#ifdef __cplusplus
}
#endif

#endif
