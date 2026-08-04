/*
 * gscopepaint89.c
 * C89 fixed-point command painter. No dynamic allocation.
 */
#include "../include/gscopepaint89.h"

#define GSP89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static short gsp89_clamp_short(long v)
{
    if (v < -32768L) return (short)-32768;
    if (v > 32767L) return (short)32767;
    return (short)v;
}

static unsigned char gsp89_clamp_byte(short v)
{
    if (v < 0) return (unsigned char)0;
    if (v > 255) return (unsigned char)255;
    return (unsigned char)v;
}

static gsp89_draw_cmd gsp89_cmd_zero(void)
{
    gsp89_draw_cmd c;
    c.kind = GSP89_CMD_NONE;
    c.layer = 0;
    c.part_id = 0;
    c.blend_mode = GSP89_BLEND_ALPHA;
    c.flags = GSP89_FLAG_VISIBLE;
    c.x0 = 0;
    c.y0 = 0;
    c.x1 = 0;
    c.y1 = 0;
    c.x2 = 0;
    c.y2 = 0;
    c.x3 = 0;
    c.y3 = 0;
    c.radius_x = 0;
    c.radius_y = 0;
    c.start_deg_x100 = 0;
    c.end_deg_x100 = 0;
    c.thickness_px = 1;
    c.outline_px = 0;
    c.asset_id = -1;
    c.glyph_id = -1;
    c.uv_x0 = 0;
    c.uv_y0 = 0;
    c.uv_x1 = 10000;
    c.uv_y1 = 10000;
    c.color = gsp89_rgba(255, 255, 255, 255);
    c.outline_color = gsp89_rgba(0, 0, 0, 255);
    return c;
}

static void gsp89_apply_style(gsp89_draw_cmd *c,
                              const gsp89_painter *p,
                              const gsp89_style *style)
{
    long alpha;
    if (!c || !p || !style) return;
    c->layer = style->layer;
    c->part_id = style->part_id;
    c->blend_mode = style->blend_mode;
    c->flags = style->flags;
    c->thickness_px = style->thickness_px < 1 ? 1 : style->thickness_px;
    c->outline_px = style->outline_px < 0 ? 0 : style->outline_px;
    c->color = style->color;
    c->outline_color = style->outline_color;
    alpha = ((long)c->color.a * (long)p->global_alpha) / 255L;
    c->color.a = (unsigned char)GSP89_CLAMP(alpha, 0L, 255L);
    alpha = ((long)c->outline_color.a * (long)p->global_alpha) / 255L;
    c->outline_color.a = (unsigned char)GSP89_CLAMP(alpha, 0L, 255L);
}

gsp89_color gsp89_rgba(short r, short g, short b, short a)
{
    gsp89_color c;
    c.r = gsp89_clamp_byte(r);
    c.g = gsp89_clamp_byte(g);
    c.b = gsp89_clamp_byte(b);
    c.a = gsp89_clamp_byte(a);
    return c;
}

gsp89_style gsp89_style_make(gsp89_color color,
                              gsp89_color outline_color,
                              short thickness_px,
                              short outline_px,
                              short layer,
                              short part_id,
                              short blend_mode,
                              short flags)
{
    gsp89_style s;
    s.color = color;
    s.outline_color = outline_color;
    s.thickness_px = thickness_px < 1 ? 1 : thickness_px;
    s.outline_px = outline_px < 0 ? 0 : outline_px;
    s.layer = layer;
    s.part_id = part_id;
    s.blend_mode = blend_mode;
    s.flags = flags | GSP89_FLAG_VISIBLE;
    if (s.outline_px > 0) s.flags = (short)(s.flags | GSP89_FLAG_OUTLINE);
    return s;
}

gsp89_fx gsp89_fx_mul(gsp89_fx a, gsp89_fx b)
{
    return (gsp89_fx)((a / 256L) * (b / 256L));
}

gsp89_fx gsp89_fx_div(gsp89_fx a, gsp89_fx b)
{
    long d;
    if (b == 0) return 0;
    d = b / 256L;
    if (d == 0) d = b < 0 ? -1L : 1L;
    return (gsp89_fx)((a * 256L) / d);
}

void gsp89_painter_init(gsp89_painter *p,
                        short screen_w,
                        short screen_h,
                        gsp89_emit_cb emit_cb,
                        void *user)
{
    short m;
    if (!p) return;
    p->screen_w = screen_w;
    p->screen_h = screen_h;
    p->center_x = (short)(screen_w / 2);
    p->center_y = (short)(screen_h / 2);
    m = screen_w < screen_h ? screen_w : screen_h;
    p->radius_px = (short)(m / 2);
    p->offset_x_px = 0;
    p->offset_y_px = 0;
    p->global_alpha = 255;
    p->scale_q16 = GSP89_FX_ONE;
    p->emit_cb = emit_cb;
    p->user = user;
    p->emitted_count = 0;
    p->dropped_count = 0;
}

