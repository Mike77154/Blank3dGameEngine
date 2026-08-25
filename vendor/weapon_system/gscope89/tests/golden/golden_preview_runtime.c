/*
 * gscope_preview_runtime.c
 * Minimal C89 fixed-point runtime and PPM raster backend for gscopepresets89.
 * This preview utility uses the preset catalog through gsvp89_emit().
 * No dynamic allocation and no floating-point arithmetic.
 */
#include <stdio.h>
#include <string.h>
#include "gscopepresets89.h"

#define GPR89_W 320
#define GPR89_H 320
#define GPR89_PIXELS (GPR89_W * GPR89_H * 3)
#define GPR89_TRIG_ONE 1024

static unsigned char gpr89_pixels[GPR89_PIXELS];
static short gpr89_clip_cx = GPR89_W / 2;
static short gpr89_clip_cy = GPR89_H / 2;
static short gpr89_clip_r = 145;
static int gpr89_clip_enabled = 1;

static const short gpr89_sin90[91] = {
    0, 18, 36, 54, 71, 89, 107, 125, 143, 160, 178, 195, 213,
    230, 248, 265, 282, 299, 316, 333, 350, 367, 384, 400, 416, 433,
    449, 465, 481, 496, 512, 527, 543, 558, 573, 587, 602, 616, 630,
    644, 658, 672, 685, 698, 711, 724, 737, 749, 761, 773, 784, 796,
    807, 818, 828, 839, 849, 859, 868, 878, 887, 896, 904, 912, 920,
    928, 935, 943, 949, 956, 962, 968, 974, 979, 984, 989, 994, 998,
    1002, 1005, 1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024, 1024
};

static long gpr89_abs_long(long v) { return v < 0 ? -v : v; }
static short gpr89_clamp_short(long v)
{
    if (v < -32768L) return -32768;
    if (v > 32767L) return 32767;
    return (short)v;
}

static short gpr89_sin_deg(short deg)
{
    short d;
    short sign;
    d = deg;
    while (d < 0) d = (short)(d + 360);
    while (d >= 360) d = (short)(d - 360);
    sign = 1;
    if (d <= 90) return gpr89_sin90[d];
    if (d <= 180) return gpr89_sin90[180 - d];
    if (d <= 270) {
        sign = -1;
        return (short)(sign * gpr89_sin90[d - 180]);
    }
    sign = -1;
    return (short)(sign * gpr89_sin90[360 - d]);
}

static short gpr89_cos_deg(short deg)
{
    return gpr89_sin_deg((short)(deg + 90));
}

static int gpr89_inside_clip(short x, short y)
{
    long dx;
    long dy;
    if (!gpr89_clip_enabled) return 1;
    dx = (long)x - (long)gpr89_clip_cx;
    dy = (long)y - (long)gpr89_clip_cy;
    return dx * dx + dy * dy <= (long)gpr89_clip_r * (long)gpr89_clip_r;
}

static void gpr89_put(short x, short y, gsp89_color c)
{
    long idx;
    unsigned short a;
    unsigned short ia;
    unsigned short oldv;
    if (x < 0 || y < 0 || x >= GPR89_W || y >= GPR89_H) return;
    if (!gpr89_inside_clip(x, y)) return;
    idx = ((long)y * GPR89_W + x) * 3L;
    a = c.a;
    ia = (unsigned short)(255 - a);
    oldv = gpr89_pixels[idx + 0];
    gpr89_pixels[idx + 0] = (unsigned char)((oldv * ia + c.r * a) / 255);
    oldv = gpr89_pixels[idx + 1];
    gpr89_pixels[idx + 1] = (unsigned char)((oldv * ia + c.g * a) / 255);
    oldv = gpr89_pixels[idx + 2];
    gpr89_pixels[idx + 2] = (unsigned char)((oldv * ia + c.b * a) / 255);
}

static void gpr89_disc(short cx, short cy, short r, gsp89_color c)
{
    short x;
    short y;
    long rr;
    if (r < 1) r = 1;
    rr = (long)r * r;
    for (y = (short)-r; y <= r; ++y) {
        for (x = (short)-r; x <= r; ++x) {
            if ((long)x * x + (long)y * y <= rr) gpr89_put((short)(cx + x), (short)(cy + y), c);
        }
    }
}

