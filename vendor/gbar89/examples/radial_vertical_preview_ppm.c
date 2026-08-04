#include "gbar89.h"

#include <stdio.h>
#include <string.h>

#define PREVIEW_W 1280
#define PREVIEW_H 820
#define ANIM_W 960
#define ANIM_H 540
#define CLIP_STACK_MAX 16

static unsigned char g_pixels[PREVIEW_W * PREVIEW_H * 3];
static int g_w = PREVIEW_W;
static int g_h = PREVIEW_H;
static GBar89_Rect g_clip_stack[CLIP_STACK_MAX];
static int g_clip_count = 0;

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static GBar89_Rect rect_intersect(GBar89_Rect a, GBar89_Rect b)
{
    GBar89_Rect r;
    int x2;
    int y2;
    int bx2;
    int by2;

    r.x = (a.x > b.x) ? a.x : b.x;
    r.y = (a.y > b.y) ? a.y : b.y;
    x2 = a.x + a.w;
    y2 = a.y + a.h;
    bx2 = b.x + b.w;
    by2 = b.y + b.h;
    if (bx2 < x2) x2 = bx2;
    if (by2 < y2) y2 = by2;
    r.w = x2 - r.x;
    r.h = y2 - r.y;
    if (r.w < 0) r.w = 0;
    if (r.h < 0) r.h = 0;
    return r;
}

static GBar89_Rect current_clip(void)
{
    GBar89_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = g_w;
    r.h = g_h;
    if (g_clip_count > 0) {
        r = g_clip_stack[g_clip_count - 1];
    }
    return r;
}

static void clear_rgb(unsigned long rgb)
{
    int x;
    int y;
    int i;
    unsigned char r;
    unsigned char g;
    unsigned char b;

    r = (unsigned char)((rgb >> 16) & 255UL);
    g = (unsigned char)((rgb >> 8) & 255UL);
    b = (unsigned char)(rgb & 255UL);
    for (y = 0; y < g_h; ++y) {
        for (x = 0; x < g_w; ++x) {
            i = (y * g_w + x) * 3;
            g_pixels[i + 0] = r;
            g_pixels[i + 1] = g;
            g_pixels[i + 2] = b;
        }
    }
}

static void put_pixel(int x, int y, unsigned long rgba)
{
    int i;
    int a;
    int ia;
    int r;
    int g;
    int b;

    if (x < 0 || y < 0 || x >= g_w || y >= g_h) return;
    if (g_clip_count > 0) {
        GBar89_Rect c;
        c = g_clip_stack[g_clip_count - 1];
        if (x < c.x || y < c.y || x >= c.x + c.w || y >= c.y + c.h) return;
    }

    r = (int)((rgba >> 24) & 255UL);
    g = (int)((rgba >> 16) & 255UL);
    b = (int)((rgba >> 8) & 255UL);
    a = (int)(rgba & 255UL);
    i = (y * g_w + x) * 3;
    if (a >= 255) {
        g_pixels[i + 0] = (unsigned char)r;
        g_pixels[i + 1] = (unsigned char)g;
        g_pixels[i + 2] = (unsigned char)b;
    } else if (a > 0) {
        ia = 255 - a;
        g_pixels[i + 0] = (unsigned char)((r * a + g_pixels[i + 0] * ia) / 255);
        g_pixels[i + 1] = (unsigned char)((g * a + g_pixels[i + 1] * ia) / 255);
        g_pixels[i + 2] = (unsigned char)((b * a + g_pixels[i + 2] * ia) / 255);
    }
}

static void sw_rect(void *user, int x, int y, int w, int h,
                    unsigned long rgba)
{
    GBar89_Rect r;
    GBar89_Rect c;
    int xx;
    int yy;

    (void)user;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    c = current_clip();
    r = rect_intersect(r, c);
    for (yy = r.y; yy < r.y + r.h; ++yy) {
        for (xx = r.x; xx < r.x + r.w; ++xx) {
            put_pixel(xx, yy, rgba);
        }
    }
}