void gsp89_painter_set_view(gsp89_painter *p,
                            short center_x,
                            short center_y,
                            short radius_px,
                            gsp89_fx scale_q16,
                            short offset_x_px,
                            short offset_y_px,
                            short global_alpha)
{
    if (!p) return;
    p->center_x = center_x;
    p->center_y = center_y;
    p->radius_px = radius_px < 1 ? 1 : radius_px;
    p->scale_q16 = scale_q16 <= 0 ? GSP89_FX_ONE : scale_q16;
    p->offset_x_px = offset_x_px;
    p->offset_y_px = offset_y_px;
    p->global_alpha = (short)GSP89_CLAMP(global_alpha, 0, 255);
}

static long gsp89_norm_to_pixels(const gsp89_painter *p, gsp89_fx v)
{
    gsp89_fx scaled;
    long q;
    if (!p) return 0;
    scaled = gsp89_fx_mul(v, p->scale_q16);
    q = ((scaled / 256L) * (long)p->radius_px) / 256L;
    return q;
}

short gsp89_map_x(const gsp89_painter *p, gsp89_fx x)
{
    long v;
    if (!p) return 0;
    v = (long)p->center_x + (long)p->offset_x_px + gsp89_norm_to_pixels(p, x);
    return gsp89_clamp_short(v);
}

short gsp89_map_y(const gsp89_painter *p, gsp89_fx y)
{
    long v;
    if (!p) return 0;
    v = (long)p->center_y + (long)p->offset_y_px + gsp89_norm_to_pixels(p, y);
    return gsp89_clamp_short(v);
}

short gsp89_map_len(const gsp89_painter *p, gsp89_fx len)
{
    long v;
    if (!p) return 0;
    if (len < 0) len = -len;
    v = gsp89_norm_to_pixels(p, len);
    if (v < 0) v = -v;
    if (v < 1) v = 1;
    return gsp89_clamp_short(v);
}

void gsp89_emit_cmd(gsp89_painter *p, const gsp89_draw_cmd *cmd)
{
    if (!p || !cmd) return;
    if (!p->emit_cb) {
        p->dropped_count = (unsigned short)(p->dropped_count + 1U);
        return;
    }
    p->emit_cb(p->user, cmd);
    p->emitted_count = (unsigned short)(p->emitted_count + 1U);
}