static void gpr89_line_plain(short x0, short y0, short x1, short y1,
                             short thickness, gsp89_color c)
{
    long dx;
    long dy;
    long sx;
    long sy;
    long err;
    long e2;
    short rad;
    dx = gpr89_abs_long((long)x1 - x0);
    dy = -gpr89_abs_long((long)y1 - y0);
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    rad = (short)(thickness / 2);
    for (;;) {
        gpr89_disc(x0, y0, rad, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = err * 2;
        if (e2 >= dy) { err += dy; x0 = (short)(x0 + sx); }
        if (e2 <= dx) { err += dx; y0 = (short)(y0 + sy); }
    }
}

static void gpr89_line_style(short x0, short y0, short x1, short y1,
                             const gsp89_draw_cmd *cmd)
{
    short t;
    t = cmd->thickness_px < 1 ? 1 : cmd->thickness_px;
    if ((cmd->flags & GSP89_FLAG_OUTLINE) && cmd->outline_px > 0) {
        gpr89_line_plain(x0, y0, x1, y1,
                         (short)(t + cmd->outline_px * 2), cmd->outline_color);
    }
    gpr89_line_plain(x0, y0, x1, y1, t, cmd->color);
}

static void gpr89_rect_style(const gsp89_draw_cmd *cmd)
{
    short x0;
    short y0;
    short x1;
    short y1;
    short x;
    short y;
    x0 = cmd->x0 < cmd->x1 ? cmd->x0 : cmd->x1;
    x1 = cmd->x0 < cmd->x1 ? cmd->x1 : cmd->x0;
    y0 = cmd->y0 < cmd->y1 ? cmd->y0 : cmd->y1;
    y1 = cmd->y0 < cmd->y1 ? cmd->y1 : cmd->y0;
    if (cmd->flags & GSP89_FLAG_FILLED) {
        for (y = y0; y <= y1; ++y) for (x = x0; x <= x1; ++x) gpr89_put(x, y, cmd->color);
    } else {
        gpr89_line_style(x0, y0, x1, y0, cmd);
        gpr89_line_style(x1, y0, x1, y1, cmd);
        gpr89_line_style(x1, y1, x0, y1, cmd);
        gpr89_line_style(x0, y1, x0, y0, cmd);
    }
}

static long gpr89_edge(short ax, short ay, short bx, short by, short px, short py)
{
    return ((long)px - ax) * ((long)by - ay) - ((long)py - ay) * ((long)bx - ax);
}

static void gpr89_triangle_style(const gsp89_draw_cmd *cmd)
{
    short minx;
    short maxx;
    short miny;
    short maxy;
    short x;
    short y;
    long e0;
    long e1;
    long e2;
    if (!(cmd->flags & GSP89_FLAG_FILLED)) {
        gpr89_line_style(cmd->x0, cmd->y0, cmd->x1, cmd->y1, cmd);
        gpr89_line_style(cmd->x1, cmd->y1, cmd->x2, cmd->y2, cmd);
        gpr89_line_style(cmd->x2, cmd->y2, cmd->x0, cmd->y0, cmd);
        return;
    }
    minx = cmd->x0; if (cmd->x1 < minx) minx = cmd->x1; if (cmd->x2 < minx) minx = cmd->x2;
    maxx = cmd->x0; if (cmd->x1 > maxx) maxx = cmd->x1; if (cmd->x2 > maxx) maxx = cmd->x2;
    miny = cmd->y0; if (cmd->y1 < miny) miny = cmd->y1; if (cmd->y2 < miny) miny = cmd->y2;
    maxy = cmd->y0; if (cmd->y1 > maxy) maxy = cmd->y1; if (cmd->y2 > maxy) maxy = cmd->y2;
    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            e0 = gpr89_edge(cmd->x0, cmd->y0, cmd->x1, cmd->y1, x, y);
            e1 = gpr89_edge(cmd->x1, cmd->y1, cmd->x2, cmd->y2, x, y);
            e2 = gpr89_edge(cmd->x2, cmd->y2, cmd->x0, cmd->y0, x, y);
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0)) gpr89_put(x, y, cmd->color);
        }
    }
}

