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

static void setup_boss(GBar89_Meter *m, int x, int y, int w)
{
    gbar89_init(m);
    gbar89_set_rect(m, x, y, w, 42);
    gbar89_set_range(m, 0, 3000);
    gbar89_set_layers(m, 3, 1000);
    gbar89_set_value(m, 2320);
    m->visual_value = 2320;
    m->lag_value = 2760;
    gbar89_set_mid_value(m, 2480);
    m->mid_visual_value = 2480;
    m->flags |= GBAR89_FLAG_LAYERED |
                GBAR89_FLAG_DRAW_LAYER_PIPS |
                GBAR89_FLAG_DAMAGE_LAG |
                GBAR89_FLAG_DRAW_MID_VALUE |
                GBAR89_FLAG_DRAW_PATTERN |
                GBAR89_FLAG_AUTO_STATE;
    m->style.padding_left = 6;
    m->style.padding_right = 6;
    m->style.padding_top = 6;
    m->style.padding_bottom = 6;
    m->style.color_bg = 0x07131FFFUL;
    m->style.color_fill = 0x27E7C8FFUL;
    m->style.color_lag = 0xFF5D6CFFUL;
    m->style.color_mid = 0xF7D154FFUL;
    m->style.color_border = 0x8FF9E9FFUL;
    m->style.color_highlight = 0xE9FFFFFFUL;
    m->style.color_shadow = 0x023B4AFFUL;
    m->style.color_outline = 0x02070DFFUL;
    m->style.color_extrude = 0x031522FFUL;
    m->style.color_pattern = 0x002B38A0UL;
    m->style.color_bg_detail = 0x138B9180UL;
    m->style.color_gloss = 0xFFFFFF45UL;
    gbar89_set_frame(m, GBAR89_FRAME_DOUBLE, 2, 2, 10);
    gbar89_set_outline(m, 2, 0x02070DFFUL);
    gbar89_set_background(m, GBAR89_BG_GRID, 8, 1, 0x138B9180UL);
    gbar89_set_pattern(m, GBAR89_PATTERN_VERTICAL_STRIPES, 11, 2);
    gbar89_set_mask(m, GBAR89_MASK_SLANT_RIGHT, 12, 8);
    gbar89_set_fx_flags(m, GBAR89_FX_OUTER_OUTLINE |
                            GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_EXTRUDE |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_INNER_SHADOW);
    gbar89_add_marker(m, 750);
    gbar89_add_marker(m, 1500);
    gbar89_add_marker(m, 2250);
    m->flags |= GBAR89_FLAG_DRAW_MARKERS;
    m->style.color_marker = 0xFFFFFFFFUL;
}

static const GBar89_VectorPoint diamond_points[4] = {
    {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
};

static const GBar89_VectorPoint bolt_points[8] = {
    {560, 0}, {170, 540}, {430, 540}, {280, 1000},
    {830, 390}, {570, 390}, {760, 0}, {560, 0}
};

static void setup_vector_units(GBar89_Meter *m, int x, int y, int w)
{
    gbar89_init(m);
    gbar89_set_rect(m, x, y, w, 54);
    gbar89_set_range(m, 0, 8);
    gbar89_set_value(m, 5);
    m->visual_value = 5;
    m->flags |= GBAR89_FLAG_DRAW_VECTOR_UNITS;
    m->style.padding_left = 5;
    m->style.padding_right = 5;
    m->style.padding_top = 5;
    m->style.padding_bottom = 5;
    m->style.color_bg = 0x221A13FFUL;
    m->style.color_fill = 0xF59E42FFUL;
    m->style.color_border = 0xFFD2A1FFUL;
    m->style.color_unit_fill = 0xFFD36BFFUL;
    m->style.color_unit_empty = 0x4C3325FFUL;
    m->style.color_unit_outline = 0x1B0D08FFUL;
    m->style.color_bg_detail = 0xE88B3A35UL;
    m->style.color_shadow = 0x2B1008FFUL;
    m->style.color_highlight = 0xFFF2D2FFUL;
    gbar89_set_frame(m, GBAR89_FRAME_RAIL, 3, 1, 8);
    gbar89_set_background(m, GBAR89_BG_DIAGONAL, 10, 2, 0xE88B3A35UL);
    gbar89_set_fx_flags(m, GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_GLOSS);
    gbar89_set_unit_vector(m, bolt_points, 7, 1, 1, 8, 3, 76);
}

static void setup_pixel(GBar89_Meter *m, int x, int y, int w)
{
    gbar89_init(m);
    gbar89_set_rect(m, x, y, w, 38);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, 67);
    m->visual_value = 67;
    gbar89_set_mid_value(m, 82);
    m->mid_visual_value = 82;
    m->flags |= GBAR89_FLAG_DRAW_MID_VALUE |
                GBAR89_FLAG_PIXEL_QUANTIZE;
    m->style.padding_left = 4;
    m->style.padding_right = 4;
    m->style.padding_top = 4;
    m->style.padding_bottom = 4;
    m->style.color_bg = 0x15131EFFUL;
    m->style.color_fill = 0xB06BFFFFUL;
    m->style.color_mid = 0x5CE1E6FFUL;
    m->style.color_border = 0xF2E8FFFFUL;
    m->style.color_shadow = 0x1B0A2AFFUL;
    m->style.color_highlight = 0xFFFFFFFFUL;
    m->style.color_pattern = 0x35114C90UL;
    m->style.color_bg_detail = 0x6B4A8438UL;
    gbar89_set_pixel_size(m, 10);
    gbar89_set_frame(m, GBAR89_FRAME_PIXEL, 3, 1, 8);
    gbar89_set_background(m, GBAR89_BG_CHECKER, 10, 10, 0x6B4A8438UL);
    gbar89_set_fx_flags(m, GBAR89_FX_PIXEL_CELLS |
                            GBAR89_FX_FILL_SCANLINES |
                            GBAR89_FX_INNER_SHADOW |
                            GBAR89_FX_OUTER_OUTLINE);
    gbar89_set_outline(m, 2, 0x07030DFFUL);
}

