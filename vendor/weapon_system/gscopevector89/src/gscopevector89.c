/*
 * gscopevector89.c
 * C89 vector reticle geometry. No dynamic allocation.
 */
#include "../include/gscopevector89.h"

#define GSV89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static const short gsv89_sin64_q15[64] = {
    0, 3212, 6393, 9512, 12539, 15446, 18205, 20787,
    23170, 25329, 27245, 28898, 30274, 31356, 32138, 32610,
    32767, 32610, 32138, 31356, 30274, 28898, 27245, 25329,
    23170, 20787, 18205, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18205, -20787,
    -23170, -25329, -27245, -28898, -30274, -31356, -32138, -32610,
    -32767, -32610, -32138, -31356, -30274, -28898, -27245, -25329,
    -23170, -20787, -18205, -15446, -12539, -9512, -6393, -3212
};

static short gsv89_angle_index(long deg_x100)
{
    long d;
    d = deg_x100;
    while (d < 0L) d += 36000L;
    while (d >= 36000L) d -= 36000L;
    d = (d * 64L) / 36000L;
    return (short)(d & 63L);
}

static gsp89_fx gsv89_sin_fx(long deg_x100)
{
    short i;
    i = gsv89_angle_index(deg_x100);
    return (gsp89_fx)((long)gsv89_sin64_q15[i] * 2L);
}

static gsp89_fx gsv89_cos_fx(long deg_x100)
{
    return gsv89_sin_fx(deg_x100 + 9000L);
}

static gsp89_fx gsv89_lerp(gsp89_fx a, gsp89_fx b, short t_x1000)
{
    return a + (gsp89_fx)(((b - a) * (long)t_x1000) / 1000L);
}

void gsv89_palette_init(gsv89_palette *palette)
{
    short i;
    if (!palette) return;
    for (i = 0; i < GSV89_MAX_PARTS; ++i) {
        palette->part[i].color = gsp89_rgba(255, 255, 255, 255);
        palette->part[i].outline_color = gsp89_rgba(0, 0, 0, 255);
        palette->part[i].thickness_px = 1;
        palette->part[i].outline_px = 0;
        palette->part[i].layer = 10;
        palette->part[i].blend_mode = GSP89_BLEND_ALPHA;
        palette->part[i].flags = GSP89_FLAG_VISIBLE;
        palette->part[i].visible = 1;
    }
    palette->part[GSV89_PART_ILLUMINATION].color = gsp89_rgba(255, 32, 32, 255);
    palette->part[GSV89_PART_WARNING].color = gsp89_rgba(255, 176, 32, 255);
}

void gsv89_palette_set_part(gsv89_palette *palette,
                            short part_id,
                            gsp89_color color,
                            gsp89_color outline_color,
                            short thickness_px,
                            short outline_px,
                            short layer,
                            short blend_mode,
                            short flags,
                            short visible)
{
    if (!palette) return;
    if (part_id < 0 || part_id >= GSV89_MAX_PARTS) return;
    palette->part[part_id].color = color;
    palette->part[part_id].outline_color = outline_color;
    palette->part[part_id].thickness_px = thickness_px < 1 ? 1 : thickness_px;
    palette->part[part_id].outline_px = outline_px < 0 ? 0 : outline_px;
    palette->part[part_id].layer = layer;
    palette->part[part_id].blend_mode = blend_mode;
    palette->part[part_id].flags = flags | GSP89_FLAG_VISIBLE;
    palette->part[part_id].visible = visible ? 1 : 0;
}