static void gpr89_ellipse_arc(const gsp89_draw_cmd *cmd, short start_deg, short end_deg)
{
    short deg;
    short prevx;
    short prevy;
    short x;
    short y;
    short step;
    short rx;
    short ry;
    rx = cmd->radius_x < 1 ? 1 : cmd->radius_x;
    ry = cmd->radius_y < 1 ? 1 : cmd->radius_y;
    step = 2;
    prevx = (short)(cmd->x0 + ((long)gpr89_cos_deg(start_deg) * rx) / GPR89_TRIG_ONE);
    prevy = (short)(cmd->y0 + ((long)gpr89_sin_deg(start_deg) * ry) / GPR89_TRIG_ONE);
    deg = (short)(start_deg + step);
    while (deg <= end_deg) {
        x = (short)(cmd->x0 + ((long)gpr89_cos_deg(deg) * rx) / GPR89_TRIG_ONE);
        y = (short)(cmd->y0 + ((long)gpr89_sin_deg(deg) * ry) / GPR89_TRIG_ONE);
        gpr89_line_style(prevx, prevy, x, y, cmd);
        prevx = x; prevy = y;
        deg = (short)(deg + step);
    }
}

static void gpr89_circle_style(const gsp89_draw_cmd *cmd)
{
    short x;
    short y;
    short rx;
    short ry;
    long dx;
    long dy;
    long lhs;
    long outer;
    long inner;
    short t;
    rx = cmd->radius_x < 1 ? 1 : cmd->radius_x;
    ry = cmd->radius_y < 1 ? 1 : cmd->radius_y;
    t = cmd->thickness_px < 1 ? 1 : cmd->thickness_px;
    if (cmd->flags & GSP89_FLAG_FILLED) {
        for (y = (short)-ry; y <= ry; ++y) {
            for (x = (short)-rx; x <= rx; ++x) {
                dx = x; dy = y;
                lhs = dx * dx * ry * ry + dy * dy * rx * rx;
                outer = (long)rx * rx * ry * ry;
                if (lhs <= outer) gpr89_put((short)(cmd->x0 + x), (short)(cmd->y0 + y), cmd->color);
            }
        }
        return;
    }
    outer = (long)rx * rx * ry * ry;
    inner = (long)(rx - t) * (rx - t) * (ry - t) * (ry - t);
    if (rx <= t || ry <= t) inner = 0;
    for (y = (short)-ry; y <= ry; ++y) {
        for (x = (short)-rx; x <= rx; ++x) {
            dx = x; dy = y;
            lhs = dx * dx * ry * ry + dy * dy * rx * rx;
            if (lhs <= outer && lhs >= inner) gpr89_put((short)(cmd->x0 + x), (short)(cmd->y0 + y), cmd->color);
        }
    }
}

static unsigned char gpr89_digit_segments(short glyph)
{
    static const unsigned char seg[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };
    if (glyph >= '0' && glyph <= '9') return seg[glyph - '0'];
    return 0;
}

static void gpr89_glyph_style(const gsp89_draw_cmd *cmd)
{
    unsigned char s;
    short w;
    short h;
    short x;
    short y;
    short t;
    s = gpr89_digit_segments(cmd->glyph_id);
    w = cmd->radius_x < 3 ? 3 : cmd->radius_x;
    h = cmd->radius_y < 5 ? 5 : cmd->radius_y;
    x = (short)(cmd->x0 - w / 2);
    y = (short)(cmd->y0 - h / 2);
    t = cmd->thickness_px < 1 ? 1 : cmd->thickness_px;
    if (s & 0x01) gpr89_line_plain(x, y, (short)(x + w), y, t, cmd->color);
    if (s & 0x02) gpr89_line_plain((short)(x + w), y, (short)(x + w), (short)(y + h / 2), t, cmd->color);
    if (s & 0x04) gpr89_line_plain((short)(x + w), (short)(y + h / 2), (short)(x + w), (short)(y + h), t, cmd->color);
    if (s & 0x08) gpr89_line_plain(x, (short)(y + h), (short)(x + w), (short)(y + h), t, cmd->color);
    if (s & 0x10) gpr89_line_plain(x, (short)(y + h / 2), x, (short)(y + h), t, cmd->color);
    if (s & 0x20) gpr89_line_plain(x, y, x, (short)(y + h / 2), t, cmd->color);
    if (s & 0x40) gpr89_line_plain(x, (short)(y + h / 2), (short)(x + w), (short)(y + h / 2), t, cmd->color);
}

static void gpr89_emit(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    if (!cmd) return;
    if (cmd->kind == GSP89_CMD_LINE) gpr89_line_style(cmd->x0, cmd->y0, cmd->x1, cmd->y1, cmd);
    else if (cmd->kind == GSP89_CMD_RECT) gpr89_rect_style(cmd);
    else if (cmd->kind == GSP89_CMD_TRIANGLE) gpr89_triangle_style(cmd);
    else if (cmd->kind == GSP89_CMD_CIRCLE || cmd->kind == GSP89_CMD_ELLIPSE) gpr89_circle_style(cmd);
    else if (cmd->kind == GSP89_CMD_ARC) gpr89_ellipse_arc(cmd, (short)(cmd->start_deg_x100 / 100), (short)(cmd->end_deg_x100 / 100));
    else if (cmd->kind == GSP89_CMD_GLYPH) gpr89_glyph_style(cmd);
}