void gsp89_paint_line(gsp89_painter *p,
                      gsp89_fx x0,
                      gsp89_fx y0,
                      gsp89_fx x1,
                      gsp89_fx y1,
                      const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_LINE;
    c.x0 = gsp89_map_x(p, x0);
    c.y0 = gsp89_map_y(p, y0);
    c.x1 = gsp89_map_x(p, x1);
    c.y1 = gsp89_map_y(p, y1);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_rect(gsp89_painter *p,
                      gsp89_fx x0,
                      gsp89_fx y0,
                      gsp89_fx x1,
                      gsp89_fx y1,
                      const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_RECT;
    c.x0 = gsp89_map_x(p, x0);
    c.y0 = gsp89_map_y(p, y0);
    c.x1 = gsp89_map_x(p, x1);
    c.y1 = gsp89_map_y(p, y1);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_triangle(gsp89_painter *p,
                          gsp89_fx x0,
                          gsp89_fx y0,
                          gsp89_fx x1,
                          gsp89_fx y1,
                          gsp89_fx x2,
                          gsp89_fx y2,
                          const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_TRIANGLE;
    c.x0 = gsp89_map_x(p, x0);
    c.y0 = gsp89_map_y(p, y0);
    c.x1 = gsp89_map_x(p, x1);
    c.y1 = gsp89_map_y(p, y1);
    c.x2 = gsp89_map_x(p, x2);
    c.y2 = gsp89_map_y(p, y2);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_circle(gsp89_painter *p,
                        gsp89_fx cx,
                        gsp89_fx cy,
                        gsp89_fx radius,
                        const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_CIRCLE;
    c.x0 = gsp89_map_x(p, cx);
    c.y0 = gsp89_map_y(p, cy);
    c.radius_x = gsp89_map_len(p, radius);
    c.radius_y = c.radius_x;
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_ellipse(gsp89_painter *p,
                         gsp89_fx cx,
                         gsp89_fx cy,
                         gsp89_fx radius_x,
                         gsp89_fx radius_y,
                         const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_ELLIPSE;
    c.x0 = gsp89_map_x(p, cx);
    c.y0 = gsp89_map_y(p, cy);
    c.radius_x = gsp89_map_len(p, radius_x);
    c.radius_y = gsp89_map_len(p, radius_y);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_arc(gsp89_painter *p,
                     gsp89_fx cx,
                     gsp89_fx cy,
                     gsp89_fx radius_x,
                     gsp89_fx radius_y,
                     short start_deg_x100,
                     short end_deg_x100,
                     const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_ARC;
    c.x0 = gsp89_map_x(p, cx);
    c.y0 = gsp89_map_y(p, cy);
    c.radius_x = gsp89_map_len(p, radius_x);
    c.radius_y = gsp89_map_len(p, radius_y);
    c.start_deg_x100 = start_deg_x100;
    c.end_deg_x100 = end_deg_x100;
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_sprite(gsp89_painter *p,
                        short asset_id,
                        gsp89_fx x0,
                        gsp89_fx y0,
                        gsp89_fx x1,
                        gsp89_fx y1,
                        short uv_x0,
                        short uv_y0,
                        short uv_x1,
                        short uv_y1,
                        const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_SPRITE;
    c.asset_id = asset_id;
    c.x0 = gsp89_map_x(p, x0);
    c.y0 = gsp89_map_y(p, y0);
    c.x1 = gsp89_map_x(p, x1);
    c.y1 = gsp89_map_y(p, y1);
    c.uv_x0 = uv_x0;
    c.uv_y0 = uv_y0;
    c.uv_x1 = uv_x1;
    c.uv_y1 = uv_y1;
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_glyph(gsp89_painter *p,
                       short glyph_id,
                       gsp89_fx x,
                       gsp89_fx y,
                       gsp89_fx size_x,
                       gsp89_fx size_y,
                       const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_GLYPH;
    c.glyph_id = glyph_id;
    c.x0 = gsp89_map_x(p, x);
    c.y0 = gsp89_map_y(p, y);
    c.radius_x = gsp89_map_len(p, size_x);
    c.radius_y = gsp89_map_len(p, size_y);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_clip_circle_begin(gsp89_painter *p,
                             gsp89_fx cx,
                             gsp89_fx cy,
                             gsp89_fx radius)
{
    gsp89_draw_cmd c;
    if (!p) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_CLIP_CIRCLE_BEGIN;
    c.x0 = gsp89_map_x(p, cx);
    c.y0 = gsp89_map_y(p, cy);
    c.radius_x = gsp89_map_len(p, radius);
    c.radius_y = c.radius_x;
    gsp89_emit_cmd(p, &c);
}

void gsp89_clip_rect_begin(gsp89_painter *p,
                           gsp89_fx x0,
                           gsp89_fx y0,
                           gsp89_fx x1,
                           gsp89_fx y1)
{
    gsp89_draw_cmd c;
    if (!p) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_CLIP_RECT_BEGIN;
    c.x0 = gsp89_map_x(p, x0);
    c.y0 = gsp89_map_y(p, y0);
    c.x1 = gsp89_map_x(p, x1);
    c.y1 = gsp89_map_y(p, y1);
    gsp89_emit_cmd(p, &c);
}

void gsp89_clip_end(gsp89_painter *p)
{
    gsp89_draw_cmd c;
    if (!p) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_CLIP_END;
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_scope_mask(gsp89_painter *p,
                            gsp89_fx cx,
                            gsp89_fx cy,
                            gsp89_fx radius,
                            const gsp89_style *style)
{
    gsp89_draw_cmd c;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_SCOPE_MASK;
    c.x0 = gsp89_map_x(p, cx);
    c.y0 = gsp89_map_y(p, cy);
    c.radius_x = gsp89_map_len(p, radius);
    c.radius_y = c.radius_x;
    c.x1 = p->screen_w;
    c.y1 = p->screen_h;
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}

void gsp89_paint_vignette(gsp89_painter *p,
                          gsp89_fx strength_q16,
                          const gsp89_style *style)
{
    gsp89_draw_cmd c;
    long strength;
    if (!p || !style) return;
    c = gsp89_cmd_zero();
    c.kind = GSP89_CMD_VIGNETTE;
    c.x0 = 0;
    c.y0 = 0;
    c.x1 = p->screen_w;
    c.y1 = p->screen_h;
    strength = (strength_q16 * 1000L) / GSP89_FX_ONE;
    c.x2 = gsp89_clamp_short(strength);
    gsp89_apply_style(&c, p, style);
    gsp89_emit_cmd(p, &c);
}