gsp89_style gsv89_style_for(const gsv89_palette *palette,
                             const gsv89_shape *shape,
                             short global_alpha)
{
    gsv89_part_style ps;
    gsp89_color c;
    gsp89_color oc;
    long a;
    long oa;
    short part;
    short thickness;
    short outline;
    short layer;
    short flags;

    part = shape ? shape->part_id : 0;
    if (part < 0 || part >= GSV89_MAX_PARTS) part = 0;

    if (palette) {
        ps = palette->part[part];
    } else {
        ps.color = gsp89_rgba(255, 255, 255, 255);
        ps.outline_color = gsp89_rgba(0, 0, 0, 255);
        ps.thickness_px = 1;
        ps.outline_px = 0;
        ps.layer = 10;
        ps.blend_mode = GSP89_BLEND_ALPHA;
        ps.flags = GSP89_FLAG_VISIBLE;
        ps.visible = 1;
    }

    c = ps.color;
    oc = ps.outline_color;
    global_alpha = (short)GSV89_CLAMP(global_alpha, 0, 255);
    a = ((long)c.a * (long)global_alpha) / 255L;
    oa = ((long)oc.a * (long)global_alpha) / 255L;
    c.a = (unsigned char)GSV89_CLAMP(a, 0L, 255L);
    oc.a = (unsigned char)GSV89_CLAMP(oa, 0L, 255L);

    thickness = shape && shape->thickness_px > 0 ? shape->thickness_px : ps.thickness_px;
    outline = shape && shape->outline_px > 0 ? shape->outline_px : ps.outline_px;
    layer = shape && shape->layer != 0 ? shape->layer : ps.layer;
    flags = ps.flags;
    if (shape) flags = (short)(flags | shape->flags);
    if (outline > 0) flags = (short)(flags | GSP89_FLAG_OUTLINE);

    return gsp89_style_make(c, oc, thickness, outline, layer, part,
                            ps.blend_mode, flags);
}

gsv89_shape gsv89_shape_empty(void)
{
    gsv89_shape s;
    s.kind = GSV89_SHAPE_NONE;
    s.part_id = 0;
    s.flags = GSV89_FLAG_VISIBLE;
    s.layer = 0;
    s.thickness_px = 0;
    s.outline_px = 0;
    s.x0 = 0;
    s.y0 = 0;
    s.x1 = 0;
    s.y1 = 0;
    s.x2 = 0;
    s.y2 = 0;
    s.x3 = 0;
    s.y3 = 0;
    s.a = 0;
    s.b = 0;
    s.c = 0;
    s.d = 0;
    s.i0 = 0;
    s.i1 = 0;
    s.i2 = 0;
    s.i3 = 0;
    s.points = 0;
    s.point_count = 0;
    return s;
}

gsv89_shape gsv89_line(short part_id,
                        gsp89_fx x0,
                        gsp89_fx y0,
                        gsp89_fx x1,
                        gsp89_fx y1)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_LINE;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    return s;
}

gsv89_shape gsv89_hline(short part_id, gsp89_fx x0, gsp89_fx x1, gsp89_fx y)
{
    gsv89_shape s;
    s = gsv89_line(part_id, x0, y, x1, y);
    s.kind = GSV89_SHAPE_HLINE;
    return s;
}

gsv89_shape gsv89_vline(short part_id, gsp89_fx x, gsp89_fx y0, gsp89_fx y1)
{
    gsv89_shape s;
    s = gsv89_line(part_id, x, y0, x, y1);
    s.kind = GSV89_SHAPE_VLINE;
    return s;
}