gsp89_color gsp89_rgba(short r, short g, short b, short a)
{
    gsp89_color c;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    c.r = (unsigned char)r; c.g = (unsigned char)g; c.b = (unsigned char)b; c.a = (unsigned char)a;
    return c;
}

gsp89_style gsp89_style_make(gsp89_color color, gsp89_color outline_color,
                              short thickness_px, short outline_px, short layer,
                              short part_id, short blend_mode, short flags)
{
    gsp89_style s;
    s.color = color; s.outline_color = outline_color; s.thickness_px = thickness_px;
    s.outline_px = outline_px; s.layer = layer; s.part_id = part_id;
    s.blend_mode = blend_mode; s.flags = flags;
    return s;
}

gsp89_fx gsp89_fx_mul(gsp89_fx a, gsp89_fx b) { return (a / 256L) * (b / 256L); }
gsp89_fx gsp89_fx_div(gsp89_fx a, gsp89_fx b) { if (!b) return 0; return (a * 256L) / (b / 256L); }

void gsp89_painter_init(gsp89_painter *p, short screen_w, short screen_h,
                        gsp89_emit_cb emit_cb, void *user)
{
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->screen_w = screen_w; p->screen_h = screen_h;
    p->center_x = (short)(screen_w / 2); p->center_y = (short)(screen_h / 2);
    p->radius_px = (short)((screen_w < screen_h ? screen_w : screen_h) / 2 - 12);
    p->scale_q16 = GSP89_FX_ONE; p->global_alpha = 255;
    p->emit_cb = emit_cb; p->user = user;
}

void gsp89_painter_set_view(gsp89_painter *p, short center_x, short center_y,
                            short radius_px, gsp89_fx scale_q16,
                            short offset_x_px, short offset_y_px, short global_alpha)
{
    if (!p) return;
    p->center_x = center_x; p->center_y = center_y; p->radius_px = radius_px;
    p->scale_q16 = scale_q16; p->offset_x_px = offset_x_px; p->offset_y_px = offset_y_px;
    p->global_alpha = global_alpha;
}

short gsp89_map_x(const gsp89_painter *p, gsp89_fx x)
{
    gsp89_fx s;
    long px;
    s = gsp89_fx_mul(x, p->scale_q16);
    px = (long)p->center_x + p->offset_x_px + (s * p->radius_px) / GSP89_FX_ONE;
    return gpr89_clamp_short(px);
}

short gsp89_map_y(const gsp89_painter *p, gsp89_fx y)
{
    gsp89_fx s;
    long py;
    s = gsp89_fx_mul(y, p->scale_q16);
    py = (long)p->center_y + p->offset_y_px + (s * p->radius_px) / GSP89_FX_ONE;
    return gpr89_clamp_short(py);
}

short gsp89_map_len(const gsp89_painter *p, gsp89_fx len)
{
    gsp89_fx s;
    long px;
    s = gsp89_fx_mul(len, p->scale_q16);
    px = (gpr89_abs_long(s) * p->radius_px) / GSP89_FX_ONE;
    if (px < 1) px = 1;
    return gpr89_clamp_short(px);
}

void gsp89_emit_cmd(gsp89_painter *p, const gsp89_draw_cmd *cmd)
{
    if (!p || !cmd || !p->emit_cb) return;
    p->emit_cb(p->user, cmd);
    p->emitted_count = (unsigned short)(p->emitted_count + 1);
}

void gsv89_palette_init(gsv89_palette *palette)
{
    short i;
    if (!palette) return;
    for (i = 0; i < GSV89_MAX_PARTS; ++i) {
        palette->part[i].color = gsp89_rgba(0, 0, 0, 255);
        palette->part[i].outline_color = gsp89_rgba(255, 255, 255, 0);
        palette->part[i].thickness_px = 1; palette->part[i].outline_px = 0;
        palette->part[i].layer = 0; palette->part[i].blend_mode = GSP89_BLEND_ALPHA;
        palette->part[i].flags = 0; palette->part[i].visible = 1;
    }
}