static void sw_line(void *user, int x0, int y0, int x1, int y1,
                    unsigned long rgba)
{
    int dx;
    int sx;
    int dy;
    int sy;
    int err;
    int e2;

    (void)user;
    dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    sx = (x0 < x1) ? 1 : -1;
    dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    sy = (y0 < y1) ? 1 : -1;
    err = dx + dy;
    for (;;) {
        put_pixel(x0, y0, rgba);
        if (x0 == x1 && y0 == y1) break;
        e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static long edge_value(int ax, int ay, int bx, int by, int px, int py)
{
    return (long)(px - ax) * (long)(by - ay) -
           (long)(py - ay) * (long)(bx - ax);
}

static void triangle_one(const GBar89_Vertex *a,
                         const GBar89_Vertex *b,
                         const GBar89_Vertex *c)
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int x;
    int y;
    long e0;
    long e1;
    long e2;
    long area;
    unsigned long color;

    minx = a->x;
    if (b->x < minx) minx = b->x;
    if (c->x < minx) minx = c->x;
    maxx = a->x;
    if (b->x > maxx) maxx = b->x;
    if (c->x > maxx) maxx = c->x;
    miny = a->y;
    if (b->y < miny) miny = b->y;
    if (c->y < miny) miny = c->y;
    maxy = a->y;
    if (b->y > maxy) maxy = b->y;
    if (c->y > maxy) maxy = c->y;
    minx = clamp_i(minx, 0, g_w - 1);
    maxx = clamp_i(maxx, 0, g_w - 1);
    miny = clamp_i(miny, 0, g_h - 1);
    maxy = clamp_i(maxy, 0, g_h - 1);

    area = edge_value(a->x, a->y, b->x, b->y, c->x, c->y);
    if (area == 0) return;
    color = a->color;
    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            e0 = edge_value(a->x, a->y, b->x, b->y, x, y);
            e1 = edge_value(b->x, b->y, c->x, c->y, x, y);
            e2 = edge_value(c->x, c->y, a->x, a->y, x, y);
            if ((area > 0 && e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (area < 0 && e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                put_pixel(x, y, color);
            }
        }
    }
}

static void sw_triangles(void *user,
                         const GBar89_Vertex *vertices, int vertex_count,
                         const int *indices, int index_count,
                         int sprite_id)
{
    int i;
    int ia;
    int ib;
    int ic;

    (void)user;
    (void)sprite_id;
    for (i = 0; i + 2 < index_count; i += 3) {
        ia = indices[i + 0];
        ib = indices[i + 1];
        ic = indices[i + 2];
        if (ia >= 0 && ib >= 0 && ic >= 0 &&
            ia < vertex_count && ib < vertex_count && ic < vertex_count) {
            triangle_one(&vertices[ia], &vertices[ib], &vertices[ic]);
        }
    }
}

static void sw_sprite(void *user, int sprite_id,
                      int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh,
                      unsigned long tint_rgba)
{
    (void)user;
    (void)sprite_id;
    (void)sx;
    (void)sy;
    (void)sw;
    (void)sh;
    sw_rect(0, dx, dy, dw, dh, tint_rgba);
}

static void sw_push_clip(void *user, int x, int y, int w, int h)
{
    GBar89_Rect r;
    GBar89_Rect c;

    (void)user;
    if (g_clip_count >= CLIP_STACK_MAX) return;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    c = current_clip();
    g_clip_stack[g_clip_count] = rect_intersect(r, c);
    g_clip_count += 1;
}

static void sw_pop_clip(void *user)
{
    (void)user;
    if (g_clip_count > 0) g_clip_count -= 1;
}

static GBar89_RenderOps make_ops(void)
{
    GBar89_RenderOps ops;
    ops.user = 0;
    ops.draw_rect = sw_rect;
    ops.draw_line = sw_line;
    ops.draw_sprite = sw_sprite;
    ops.draw_triangles = sw_triangles;
    ops.push_clip = sw_push_clip;
    ops.pop_clip = sw_pop_clip;
    return ops;
}

static void panel(int x, int y, int w, int h)
{
    sw_rect(0, x + 4, y + 5, w, h, 0x00000080UL);
    sw_rect(0, x, y, w, h, 0x101827E8UL);
    sw_rect(0, x, y, w, 1, 0x6F8DAD80UL);
    sw_rect(0, x, y + h - 1, w, 1, 0x05080FFFUL);
}

static void scene_grid(void)
{
    int x;
    int y;
    for (x = 0; x < g_w; x += 32) {
        sw_rect(0, x, 0, 1, g_h, 0x29405A38UL);
    }
    for (y = 0; y < g_h; y += 32) {
        sw_rect(0, 0, y, g_w, 1, 0x29405A38UL);
    }
    for (y = 0; y < g_h; y += 4) {
        sw_rect(0, 0, y, g_w, 1, 0x00000018UL);
    }
}

static const GBar89_VectorPoint diamond_points[4] = {
    {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
};

static const GBar89_VectorPoint chevron_points[6] = {
    {0, 180}, {520, 180}, {1000, 500},
    {520, 820}, {0, 820}, {480, 500}
};

static void common_radial(GBar89_Meter *m, int x, int y, int size,
                          int kind, int start, int sweep, int inner_pct,
                          long value)
{
    gbar89_init(m);
    gbar89_set_kind(m, kind);
    gbar89_set_rect(m, x, y, size, size);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, value);
    m->visual_value = value;
    m->lag_value = value + 12;
    if (m->lag_value > 100) m->lag_value = 100;
    gbar89_set_mid_value(m, value + 7);
    if (m->mid_value > 100) m->mid_value = 100;
    m->mid_visual_value = m->mid_value;
    gbar89_set_radial(m, start, sweep, inner_pct, 64);
    m->flags |= GBAR89_FLAG_DAMAGE_LAG |
                GBAR89_FLAG_DRAW_MID_VALUE |
                GBAR89_FLAG_DRAW_PATTERN |
                GBAR89_FLAG_DRAW_MARKERS;
    m->style.padding_left = 10;
    m->style.padding_right = 10;
    m->style.padding_top = 10;
    m->style.padding_bottom = 10;
    m->style.color_bg = 0x091726FFUL;
    m->style.color_empty = 0x102435FFUL;
    m->style.color_lag = 0xFF6B75FFUL;
    m->style.color_mid = 0xFFD65AFFUL;
    m->style.color_border = 0xD9F8FFFFUL;
    m->style.color_outline = 0x01060DFFUL;
    m->style.color_shadow = 0x020916D0UL;
    m->style.color_extrude = 0x031322FFUL;
    m->style.color_highlight = 0xF2FFFFFFUL;
    m->style.color_gloss = 0xFFFFFF42UL;
    m->style.color_pattern = 0x052D3AA0UL;
    m->style.color_bg_detail = 0x42B9D050UL;
    m->style.color_marker = 0xFFFFFFFFUL;
    m->style.shadow_offset_x = 4;
    m->style.shadow_offset_y = 5;
    m->style.extrude_depth = 5;
    gbar89_set_outline(m, 2, 0x01060DFFUL);
    gbar89_add_marker(m, 25);
    gbar89_add_marker(m, 50);
    gbar89_add_marker(m, 75);
}

static void setup_holo_ring(GBar89_Meter *m, int x, int y, int size)
{
    common_radial(m, x, y, size, GBAR89_KIND_RADIAL_RING,
                  -90, 320, 62, 68);
    m->style.color_fill = 0x27E9C7FFUL;
    m->style.color_unit_fill = 0xE8FFFFFFUL;
    m->style.color_unit_empty = 0x244A55FFUL;
    m->style.color_unit_outline = 0x021016FFUL;
    gbar89_set_frame(m, GBAR89_FRAME_DOUBLE, 3, 2, 14);
    gbar89_set_background(m, GBAR89_BG_GRID, 24, 1, 0x42B9D050UL);
    gbar89_set_pattern(m, GBAR89_PATTERN_CROSSHATCH, 18, 1);
    gbar89_set_radial_style(m, 1, 0, GBAR89_RADIAL_CAP_ROUND, 3, 82);
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_EXTRUDE |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_INNER_SHADOW |
                            GBAR89_FX_RADIAL_SPOKES |
                            GBAR89_FX_RADIAL_RINGS |
                            GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT);
    gbar89_set_unit_vector(m, diamond_points, 4, 1, 1, 8, 1, 72);
    m->flags |= GBAR89_FLAG_DRAW_VECTOR_UNITS;
}