gsv89_shape gsv89_rect(short part_id,
                        gsp89_fx x0,
                        gsp89_fx y0,
                        gsp89_fx x1,
                        gsp89_fx y1,
                        short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_RECT;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_square(short part_id,
                          gsp89_fx cx,
                          gsp89_fx cy,
                          gsp89_fx half_size,
                          short filled)
{
    gsv89_shape s;
    s = gsv89_rect(part_id, cx - half_size, cy - half_size,
                   cx + half_size, cy + half_size, filled);
    s.kind = GSV89_SHAPE_SQUARE;
    return s;
}

gsv89_shape gsv89_triangle(short part_id,
                            gsp89_fx x0,
                            gsp89_fx y0,
                            gsp89_fx x1,
                            gsp89_fx y1,
                            gsp89_fx x2,
                            gsp89_fx y2,
                            short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_TRIANGLE;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    s.x2 = x2;
    s.y2 = y2;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_circle(short part_id, gsp89_fx cx, gsp89_fx cy,
                          gsp89_fx radius, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_CIRCLE;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_ellipse(short part_id, gsp89_fx cx, gsp89_fx cy,
                           gsp89_fx radius_x, gsp89_fx radius_y, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_ELLIPSE;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius_x;
    s.b = radius_y;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_arc(short part_id, gsp89_fx cx, gsp89_fx cy,
                       gsp89_fx radius_x, gsp89_fx radius_y,
                       short start_deg_x100, short end_deg_x100)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_ARC;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius_x;
    s.b = radius_y;
    s.i0 = start_deg_x100;
    s.i1 = end_deg_x100;
    return s;
}

gsv89_shape gsv89_dot(short part_id, gsp89_fx cx, gsp89_fx cy, gsp89_fx radius)
{
    gsv89_shape s;
    s = gsv89_circle(part_id, cx, cy, radius, 1);
    s.kind = GSV89_SHAPE_DOT;
    return s;
}

gsv89_shape gsv89_cross(short part_id, gsp89_fx cx, gsp89_fx cy,
                         gsp89_fx half_w, gsp89_fx half_h, gsp89_fx gap)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_CROSS;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = half_w;
    s.b = half_h;
    s.c = gap;
    return s;
}

gsv89_shape gsv89_xcross(short part_id, gsp89_fx cx, gsp89_fx cy,
                          gsp89_fx half_w, gsp89_fx half_h, gsp89_fx gap)
{
    gsv89_shape s;
    s = gsv89_cross(part_id, cx, cy, half_w, half_h, gap);
    s.kind = GSV89_SHAPE_XCROSS;
    return s;
}

gsv89_shape gsv89_chevron(short part_id, gsp89_fx cx, gsp89_fx cy,
                           gsp89_fx half_w, gsp89_fx height,
                           short direction, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_CHEVRON;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = half_w;
    s.b = height;
    s.i0 = direction;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_diamond(short part_id, gsp89_fx cx, gsp89_fx cy,
                           gsp89_fx half_w, gsp89_fx half_h, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_DIAMOND;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = half_w;
    s.b = half_h;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_parenthesis(short part_id, gsp89_fx cx, gsp89_fx cy,
                               gsp89_fx radius_x, gsp89_fx radius_y, short side)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_PARENTHESIS;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius_x;
    s.b = radius_y;
    s.i0 = side < 0 ? GSV89_SIDE_LEFT : GSV89_SIDE_RIGHT;
    return s;
}

gsv89_shape gsv89_bracket(short part_id, gsp89_fx cx, gsp89_fx cy,
                           gsp89_fx half_h, gsp89_fx arm, short side)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_BRACKET;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = half_h;
    s.b = arm;
    s.i0 = side < 0 ? GSV89_SIDE_LEFT : GSV89_SIDE_RIGHT;
    return s;
}

gsv89_shape gsv89_horseshoe(short part_id, gsp89_fx cx, gsp89_fx cy,
                             gsp89_fx radius_x, gsp89_fx radius_y,
                             short open_deg_x100)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_HORSESHOE;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius_x;
    s.b = radius_y;
    s.i0 = open_deg_x100;
    return s;
}

gsv89_shape gsv89_regular_polygon(short part_id, gsp89_fx cx, gsp89_fx cy,
                                   gsp89_fx radius, short sides,
                                   short rotation_deg_x100, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_REGULAR_POLYGON;
    s.part_id = part_id;
    s.x0 = cx;
    s.y0 = cy;
    s.a = radius;
    s.i0 = sides;
    s.i1 = rotation_deg_x100;
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_path(short part_id, const gsv89_point *points,
                        short point_count, short closed, short filled)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_PATH;
    s.part_id = part_id;
    s.points = points;
    s.point_count = point_count;
    if (closed) s.flags = (short)(s.flags | GSV89_FLAG_CLOSED);
    if (filled) s.flags = (short)(s.flags | GSV89_FLAG_FILLED);
    return s;
}

gsv89_shape gsv89_grid(short part_id, gsp89_fx x0, gsp89_fx y0,
                        gsp89_fx x1, gsp89_fx y1,
                        short columns, short rows)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_GRID;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    s.i0 = columns;
    s.i1 = rows;
    return s;
}

gsv89_shape gsv89_tick_strip(short part_id,
                              gsp89_fx x0, gsp89_fx y0,
                              gsp89_fx x1, gsp89_fx y1,
                              gsp89_fx tick_half,
                              short count, short major_every, short axis)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_TICK_STRIP;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    s.a = tick_half;
    s.i0 = count;
    s.i1 = major_every;
    s.i2 = axis;
    return s;
}