void gsv89_palette_set_part(gsv89_palette *palette, short part_id,
                            gsp89_color color, gsp89_color outline_color,
                            short thickness_px, short outline_px, short layer,
                            short blend_mode, short flags, short visible)
{
    gsv89_part_style *s;
    if (!palette || part_id < 0 || part_id >= GSV89_MAX_PARTS) return;
    s = &palette->part[part_id];
    s->color = color; s->outline_color = outline_color;
    s->thickness_px = thickness_px; s->outline_px = outline_px; s->layer = layer;
    s->blend_mode = blend_mode; s->flags = flags; s->visible = visible;
}

gsp89_style gsv89_style_for(const gsv89_palette *palette,
                            const gsv89_shape *shape, short global_alpha)
{
    const gsv89_part_style *ps;
    gsp89_style s;
    short a;
    ps = &palette->part[shape->part_id >= 0 && shape->part_id < GSV89_MAX_PARTS ? shape->part_id : 0];
    s = gsp89_style_make(ps->color, ps->outline_color,
                         shape->thickness_px > 0 ? shape->thickness_px : ps->thickness_px,
                         shape->outline_px > 0 ? shape->outline_px : ps->outline_px,
                         shape->layer != 0 ? shape->layer : ps->layer,
                         shape->part_id, ps->blend_mode,
                         (short)(ps->flags | shape->flags));
    a = (short)(((short)s.color.a * global_alpha) / 255);
    s.color.a = (unsigned char)a;
    a = (short)(((short)s.outline_color.a * global_alpha) / 255);
    s.outline_color.a = (unsigned char)a;
    return s;
}

static void gpr89_cmd_init(gsp89_draw_cmd *cmd, short kind, const gsp89_style *s)
{
    memset(cmd, 0, sizeof(*cmd));
    cmd->kind = kind; cmd->layer = s->layer; cmd->part_id = s->part_id;
    cmd->blend_mode = s->blend_mode; cmd->flags = s->flags;
    cmd->thickness_px = s->thickness_px; cmd->outline_px = s->outline_px;
    cmd->color = s->color; cmd->outline_color = s->outline_color;
}

static void gpr89_emit_line_fx(gsp89_painter *p, const gsp89_style *s,
                               gsp89_fx x0, gsp89_fx y0, gsp89_fx x1, gsp89_fx y1)
{
    gsp89_draw_cmd cmd;
    gpr89_cmd_init(&cmd, GSP89_CMD_LINE, s);
    cmd.x0 = gsp89_map_x(p, x0); cmd.y0 = gsp89_map_y(p, y0);
    cmd.x1 = gsp89_map_x(p, x1); cmd.y1 = gsp89_map_y(p, y1);
    gsp89_emit_cmd(p, &cmd);
}