static void setup_segment_ring(GBar89_Meter *m, int x, int y, int size)
{
    common_radial(m, x, y, size, GBAR89_KIND_RADIAL_RING,
                  -130, 280, 54, 61);
    m->style.color_fill = 0x7C71FFFFUL;
    m->style.color_mid = 0x4DE7FFFFUL;
    m->style.color_bg_detail = 0x8B7DFF42UL;
    m->style.color_pattern = 0x241B5DB0UL;
    gbar89_set_frame(m, GBAR89_FRAME_RAIL, 3, 1, 18);
    gbar89_set_background(m, GBAR89_BG_CHECKER, 15, 1, 0x8B7DFF42UL);
    gbar89_set_radial_style(m, 18, 4, GBAR89_RADIAL_CAP_SQUARE, 4, 78);
    gbar89_set_pixel_size(m, 8);
    m->flags |= GBAR89_FLAG_PIXEL_QUANTIZE;
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_PIXEL_CELLS |
                            GBAR89_FX_RADIAL_TICKS |
                            GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT);
}

static void setup_radar_pie(GBar89_Meter *m, int x, int y, int size)
{
    common_radial(m, x, y, size, GBAR89_KIND_RADIAL_PIE,
                  -210, 240, 0, 46);
    m->style.color_fill = 0xFF5D9DFFUL;
    m->style.color_mid = 0xFFB65AFFUL;
    m->style.color_bg_detail = 0xFF65A442UL;
    m->style.color_pattern = 0x59162FA0UL;
    gbar89_set_frame(m, GBAR89_FRAME_BEVEL_IN, 4, 1, 12);
    gbar89_set_background(m, GBAR89_BG_DIAGONAL, 20, 1, 0xFF65A442UL);
    gbar89_set_pattern(m, GBAR89_PATTERN_DOTS, 16, 2);
    gbar89_set_radial_style(m, 1, 0, GBAR89_RADIAL_CAP_BUTT, 4, 70);
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_EXTRUDE |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_FILL_GRID |
                            GBAR89_FX_RADIAL_RINGS |
                            GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT);
}