static void setup_bevel(GBar89_Meter *m, int x, int y, int w)
{
    gbar89_init(m);
    gbar89_set_rect(m, x, y, w, 46);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, 74);
    m->visual_value = 74;
    m->flags |= GBAR89_FLAG_DRAW_PATTERN;
    m->style.padding_left = 7;
    m->style.padding_right = 7;
    m->style.padding_top = 7;
    m->style.padding_bottom = 7;
    m->style.color_bg = 0x1C2418FFUL;
    m->style.color_fill = 0x7FD34EFFUL;
    m->style.color_border = 0xA9C896FFUL;
    m->style.color_highlight = 0xE9FFD8FFUL;
    m->style.color_shadow = 0x223019FFUL;
    m->style.color_extrude = 0x11180DFFUL;
    m->style.color_pattern = 0x365C24A0UL;
    m->style.color_bg_detail = 0x566A4338UL;
    gbar89_set_pattern(m, GBAR89_PATTERN_DOTS, 7, 2);
    gbar89_set_frame(m, GBAR89_FRAME_BEVEL_OUT, 4, 1, 10);
    gbar89_set_background(m, GBAR89_BG_DITHER, 5, 1, 0x566A4338UL);
    gbar89_set_fx_flags(m, GBAR89_FX_EXTRUDE |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_INNER_SHADOW);
    m->style.shadow_offset_x = 1;
    m->style.shadow_offset_y = 1;
    m->style.extrude_depth = 7;
}

static void setup_segmented(GBar89_Meter *m, int x, int y, int w)
{
    gbar89_init(m);
    gbar89_set_kind(m, GBAR89_KIND_SEGMENTED);
    gbar89_set_rect(m, x, y, w, 42);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, 58);
    m->visual_value = 58;
    gbar89_set_segments(m, 12, 3);
    m->style.padding_left = 5;
    m->style.padding_right = 5;
    m->style.padding_top = 5;
    m->style.padding_bottom = 5;
    m->style.color_empty = 0x182635FFUL;
    m->style.color_fill = 0x58B7FFFFUL;
    m->style.color_border = 0xB7DEFFFFUL;
    m->style.color_highlight = 0xFFFFFFFFUL;
    m->style.color_shadow = 0x0B1520FFUL;
    m->style.color_bg_detail = 0x64C3FF38UL;
    gbar89_set_frame(m, GBAR89_FRAME_BRACKETS, 2, 1, 14);
    gbar89_set_background(m, GBAR89_BG_SCANLINES, 4, 1, 0x64C3FF38UL);
    gbar89_set_fx_flags(m, GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_GLOSS |
                            GBAR89_FX_FILL_GRID);
}

static void setup_radial(GBar89_Meter *m, int x, int y, int size,
                         int ring, unsigned long fill)
{
    gbar89_init(m);
    gbar89_set_kind(m, ring ? GBAR89_KIND_RADIAL_RING : GBAR89_KIND_RADIAL_PIE);
    gbar89_set_rect(m, x, y, size, size);
    gbar89_set_range(m, 0, 100);
    gbar89_set_value(m, ring ? 72 : 41);
    m->visual_value = m->value;
    gbar89_set_radial(m, -90, ring ? 300 : 360, ring ? 62 : 0, 56);
    m->style.color_bg = 0x172131FFUL;
    m->style.color_fill = fill;
    m->style.color_border = 0xDAE9FFFFUL;
    m->style.color_shadow = 0x04070DFFUL;
    m->style.color_highlight = 0xFFFFFFFFUL;
    gbar89_set_frame(m, ring ? GBAR89_FRAME_DOUBLE : GBAR89_FRAME_BEVEL_IN,
                     2, 2, 8);
    gbar89_set_fx_flags(m, GBAR89_FX_DROP_SHADOW |
                            GBAR89_FX_OUTER_OUTLINE);
    gbar89_set_outline(m, 2, 0x03070DFFUL);
}