void gsv89_emit_shape(gsp89_painter *p, const gsv89_shape *sh,
                      const gsv89_palette *palette, short global_alpha)
{
    gsp89_style s;
    gsp89_draw_cmd cmd;
    short i;
    short n;
    short dir;
    gsp89_fx cx;
    gsp89_fx cy;
    gsp89_fx hw;
    gsp89_fx hh;
    gsp89_fx gap;
    gsp89_fx px;
    gsp89_fx py;
    gsp89_fx nx;
    gsp89_fx ny;
    gsp89_fx den;
    gsp89_fx t;
    gsp89_fx omt;
    if (!p || !sh || !palette) return;
    if (!(sh->flags & GSV89_FLAG_VISIBLE)) return;
    s = gsv89_style_for(palette, sh, global_alpha);
    if (!palette->part[sh->part_id].visible) return;
    if (sh->kind == GSV89_SHAPE_LINE || sh->kind == GSV89_SHAPE_HLINE || sh->kind == GSV89_SHAPE_VLINE) {
        gpr89_emit_line_fx(p, &s, sh->x0, sh->y0, sh->x1, sh->y1);
    } else if (sh->kind == GSV89_SHAPE_RECT || sh->kind == GSV89_SHAPE_SQUARE) {
        gpr89_cmd_init(&cmd, GSP89_CMD_RECT, &s);
        cmd.x0 = gsp89_map_x(p, sh->x0); cmd.y0 = gsp89_map_y(p, sh->y0);
        cmd.x1 = gsp89_map_x(p, sh->x1); cmd.y1 = gsp89_map_y(p, sh->y1);
        gsp89_emit_cmd(p, &cmd);
    } else if (sh->kind == GSV89_SHAPE_TRIANGLE) {
        gpr89_cmd_init(&cmd, GSP89_CMD_TRIANGLE, &s);
        cmd.x0 = gsp89_map_x(p, sh->x0); cmd.y0 = gsp89_map_y(p, sh->y0);
        cmd.x1 = gsp89_map_x(p, sh->x1); cmd.y1 = gsp89_map_y(p, sh->y1);
        cmd.x2 = gsp89_map_x(p, sh->x2); cmd.y2 = gsp89_map_y(p, sh->y2);
        gsp89_emit_cmd(p, &cmd);
    } else if (sh->kind == GSV89_SHAPE_CIRCLE || sh->kind == GSV89_SHAPE_DOT) {
        gpr89_cmd_init(&cmd, GSP89_CMD_CIRCLE, &s);
        cmd.x0 = gsp89_map_x(p, sh->x0); cmd.y0 = gsp89_map_y(p, sh->y0);
        cmd.radius_x = gsp89_map_len(p, sh->a); cmd.radius_y = cmd.radius_x;
        if (sh->kind == GSV89_SHAPE_DOT) cmd.flags = (short)(cmd.flags | GSP89_FLAG_FILLED);
        gsp89_emit_cmd(p, &cmd);
    } else if (sh->kind == GSV89_SHAPE_CROSS) {
        cx = sh->x0; cy = sh->y0; hw = sh->a; hh = sh->b; gap = sh->c;
        gpr89_emit_line_fx(p, &s, cx - hw, cy, cx - gap, cy);
        gpr89_emit_line_fx(p, &s, cx + gap, cy, cx + hw, cy);
        gpr89_emit_line_fx(p, &s, cx, cy - hh, cx, cy - gap);
        gpr89_emit_line_fx(p, &s, cx, cy + gap, cx, cy + hh);
    } else if (sh->kind == GSV89_SHAPE_CHEVRON) {
        cx = sh->x0; cy = sh->y0; hw = sh->a; hh = sh->b; dir = sh->i0;
        if (dir == GSV89_DIR_UP) {
            gpr89_emit_line_fx(p, &s, cx - hw, cy + hh / 2, cx, cy - hh / 2);
            gpr89_emit_line_fx(p, &s, cx, cy - hh / 2, cx + hw, cy + hh / 2);
        } else if (dir == GSV89_DIR_DOWN) {
            gpr89_emit_line_fx(p, &s, cx - hw, cy - hh / 2, cx, cy + hh / 2);
            gpr89_emit_line_fx(p, &s, cx, cy + hh / 2, cx + hw, cy - hh / 2);
        } else if (dir == GSV89_DIR_LEFT) {
            gpr89_emit_line_fx(p, &s, cx + hh / 2, cy - hw, cx - hh / 2, cy);
            gpr89_emit_line_fx(p, &s, cx - hh / 2, cy, cx + hh / 2, cy + hw);
        } else {
            gpr89_emit_line_fx(p, &s, cx - hh / 2, cy - hw, cx + hh / 2, cy);
            gpr89_emit_line_fx(p, &s, cx + hh / 2, cy, cx - hh / 2, cy + hw);
        }
    } else if (sh->kind == GSV89_SHAPE_DIAMOND) {
        cx = sh->x0; cy = sh->y0; hw = sh->a; hh = sh->b;
        gpr89_emit_line_fx(p, &s, cx, cy - hh, cx + hw, cy);
        gpr89_emit_line_fx(p, &s, cx + hw, cy, cx, cy + hh);
        gpr89_emit_line_fx(p, &s, cx, cy + hh, cx - hw, cy);
        gpr89_emit_line_fx(p, &s, cx - hw, cy, cx, cy - hh);
    } else if (sh->kind == GSV89_SHAPE_BRACKET) {
        cx = sh->x0; cy = sh->y0; hh = sh->a; hw = sh->b;
        gpr89_emit_line_fx(p, &s, cx, cy - hh, cx, cy + hh);
        if (sh->i0 < 0) {
            gpr89_emit_line_fx(p, &s, cx, cy - hh, cx + hw, cy - hh);
            gpr89_emit_line_fx(p, &s, cx, cy + hh, cx + hw, cy + hh);
        } else {
            gpr89_emit_line_fx(p, &s, cx, cy - hh, cx - hw, cy - hh);
            gpr89_emit_line_fx(p, &s, cx, cy + hh, cx - hw, cy + hh);
        }
    } else if (sh->kind == GSV89_SHAPE_PARENTHESIS) {
        short deg;
        short prevdeg;
        gsp89_fx ex0;
        gsp89_fx ey0;
        gsp89_fx ex1;
        gsp89_fx ey1;
        cx = sh->x0; cy = sh->y0; hw = sh->a; hh = sh->b;
        prevdeg = -90;
        ex0 = cx + (sh->i0 < 0 ? -1 : 1) * (gsp89_fx)(((long)gpr89_cos_deg(prevdeg) * hw) / GPR89_TRIG_ONE);
        ey0 = cy + (gsp89_fx)(((long)gpr89_sin_deg(prevdeg) * hh) / GPR89_TRIG_ONE);
        for (deg = -84; deg <= 90; deg = (short)(deg + 6)) {
            ex1 = cx + (sh->i0 < 0 ? -1 : 1) * (gsp89_fx)(((long)gpr89_cos_deg(deg) * hw) / GPR89_TRIG_ONE);
            ey1 = cy + (gsp89_fx)(((long)gpr89_sin_deg(deg) * hh) / GPR89_TRIG_ONE);
            gpr89_emit_line_fx(p, &s, ex0, ey0, ex1, ey1);
            ex0 = ex1; ey0 = ey1;
        }
    } else if (sh->kind == GSV89_SHAPE_HORSESHOE) {
        short deg;
        short openhalf;
        gsp89_fx ex0;
        gsp89_fx ey0;
        gsp89_fx ex1;
        gsp89_fx ey1;
        cx = sh->x0; cy = sh->y0; hw = sh->a; hh = sh->b;
        openhalf = (short)(sh->i0 / 200);
        ex0 = cx + (gsp89_fx)(((long)gpr89_cos_deg((short)(90 + openhalf)) * hw) / GPR89_TRIG_ONE);
        ey0 = cy + (gsp89_fx)(((long)gpr89_sin_deg((short)(90 + openhalf)) * hh) / GPR89_TRIG_ONE);
        for (deg = (short)(96 + openhalf); deg <= (short)(450 - openhalf); deg = (short)(deg + 6)) {
            ex1 = cx + (gsp89_fx)(((long)gpr89_cos_deg(deg) * hw) / GPR89_TRIG_ONE);
            ey1 = cy + (gsp89_fx)(((long)gpr89_sin_deg(deg) * hh) / GPR89_TRIG_ONE);
            gpr89_emit_line_fx(p, &s, ex0, ey0, ex1, ey1);
            ex0 = ex1; ey0 = ey1;
        }
    } else if (sh->kind == GSV89_SHAPE_GRID) {
        n = sh->i0;
        if (n < 1) n = 1;
        for (i = 0; i <= n; ++i) {
            px = sh->x0 + (gsp89_fx)(((sh->x1 - sh->x0) / n) * i);
            gpr89_emit_line_fx(p, &s, px, sh->y0, px, sh->y1);
        }
        n = sh->i1;
        if (n < 1) n = 1;
        for (i = 0; i <= n; ++i) {
            py = sh->y0 + (gsp89_fx)(((sh->y1 - sh->y0) / n) * i);
            gpr89_emit_line_fx(p, &s, sh->x0, py, sh->x1, py);
        }
    } else if (sh->kind == GSV89_SHAPE_TICK_STRIP) {
        n = sh->i0; if (n < 1) n = 1;
        for (i = 0; i < n; ++i) {
            den = n > 1 ? (gsp89_fx)(n - 1) : 1;
            px = sh->x0 + (gsp89_fx)(((sh->x1 - sh->x0) / den) * i);
            py = sh->y0 + (gsp89_fx)(((sh->y1 - sh->y0) / den) * i);
            hh = sh->a;
            if (sh->i1 > 0 && (i % sh->i1) == 0) hh = hh * 2;
            if (sh->i2 == GSV89_AXIS_HORIZONTAL) gpr89_emit_line_fx(p, &s, px, py - hh, px, py + hh);
            else gpr89_emit_line_fx(p, &s, px - hh, py, px + hh, py);
        }
    } else if (sh->kind == GSV89_SHAPE_BEZIER_QUAD) {
        n = sh->i0; if (n < 2) n = 2;
        px = sh->x0; py = sh->y0;
        for (i = 1; i <= n; ++i) {
            t = (gsp89_fx)(((long)i * GSP89_FX_ONE) / n);
            omt = GSP89_FX_ONE - t;
            nx = gsp89_fx_mul(gsp89_fx_mul(omt, omt), sh->x0) +
                 2 * gsp89_fx_mul(gsp89_fx_mul(omt, t), sh->x1) +
                 gsp89_fx_mul(gsp89_fx_mul(t, t), sh->x2);
            ny = gsp89_fx_mul(gsp89_fx_mul(omt, omt), sh->y0) +
                 2 * gsp89_fx_mul(gsp89_fx_mul(omt, t), sh->y1) +
                 gsp89_fx_mul(gsp89_fx_mul(t, t), sh->y2);
            gpr89_emit_line_fx(p, &s, px, py, nx, ny);
            px = nx; py = ny;
        }
    } else if (sh->kind == GSV89_SHAPE_GLYPH) {
        gpr89_cmd_init(&cmd, GSP89_CMD_GLYPH, &s);
        cmd.x0 = gsp89_map_x(p, sh->x0); cmd.y0 = gsp89_map_y(p, sh->y0);
        cmd.radius_x = gsp89_map_len(p, sh->a); cmd.radius_y = gsp89_map_len(p, sh->b);
        cmd.glyph_id = sh->i0;
        gsp89_emit_cmd(p, &cmd);
    }
}