gsv89_shape gsv89_bezier_quad(short part_id,
                               gsp89_fx x0, gsp89_fx y0,
                               gsp89_fx x1, gsp89_fx y1,
                               gsp89_fx x2, gsp89_fx y2,
                               short segments)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_BEZIER_QUAD;
    s.part_id = part_id;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    s.x2 = x2;
    s.y2 = y2;
    s.i0 = segments;
    return s;
}

gsv89_shape gsv89_glyph(short part_id, short glyph_id,
                         gsp89_fx x, gsp89_fx y,
                         gsp89_fx size_x, gsp89_fx size_y)
{
    gsv89_shape s;
    s = gsv89_shape_empty();
    s.kind = GSV89_SHAPE_GLYPH;
    s.part_id = part_id;
    s.x0 = x;
    s.y0 = y;
    s.a = size_x;
    s.b = size_y;
    s.i0 = glyph_id;
    return s;
}

void gsv89_builder_init(gsv89_builder *builder, gsv89_shape *storage, short capacity)
{
    if (!builder) return;
    builder->items = storage;
    builder->capacity = capacity < 0 ? 0 : capacity;
    builder->count = 0;
    builder->overflowed = 0;
}

int gsv89_builder_push(gsv89_builder *builder, const gsv89_shape *shape)
{
    if (!builder || !shape || !builder->items) return 0;
    if (builder->count >= builder->capacity) {
        builder->overflowed = 1;
        return 0;
    }
    builder->items[builder->count] = *shape;
    builder->count = (short)(builder->count + 1);
    return 1;
}

static int gsv89_part_visible(const gsv89_palette *palette, short part_id)
{
    if (!palette) return 1;
    if (part_id < 0 || part_id >= GSV89_MAX_PARTS) return 1;
    return palette->part[part_id].visible ? 1 : 0;
}

static void gsv89_emit_polyline(gsp89_painter *painter,
                                const gsv89_point *pts,
                                short count,
                                short closed,
                                short filled,
                                const gsp89_style *style)
{
    short i;
    if (!painter || !pts || count < 2 || !style) return;
    if (filled && count >= 3) {
        for (i = 1; i < count - 1; ++i) {
            gsp89_paint_triangle(painter,
                                 pts[0].x, pts[0].y,
                                 pts[i].x, pts[i].y,
                                 pts[i + 1].x, pts[i + 1].y,
                                 style);
        }
    } else {
        for (i = 0; i < count - 1; ++i) {
            gsp89_paint_line(painter,
                             pts[i].x, pts[i].y,
                             pts[i + 1].x, pts[i + 1].y,
                             style);
        }
        if (closed) {
            gsp89_paint_line(painter,
                             pts[count - 1].x, pts[count - 1].y,
                             pts[0].x, pts[0].y,
                             style);
        }
    }
}

static void gsv89_emit_cross(gsp89_painter *painter,
                             const gsv89_shape *s,
                             const gsp89_style *style)
{
    gsp89_fx cx;
    gsp89_fx cy;
    gsp89_fx hw;
    gsp89_fx hh;
    gsp89_fx gap;
    cx = s->x0;
    cy = s->y0;
    hw = s->a;
    hh = s->b;
    gap = s->c;
    gsp89_paint_line(painter, cx - hw, cy, cx - gap, cy, style);
    gsp89_paint_line(painter, cx + gap, cy, cx + hw, cy, style);
    gsp89_paint_line(painter, cx, cy - hh, cx, cy - gap, style);
    gsp89_paint_line(painter, cx, cy + gap, cx, cy + hh, style);
}

static void gsv89_emit_xcross(gsp89_painter *painter,
                              const gsv89_shape *s,
                              const gsp89_style *style)
{
    gsp89_fx cx;
    gsp89_fx cy;
    gsp89_fx hw;
    gsp89_fx hh;
    gsp89_fx gap;
    cx = s->x0;
    cy = s->y0;
    hw = s->a;
    hh = s->b;
    gap = s->c;
    gsp89_paint_line(painter, cx - hw, cy - hh, cx - gap, cy - gap, style);
    gsp89_paint_line(painter, cx + gap, cy + gap, cx + hw, cy + hh, style);
    gsp89_paint_line(painter, cx - hw, cy + hh, cx - gap, cy + gap, style);
    gsp89_paint_line(painter, cx + gap, cy - gap, cx + hw, cy - hh, style);
}