static void render_preview(void)
{
    GBar89_RenderOps ops;
    GBar89_Meter boss;
    GBar89_Meter units;
    GBar89_Meter pixel;
    GBar89_Meter bevel;
    GBar89_Meter segmented;
    GBar89_Meter radial1;
    GBar89_Meter radial2;

    g_w = PREVIEW_W;
    g_h = PREVIEW_H;
    g_clip_count = 0;
    clear_rgb(0x080D16UL);
    scene_grid();
    ops = make_ops();

    panel(58, 72, 1164, 126);
    panel(58, 226, 560, 152);
    panel(662, 226, 560, 152);
    panel(58, 408, 560, 150);
    panel(662, 408, 560, 150);
    panel(58, 590, 1164, 170);

    setup_boss(&boss, 104, 124, 930);
    setup_vector_units(&units, 104, 278, 460);
    setup_pixel(&pixel, 708, 289, 460);
    setup_bevel(&bevel, 104, 467, 460);
    setup_segmented(&segmented, 708, 469, 460);
    setup_radial(&radial1, 170, 635, 118, 1, 0x34D1B6FFUL);
    setup_radial(&radial2, 360, 635, 118, 0, 0xFF6B9DFFUL);

    gbar89_draw(&boss, &ops);
    gbar89_draw(&units, &ops);
    gbar89_draw(&pixel, &ops);
    gbar89_draw(&bevel, &ops);
    gbar89_draw(&segmented, &ops);
    gbar89_draw(&radial1, &ops);
    gbar89_draw(&radial2, &ops);

    setup_vector_units(&units, 590, 650, 520);
    units.style.frame_kind = GBAR89_FRAME_DOUBLE;
    units.style.color_unit_fill = 0x81F4E1FFUL;
    units.style.color_unit_empty = 0x243A46FFUL;
    units.style.color_border = 0xB7FFF5FFUL;
    units.style.color_bg = 0x0D2027FFUL;
    units.style.color_bg_detail = 0x32C8B438UL;
    gbar89_set_background(&units, GBAR89_BG_GRID, 9, 1, 0x32C8B438UL);
    gbar89_set_unit_vector(&units, diamond_points, 4, 1, 1, 8, 4, 70);
    gbar89_draw(&units, &ops);
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
    if (t <= half) {
        return lo + (span * t) / half;
    }
    return hi - (span * (t - half)) / (period - half);
}

static void render_animation_frame(int frame)
{
    GBar89_RenderOps ops;
    GBar89_Meter boss;
    GBar89_Meter units;
    GBar89_Meter pixel;
    GBar89_Meter radial;
    int v;
    int mid;

    g_w = ANIM_W;
    g_h = ANIM_H;
    g_clip_count = 0;
    clear_rgb(0x080D16UL);
    scene_grid();
    ops = make_ops();

    panel(36, 42, 888, 116);
    panel(36, 190, 430, 122);
    panel(494, 190, 430, 122);
    panel(36, 342, 888, 142);

    setup_boss(&boss, 78, 88, 700);
    v = tri_wave(frame, 48, 240, 2920);
    mid = v + 280;
    if (mid > 3000) mid = 3000;
    gbar89_set_value(&boss, v);
    boss.visual_value = v;
    boss.lag_value = v + 320;
    if (boss.lag_value > 3000) boss.lag_value = 3000;
    gbar89_set_mid_value(&boss, mid);
    boss.mid_visual_value = mid;
    if (v < 500) {
        boss.flags |= GBAR89_FLAG_STATE_BLINK;
        boss.state = GBAR89_STATE_CRITICAL;
    }

    setup_vector_units(&units, 78, 226, 344);
    v = tri_wave(frame + 8, 32, 0, 8);
    gbar89_set_value(&units, v);
    units.visual_value = v;

    setup_pixel(&pixel, 540, 235, 336);
    v = tri_wave(frame + 16, 40, 5, 98);
    gbar89_set_value(&pixel, v);
    pixel.visual_value = v;
    mid = v + 18;
    if (mid > 100) mid = 100;
    gbar89_set_mid_value(&pixel, mid);
    pixel.mid_visual_value = mid;

    setup_radial(&radial, 390, 358, 110, 1, 0x34D1B6FFUL);
    v = tri_wave(frame + 4, 48, 4, 98);
    gbar89_set_value(&radial, v);
    radial.visual_value = v;
    radial.radial_start_deg = -90 + frame * 6;

    gbar89_draw(&boss, &ops);
    gbar89_draw(&units, &ops);
    gbar89_draw(&pixel, &ops);
    gbar89_draw(&radial, &ops);

    setup_segmented(&units, 548, 391, 310);
    v = tri_wave(frame + 22, 44, 0, 100);
    gbar89_set_value(&units, v);
    units.visual_value = v;
    gbar89_draw(&units, &ops);
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
    if (!write_ppm("preview_raw.ppm")) return 1;

    for (i = 0; i < 48; ++i) {
        render_animation_frame(i);
        sprintf(path, "preview_frames/frame_%03d.ppm", i);
        if (!write_ppm(path)) return 2;
    }

    return 0;
}