void gsv89_emit_shapes(gsp89_painter *p, const gsv89_shape *shapes,
                       short shape_count, const gsv89_palette *palette,
                       short global_alpha)
{
    short i;
    if (!p || !shapes || !palette) return;
    for (i = 0; i < shape_count; ++i) gsv89_emit_shape(p, &shapes[i], palette, global_alpha);
}

static void gpr89_clear(void)
{
    short x;
    short y;
    long idx;
    long dx;
    long dy;
    long rr;
    gpr89_clip_enabled = 0;
    rr = (long)gpr89_clip_r * gpr89_clip_r;
    for (y = 0; y < GPR89_H; ++y) {
        for (x = 0; x < GPR89_W; ++x) {
            idx = ((long)y * GPR89_W + x) * 3L;
            dx = (long)x - gpr89_clip_cx; dy = (long)y - gpr89_clip_cy;
            if (dx * dx + dy * dy <= rr) {
                gpr89_pixels[idx + 0] = (unsigned char)(214 + (y * 18) / GPR89_H);
                gpr89_pixels[idx + 1] = (unsigned char)(221 + (y * 16) / GPR89_H);
                gpr89_pixels[idx + 2] = (unsigned char)(225 + (y * 12) / GPR89_H);
            } else {
                gpr89_pixels[idx + 0] = 18; gpr89_pixels[idx + 1] = 21; gpr89_pixels[idx + 2] = 24;
            }
        }
    }
    gpr89_clip_enabled = 1;
}