static void setup_pixel_ring(GBar89_Meter *m, int x, int y, int size)
{
    common_radial(m, x, y, size, GBAR89_KIND_RADIAL_RING,
                  -90, 360, 70, 79);
    m->style.color_fill = 0x9DE14EFFUL;
    m->style.color_mid = 0xE6FF63FFUL;
    m->style.color_bg_detail = 0x8DD34E42UL;
    m->style.color_pattern = 0x294218B0UL;
    gbar89_set_frame(m, GBAR89_FRAME_PIXEL, 4, 1, 20);
    gbar89_set_background(m, GBAR89_BG_DITHER, 12, 2, 0x8DD34E42UL);
    gbar89_set_radial_style(m, 24, 3, GBAR89_RADIAL_CAP_BUTT, 2, 84);
    gbar89_set_pixel_size(m, 12);
    m->flags |= GBAR89_FLAG_PIXEL_QUANTIZE;
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_EXTRUDE |
                            GBAR89_FX_FILL_SCANLINES |
                            GBAR89_FX_PIXEL_CELLS |
                            GBAR89_FX_INNER_SHADOW |
                            GBAR89_FX_RADIAL_TICKS);
}

static void setup_vertical(GBar89_Meter *m, int x, int y, int w, int h,
                           int direction, int segmented, unsigned long fill)
{
    gbar89_init(m);
    if (segmented) gbar89_set_kind(m, GBAR89_KIND_SEGMENTED);
    gbar89_set_rect(m, x, y, w, h);
    gbar89_set_direction(m, direction);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, 64);
    m->visual_value = 64;
    m->lag_value = 82;
    gbar89_set_mid_value(m, 74);
    m->mid_visual_value = 74;
    m->flags |= GBAR89_FLAG_DAMAGE_LAG |
                GBAR89_FLAG_DRAW_MID_VALUE |
                GBAR89_FLAG_DRAW_PATTERN |
                GBAR89_FLAG_DRAW_MARKERS;
    if (segmented) gbar89_set_segments(m, 12, 4);
    m->style.padding_left = 7;
    m->style.padding_right = 7;
    m->style.padding_top = 7;
    m->style.padding_bottom = 7;
    m->style.color_bg = 0x101B29FFUL;
    m->style.color_empty = 0x172638FFUL;
    m->style.color_fill = fill;
    m->style.color_lag = 0xFF6874FFUL;
    m->style.color_mid = 0xFFD65AFFUL;
    m->style.color_border = 0xE1F5FFFFUL;
    m->style.color_outline = 0x02060CFFUL;
    m->style.color_highlight = 0xFFFFFFFFUL;
    m->style.color_shadow = 0x05101CFFUL;
    m->style.color_extrude = 0x071523FFUL;
    m->style.color_pattern = 0x092C3E90UL;
    m->style.color_bg_detail = 0x3F9BBD42UL;
    m->style.color_marker = 0xFFFFFFFFUL;
    gbar89_set_frame(m, segmented ? GBAR89_FRAME_RAIL : GBAR89_FRAME_BEVEL_OUT,
                     3, 2, 14);
    gbar89_set_outline(m, 2, 0x02060CFFUL);
    gbar89_set_background(m, segmented ? GBAR89_BG_CHECKER : GBAR89_BG_GRID,
                          12, 1, 0x3F9BBD42UL);
    gbar89_set_pattern(m, GBAR89_PATTERN_CROSSHATCH, 11, 1);
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_EXTRUDE |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_INNER_SHADOW |
                            GBAR89_FX_FILL_GRID |
                            GBAR89_FX_PIXEL_CELLS);
    gbar89_set_pixel_size(m, 10);
    gbar89_add_marker(m, 25);
    gbar89_add_marker(m, 50);
    gbar89_add_marker(m, 75);
}