static void gsv89_emit_chevron(gsp89_painter *painter,
                               const gsv89_shape *s,
                               const gsp89_style *style)
{
    gsv89_point p[3];
    gsp89_fx cx;
    gsp89_fx cy;
    gsp89_fx hw;
    gsp89_fx h;
    cx = s->x0;
    cy = s->y0;
    hw = s->a;
    h = s->b;
    if (s->i0 == GSV89_DIR_DOWN) {
        p[0].x = cx - hw; p[0].y = cy - h;
        p[1].x = cx;      p[1].y = cy;
        p[2].x = cx + hw; p[2].y = cy - h;
    } else if (s->i0 == GSV89_DIR_LEFT) {
        p[0].x = cx + h; p[0].y = cy - hw;
        p[1].x = cx;     p[1].y = cy;
        p[2].x = cx + h; p[2].y = cy + hw;
    } else if (s->i0 == GSV89_DIR_RIGHT) {
        p[0].x = cx - h; p[0].y = cy - hw;
        p[1].x = cx;     p[1].y = cy;
        p[2].x = cx - h; p[2].y = cy + hw;
    } else {
        p[0].x = cx - hw; p[0].y = cy + h;
        p[1].x = cx;      p[1].y = cy;
        p[2].x = cx + hw; p[2].y = cy + h;
    }
    if (s->flags & GSV89_FLAG_FILLED) {
        gsp89_paint_triangle(painter,
                             p[0].x, p[0].y,
                             p[1].x, p[1].y,
                             p[2].x, p[2].y,
                             style);
    } else {
        gsp89_paint_line(painter, p[0].x, p[0].y, p[1].x, p[1].y, style);
        gsp89_paint_line(painter, p[1].x, p[1].y, p[2].x, p[2].y, style);
    }
}

static void gsv89_emit_diamond(gsp89_painter *painter,
                               const gsv89_shape *s,
                               const gsp89_style *style)
{
    gsv89_point p[4];
    p[0].x = s->x0;        p[0].y = s->y0 - s->b;
    p[1].x = s->x0 + s->a; p[1].y = s->y0;
    p[2].x = s->x0;        p[2].y = s->y0 + s->b;
    p[3].x = s->x0 - s->a; p[3].y = s->y0;
    gsv89_emit_polyline(painter, p, 4, 1,
                        (short)((s->flags & GSV89_FLAG_FILLED) != 0), style);
}

static void gsv89_emit_bracket(gsp89_painter *painter,
                               const gsv89_shape *s,
                               const gsp89_style *style)
{
    gsp89_fx x;
    gsp89_fx y0;
    gsp89_fx y1;
    gsp89_fx arm;
    short side;
    x = s->x0;
    y0 = s->y0 - s->a;
    y1 = s->y0 + s->a;
    arm = s->b;
    side = s->i0;
    gsp89_paint_line(painter, x, y0, x, y1, style);
    if (side < 0) {
        gsp89_paint_line(painter, x, y0, x + arm, y0, style);
        gsp89_paint_line(painter, x, y1, x + arm, y1, style);
    } else {
        gsp89_paint_line(painter, x - arm, y0, x, y0, style);
        gsp89_paint_line(painter, x - arm, y1, x, y1, style);
    }
}

static void gsv89_emit_regular_polygon(gsp89_painter *painter,
                                       const gsv89_shape *s,
                                       const gsp89_style *style)
{
    gsv89_point p[GSV89_MAX_REGULAR_SIDES];
    short sides;
    short i;
    short angle;
    gsp89_fx cs;
    gsp89_fx sn;
    sides = (short)GSV89_CLAMP(s->i0, 3, GSV89_MAX_REGULAR_SIDES);
    for (i = 0; i < sides; ++i) {
        angle = (short)(s->i1 + (short)(((long)i * 36000L) / (long)sides));
        cs = gsv89_cos_fx(angle);
        sn = gsv89_sin_fx(angle);
        p[i].x = s->x0 + gsp89_fx_mul(s->a, cs);
        p[i].y = s->y0 + gsp89_fx_mul(s->a, sn);
    }
    gsv89_emit_polyline(painter, p, sides, 1,
                        (short)((s->flags & GSV89_FLAG_FILLED) != 0), style);
}