static void gpr89_scope_ring(void)
{
    short deg;
    short x0;
    short y0;
    short x1;
    short y1;
    gsp89_color c;
    c = gsp89_rgba(5, 7, 9, 255);
    gpr89_clip_enabled = 0;
    x0 = (short)(gpr89_clip_cx + ((long)gpr89_cos_deg(0) * gpr89_clip_r) / GPR89_TRIG_ONE);
    y0 = gpr89_clip_cy;
    for (deg = 2; deg <= 360; deg = (short)(deg + 2)) {
        x1 = (short)(gpr89_clip_cx + ((long)gpr89_cos_deg(deg) * gpr89_clip_r) / GPR89_TRIG_ONE);
        y1 = (short)(gpr89_clip_cy + ((long)gpr89_sin_deg(deg) * gpr89_clip_r) / GPR89_TRIG_ONE);
        gpr89_line_plain(x0, y0, x1, y1, 5, c);
        x0 = x1; y0 = y1;
    }
    gpr89_clip_enabled = 1;
}

int gpr89_render_preset(const gsvp89_preset *preset, const char *filename)
{
    FILE *f;
    gsp89_painter painter;
    gsv89_palette palette;
    if (!preset || !filename) return 0;
    gpr89_clear();
    gsp89_painter_init(&painter, GPR89_W, GPR89_H, gpr89_emit, 0);
    gsp89_painter_set_view(&painter, GPR89_W / 2, GPR89_H / 2, 134,
                           GSP89_FX_ONE, 0, 0, 255);
    gsvp89_default_palette(preset, &palette);
    gsvp89_emit(&painter, preset, &palette, 255);
    gpr89_scope_ring();
    f = fopen(filename, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n%d %d\n255\n", GPR89_W, GPR89_H);
    fwrite(gpr89_pixels, 1, GPR89_PIXELS, f);
    fclose(f);
    return 1;
}