static void render_preview(void)
{
    GBar89_RenderOps ops;
    GBar89_Meter r1;
    GBar89_Meter r2;
    GBar89_Meter r3;
    GBar89_Meter r4;
    GBar89_Meter v1;
    GBar89_Meter v2;
    GBar89_Meter v3;

    g_w = PREVIEW_W;
    g_h = PREVIEW_H;
    g_clip_count = 0;
    clear_rgb(0x070C15UL);
    scene_grid();
    ops = make_ops();

    panel(40, 42, 1200, 486);
    panel(40, 558, 1200, 220);

    setup_holo_ring(&r1, 92, 92, 190);
    setup_segment_ring(&r2, 345, 92, 190);
    setup_radar_pie(&r3, 598, 92, 190);
    setup_pixel_ring(&r4, 851, 92, 190);

    setup_vertical(&v1, 1075, 86, 58, 390,
                   GBAR89_DIR_BOTTOM_TO_TOP, 0, 0x29E5C2FFUL);
    setup_vertical(&v2, 1150, 86, 58, 390,
                   GBAR89_DIR_TOP_TO_BOTTOM, 1, 0x7D73FFFFUL);

    setup_vertical(&v3, 92, 592, 90, 150,
                   GBAR89_DIR_CENTER_VERTICAL, 0, 0xFF609EFFUL);
    gbar89_set_unit_vector(&v3, chevron_points, 6, 1, 1, 7, 2, 70);
    v3.flags |= GBAR89_FLAG_DRAW_VECTOR_UNITS;

    gbar89_draw(&r1, &ops);
    gbar89_draw(&r2, &ops);
    gbar89_draw(&r3, &ops);
    gbar89_draw(&r4, &ops);
    gbar89_draw(&v1, &ops);
    gbar89_draw(&v2, &ops);
    gbar89_draw(&v3, &ops);

    setup_vertical(&v1, 245, 592, 90, 150,
                   GBAR89_DIR_BOTTOM_TO_TOP, 1, 0x67B8FFFFUL);
    setup_vertical(&v2, 398, 592, 90, 150,
                   GBAR89_DIR_TOP_TO_BOTTOM, 0, 0xA8DF55FFUL);
    setup_vertical(&v3, 551, 592, 90, 150,
                   GBAR89_DIR_CENTER_VERTICAL, 1, 0xFFB64FFFUL);
    gbar89_draw(&v1, &ops);
    gbar89_draw(&v2, &ops);
    gbar89_draw(&v3, &ops);

    setup_holo_ring(&r1, 720, 575, 160);
    r1.radial_start_deg = -180;
    r1.radial_sweep_deg = 180;
    r1.style.frame_kind = GBAR89_FRAME_BRACKETS;
    r1.style.frame_corner = 20;
    gbar89_draw(&r1, &ops);

    setup_segment_ring(&r2, 930, 575, 160);
    r2.style.frame_kind = GBAR89_FRAME_BEVEL_OUT;
    r2.style.radial_segments = 10;
    r2.style.radial_gap_deg = 7;
    gbar89_draw(&r2, &ops);
}