static void gsv89_emit_grid(gsp89_painter *painter,
                            const gsv89_shape *s,
                            const gsp89_style *style)
{
    short i;
    short cols;
    short rows;
    gsp89_fx x;
    gsp89_fx y;
    cols = s->i0 < 1 ? 1 : s->i0;
    rows = s->i1 < 1 ? 1 : s->i1;
    for (i = 0; i <= cols; ++i) {
        x = s->x0 + (gsp89_fx)(((s->x1 - s->x0) * (long)i) / (long)cols);
        gsp89_paint_line(painter, x, s->y0, x, s->y1, style);
    }
    for (i = 0; i <= rows; ++i) {
        y = s->y0 + (gsp89_fx)(((s->y1 - s->y0) * (long)i) / (long)rows);
        gsp89_paint_line(painter, s->x0, y, s->x1, y, style);
    }
}

static void gsv89_emit_tick_strip(gsp89_painter *painter,
                                  const gsv89_shape *s,
                                  const gsp89_style *style)
{
    short i;
    short count;
    short major;
    gsp89_fx x;
    gsp89_fx y;
    gsp89_fx half;
    count = s->i0 < 1 ? 1 : s->i0;
    major = s->i1 < 1 ? 0 : s->i1;
    for (i = 0; i < count; ++i) {
        short t;
        t = count == 1 ? 0 : (short)(((long)i * 1000L) / (long)(count - 1));
        x = gsv89_lerp(s->x0, s->x1, t);
        y = gsv89_lerp(s->y0, s->y1, t);
        half = s->a;
        if (major > 0 && (i % major) == 0) half = half + half / 2;
        if (s->i2 == GSV89_AXIS_VERTICAL) {
            gsp89_paint_line(painter, x - half, y, x + half, y, style);
        } else {
            gsp89_paint_line(painter, x, y - half, x, y + half, style);
        }
    }
}

static void gsv89_emit_bezier(gsp89_painter *painter,
                              const gsv89_shape *s,
                              const gsp89_style *style)
{
    short i;
    short seg;
    short t_x1000;
    gsp89_fx px;
    gsp89_fx py;
    gsp89_fx ax;
    gsp89_fx ay;
    gsp89_fx bx;
    gsp89_fx by;
    gsp89_fx nx;
    gsp89_fx ny;
    seg = (short)GSV89_CLAMP(s->i0, 2, 64);
    px = s->x0;
    py = s->y0;
    for (i = 1; i <= seg; ++i) {
        t_x1000 = (short)(((long)i * 1000L) / (long)seg);
        ax = gsv89_lerp(s->x0, s->x1, t_x1000);
        ay = gsv89_lerp(s->y0, s->y1, t_x1000);
        bx = gsv89_lerp(s->x1, s->x2, t_x1000);
        by = gsv89_lerp(s->y1, s->y2, t_x1000);
        nx = gsv89_lerp(ax, bx, t_x1000);
        ny = gsv89_lerp(ay, by, t_x1000);
        gsp89_paint_line(painter, px, py, nx, ny, style);
        px = nx;
        py = ny;
    }
}


#define GSV89_SEG_A  1
#define GSV89_SEG_B  2
#define GSV89_SEG_C  4
#define GSV89_SEG_D  8
#define GSV89_SEG_E  16
#define GSV89_SEG_F  32
#define GSV89_SEG_G  64