static int tri_wave(int frame, int period, int lo, int hi)
{
    int t;
    int half;
    int span;

    if (period < 2) period = 2;
    t = frame % period;
    half = period / 2;
    span = hi - lo;
    if (t <= half) return lo + (span * t) / half;
    return hi - (span * (t - half)) / (period - half);
}

static void render_animation_frame(int frame)
{
    GBar89_RenderOps ops;
    GBar89_Meter r1;
    GBar89_Meter r2;
    GBar89_Meter v1;
    GBar89_Meter v2;
    int value;
    int mid;

    g_w = ANIM_W;
    g_h = ANIM_H;
    g_clip_count = 0;
    clear_rgb(0x070C15UL);
    scene_grid();
    ops = make_ops();

    panel(34, 32, 892, 474);

    setup_holo_ring(&r1, 76, 78, 250);
    value = tri_wave(frame, 48, 8, 94);
    gbar89_set_value(&r1, value);
    r1.visual_value = value;
    r1.lag_value = value + 14;
    if (r1.lag_value > 100) r1.lag_value = 100;
    mid = value + 8;
    if (mid > 100) mid = 100;
    gbar89_set_mid_value(&r1, mid);
    r1.mid_visual_value = mid;
    r1.style.radial_phase_deg = frame * 4;

    setup_segment_ring(&r2, 360, 78, 250);
    value = tri_wave(frame + 15, 44, 4, 98);
    gbar89_set_value(&r2, value);
    r2.visual_value = value;
    r2.lag_value = value + 10;
    if (r2.lag_value > 100) r2.lag_value = 100;
    r2.style.radial_phase_deg = -frame * 5;
    r2.radial_start_deg = -130 + frame * 3;

    setup_vertical(&v1, 690, 68, 68, 390,
                   GBAR89_DIR_BOTTOM_TO_TOP, 0, 0x29E5C2FFUL);
    value = tri_wave(frame + 7, 48, 5, 98);
    gbar89_set_value(&v1, value);
    v1.visual_value = value;
    v1.lag_value = value + 12;
    if (v1.lag_value > 100) v1.lag_value = 100;

    setup_vertical(&v2, 800, 68, 68, 390,
                   GBAR89_DIR_TOP_TO_BOTTOM, 1, 0x8B7DFFFFUL);
    value = tri_wave(frame + 24, 40, 0, 100);
    gbar89_set_value(&v2, value);
    v2.visual_value = value;

    gbar89_draw(&r1, &ops);
    gbar89_draw(&r2, &ops);
    gbar89_draw(&v1, &ops);
    gbar89_draw(&v2, &ops);
}

static int write_ppm(const char *path)
{
    FILE *f;
    size_t n;
    size_t want;

    f = fopen(path, "wb");
    if (f == 0) return 0;
    fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
    want = (size_t)g_w * (size_t)g_h * 3U;
    n = fwrite(g_pixels, 1U, want, f);
    fclose(f);
    return n == want;
}

int main(void)
{
    int i;
    char path[256];

    render_preview();
    if (!write_ppm("radial_vertical_preview_raw.ppm")) return 1;

    for (i = 0; i < 48; ++i) {
        render_animation_frame(i);
        sprintf(path, "radial_vertical_frames/frame_%03d.ppm", i);
        if (!write_ppm(path)) return 2;
    }
    return 0;
}