static void gsv89_emit_stroke_glyph(gsp89_painter *painter,
                                    short glyph_id,
                                    gsp89_fx cx,
                                    gsp89_fx cy,
                                    gsp89_fx size_x,
                                    gsp89_fx size_y,
                                    const gsp89_style *style)
{
    short bits;
    gsp89_fx left;
    gsp89_fx right;
    gsp89_fx top;
    gsp89_fx middle;
    gsp89_fx bottom;
    gsp89_fx dot_radius;

    left = cx - size_x / 2L;
    right = cx + size_x / 2L;
    top = cy - size_y / 2L;
    middle = cy;
    bottom = cy + size_y / 2L;
    bits = 0;

    switch (glyph_id) {
        case '0': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_C |
                         GSV89_SEG_D | GSV89_SEG_E | GSV89_SEG_F; break;
        case '1': bits = GSV89_SEG_B | GSV89_SEG_C; break;
        case '2': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_G |
                         GSV89_SEG_E | GSV89_SEG_D; break;
        case '3': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_G |
                         GSV89_SEG_C | GSV89_SEG_D; break;
        case '4': bits = GSV89_SEG_F | GSV89_SEG_G |
                         GSV89_SEG_B | GSV89_SEG_C; break;
        case '5': bits = GSV89_SEG_A | GSV89_SEG_F | GSV89_SEG_G |
                         GSV89_SEG_C | GSV89_SEG_D; break;
        case '6': bits = GSV89_SEG_A | GSV89_SEG_F | GSV89_SEG_G |
                         GSV89_SEG_E | GSV89_SEG_C | GSV89_SEG_D; break;
        case '7': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_C; break;
        case '8': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_C |
                         GSV89_SEG_D | GSV89_SEG_E | GSV89_SEG_F |
                         GSV89_SEG_G; break;
        case '9': bits = GSV89_SEG_A | GSV89_SEG_B | GSV89_SEG_C |
                         GSV89_SEG_D | GSV89_SEG_F | GSV89_SEG_G; break;
        case '-':
            gsp89_paint_line(painter, left, middle, right, middle, style);
            return;
        case '+':
            gsp89_paint_line(painter, left, middle, right, middle, style);
            gsp89_paint_line(painter, cx, top, cx, bottom, style);
            return;
        case '.':
            dot_radius = size_x < size_y ? size_x / 8L : size_y / 8L;
            if (dot_radius < 1L) dot_radius = 1L;
            gsp89_paint_circle(painter, cx, bottom, dot_radius, style);
            return;
        case 'U':
        case 'u':
            gsp89_paint_line(painter, left, top, left, bottom, style);
            gsp89_paint_line(painter, right, top, right, bottom, style);
            gsp89_paint_line(painter, left, bottom, right, bottom, style);
            return;
        case 'L':
        case 'l':
            gsp89_paint_line(painter, left, top, left, bottom, style);
            gsp89_paint_line(painter, left, bottom, right, bottom, style);
            return;
        case 'R':
        case 'r':
            gsp89_paint_line(painter, left, top, left, bottom, style);
            gsp89_paint_line(painter, left, top, right, top, style);
            gsp89_paint_line(painter, right, top, right, middle, style);
            gsp89_paint_line(painter, left, middle, right, middle, style);
            gsp89_paint_line(painter, cx, middle, right, bottom, style);
            return;
        default:
            gsp89_paint_line(painter, left, top, right, top, style);
            gsp89_paint_line(painter, right, top, right, bottom, style);
            gsp89_paint_line(painter, right, bottom, left, bottom, style);
            gsp89_paint_line(painter, left, bottom, left, top, style);
            return;
    }

    if (bits & GSV89_SEG_A) gsp89_paint_line(painter, left, top, right, top, style);
    if (bits & GSV89_SEG_B) gsp89_paint_line(painter, right, top, right, middle, style);
    if (bits & GSV89_SEG_C) gsp89_paint_line(painter, right, middle, right, bottom, style);
    if (bits & GSV89_SEG_D) gsp89_paint_line(painter, left, bottom, right, bottom, style);
    if (bits & GSV89_SEG_E) gsp89_paint_line(painter, left, middle, left, bottom, style);
    if (bits & GSV89_SEG_F) gsp89_paint_line(painter, left, top, left, middle, style);
    if (bits & GSV89_SEG_G) gsp89_paint_line(painter, left, middle, right, middle, style);
}

void gsv89_emit_shape(gsp89_painter *painter,
                       const gsv89_shape *s,
                       const gsv89_palette *palette,
                       short global_alpha)
{
    gsp89_style style;
    if (!painter || !s) return;
    if (!gsv89_part_visible(palette, s->part_id)) return;
    if (!(s->flags & GSV89_FLAG_VISIBLE)) return;
    style = gsv89_style_for(palette, s, global_alpha);

    switch (s->kind) {
        case GSV89_SHAPE_LINE:
        case GSV89_SHAPE_HLINE:
        case GSV89_SHAPE_VLINE:
            gsp89_paint_line(painter, s->x0, s->y0, s->x1, s->y1, &style);
            break;
        case GSV89_SHAPE_RECT:
        case GSV89_SHAPE_SQUARE:
            gsp89_paint_rect(painter, s->x0, s->y0, s->x1, s->y1, &style);
            break;
        case GSV89_SHAPE_TRIANGLE:
            if (s->flags & GSV89_FLAG_FILLED) {
                gsp89_paint_triangle(painter, s->x0, s->y0,
                                     s->x1, s->y1, s->x2, s->y2, &style);
            } else {
                gsp89_paint_line(painter, s->x0, s->y0, s->x1, s->y1, &style);
                gsp89_paint_line(painter, s->x1, s->y1, s->x2, s->y2, &style);
                gsp89_paint_line(painter, s->x2, s->y2, s->x0, s->y0, &style);
            }
            break;
        case GSV89_SHAPE_CIRCLE:
        case GSV89_SHAPE_DOT:
            gsp89_paint_circle(painter, s->x0, s->y0, s->a, &style);
            break;
        case GSV89_SHAPE_ELLIPSE:
            gsp89_paint_ellipse(painter, s->x0, s->y0, s->a, s->b, &style);
            break;
        case GSV89_SHAPE_ARC:
            gsp89_paint_arc(painter, s->x0, s->y0, s->a, s->b,
                            s->i0, s->i1, &style);
            break;
        case GSV89_SHAPE_CROSS:
            gsv89_emit_cross(painter, s, &style);
            break;
        case GSV89_SHAPE_XCROSS:
            gsv89_emit_xcross(painter, s, &style);
            break;
        case GSV89_SHAPE_CHEVRON:
            gsv89_emit_chevron(painter, s, &style);
            break;
        case GSV89_SHAPE_DIAMOND:
            gsv89_emit_diamond(painter, s, &style);
            break;
        case GSV89_SHAPE_PARENTHESIS:
            if (s->i0 < 0) {
                gsp89_paint_arc(painter, s->x0, s->y0, s->a, s->b,
                                9000, 27000, &style);
            } else {
                gsp89_paint_arc(painter, s->x0, s->y0, s->a, s->b,
                                -9000, 9000, &style);
            }
            break;
        case GSV89_SHAPE_BRACKET:
            gsv89_emit_bracket(painter, s, &style);
            break;
        case GSV89_SHAPE_HORSESHOE:
            gsp89_paint_arc(painter, s->x0, s->y0, s->a, s->b,
                            (short)(-9000 + s->i0 / 2),
                            (short)(27000 - s->i0 / 2), &style);
            break;
        case GSV89_SHAPE_REGULAR_POLYGON:
            gsv89_emit_regular_polygon(painter, s, &style);
            break;
        case GSV89_SHAPE_PATH:
            gsv89_emit_polyline(painter, s->points, s->point_count,
                                (short)((s->flags & GSV89_FLAG_CLOSED) != 0),
                                (short)((s->flags & GSV89_FLAG_FILLED) != 0), &style);
            break;
        case GSV89_SHAPE_GRID:
            gsv89_emit_grid(painter, s, &style);
            break;
        case GSV89_SHAPE_TICK_STRIP:
            gsv89_emit_tick_strip(painter, s, &style);
            break;
        case GSV89_SHAPE_BEZIER_QUAD:
            gsv89_emit_bezier(painter, s, &style);
            break;
        case GSV89_SHAPE_GLYPH:
            gsv89_emit_stroke_glyph(painter, s->i0, s->x0, s->y0,
                                    s->a, s->b, &style);
            break;
        case GSV89_SHAPE_SCOPE_MASK:
            gsp89_paint_scope_mask(painter, s->x0, s->y0, s->a, &style);
            break;
        case GSV89_SHAPE_VIGNETTE:
            gsp89_paint_vignette(painter, s->a, &style);
            break;
        default:
            break;
    }
}

void gsv89_emit_shapes(gsp89_painter *painter,
                        const gsv89_shape *shapes,
                        short shape_count,
                        const gsv89_palette *palette,
                        short global_alpha)
{
    short i;
    if (!painter || !shapes || shape_count < 1) return;
    for (i = 0; i < shape_count; ++i) {
        gsv89_emit_shape(painter, &shapes[i], palette, global_alpha);
    }
}
