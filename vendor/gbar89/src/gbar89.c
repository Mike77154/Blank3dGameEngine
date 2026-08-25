#include "gbar89.h"

static const int gbar89_sin_lut_0_90[91] = {
    0, 18, 36, 54, 71, 89, 107, 125, 143, 160, 178, 195, 213,
    230, 248, 265, 282, 299, 316, 333, 350, 367, 384, 400, 416, 433,
    449, 465, 481, 496, 512, 527, 543, 558, 573, 587, 602, 616, 630,
    644, 658, 672, 685, 698, 711, 724, 737, 749, 761, 773, 784, 796,
    807, 818, 828, 839, 849, 859, 868, 878, 887, 896, 904, 912, 920,
    928, 935, 943, 949, 956, 962, 968, 974, 979, 984, 989, 994, 998,
    1002, 1005, 1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024, 1024
};

static int gbar89_abs_int(int v)
{
    if (v < 0) {
        return -v;
    }
    return v;
}

static int gbar89_mod360(int deg)
{
    int r;

    r = deg % 360;
    if (r < 0) {
        r += 360;
    }
    return r;
}

int gbar89_sin_deg(int deg)
{
    int d;

    d = gbar89_mod360(deg);

    if (d <= 90) {
        return gbar89_sin_lut_0_90[d];
    }
    if (d <= 180) {
        return gbar89_sin_lut_0_90[180 - d];
    }
    if (d <= 270) {
        return -gbar89_sin_lut_0_90[d - 180];
    }
    return -gbar89_sin_lut_0_90[360 - d];
}

int gbar89_cos_deg(int deg)
{
    return gbar89_sin_deg(deg + 90);
}

GBar89_Fix gbar89_fix_from_int(int v)
{
    return ((GBar89_Fix)v) << GBAR89_FIX_SHIFT;
}

int gbar89_fix_to_int(GBar89_Fix v)
{
    if (v >= 0) {
        return (int)(v >> GBAR89_FIX_SHIFT);
    }
    return (int)(-((-v) >> GBAR89_FIX_SHIFT));
}

GBar89_Fix gbar89_fix_mul(GBar89_Fix a, GBar89_Fix b)
{
    return (GBar89_Fix)((a * b) >> GBAR89_FIX_SHIFT);
}

GBar89_Fix gbar89_fix_div(GBar89_Fix a, GBar89_Fix b)
{
    if (b == 0) {
        return 0;
    }
    return (GBar89_Fix)((a << GBAR89_FIX_SHIFT) / b);
}

GBar89_Fix gbar89_fix_clamp01(GBar89_Fix v)
{
    if (v < 0) {
        return 0;
    }
    if (v > GBAR89_FIX_ONE) {
        return GBAR89_FIX_ONE;
    }
    return v;
}

long gbar89_lerp_long(long a, long b, GBar89_Fix t)
{
    long d;

    t = gbar89_fix_clamp01(t);
    d = b - a;
    return a + (long)((d * t) >> GBAR89_FIX_SHIFT);
}

GBar89_Fix gbar89_ease(int ease_kind, GBar89_Fix t)
{
    GBar89_Fix u;
    GBar89_Fix a;
    GBar89_Fix b;

    t = gbar89_fix_clamp01(t);
    u = GBAR89_FIX_ONE - t;

    if (ease_kind == GBAR89_EASE_IN_QUAD) {
        return gbar89_fix_mul(t, t);
    }
    if (ease_kind == GBAR89_EASE_OUT_QUAD) {
        return GBAR89_FIX_ONE - gbar89_fix_mul(u, u);
    }
    if (ease_kind == GBAR89_EASE_IN_OUT_QUAD) {
        if (t < GBAR89_FIX_HALF) {
            return gbar89_fix_mul(t, t) << 1;
        }
        return GBAR89_FIX_ONE - (gbar89_fix_mul(u, u) << 1);
    }
    if (ease_kind == GBAR89_EASE_SMOOTHSTEP) {
        a = gbar89_fix_mul(t, t);
        b = gbar89_fix_from_int(3) - (t << 1);
        return gbar89_fix_mul(a, b);
    }
    if (ease_kind == GBAR89_EASE_OUT_CUBIC) {
        return GBAR89_FIX_ONE - gbar89_fix_mul(gbar89_fix_mul(u, u), u);
    }
    return t;
}

static GBar89_Rect gbar89_rect_make(int x, int y, int w, int h)
{
    GBar89_Rect r;

    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

static int gbar89_rect_valid(const GBar89_Rect *r)
{
    if (r == 0) {
        return 0;
    }
    if (r->w <= 0 || r->h <= 0) {
        return 0;
    }
    return 1;
}

static GBar89_Rect gbar89_rect_intersect(const GBar89_Rect *a,
                                         const GBar89_Rect *b)
{
    int x1;
    int y1;
    int x2;
    int y2;
    GBar89_Rect r;

    x1 = a->x > b->x ? a->x : b->x;
    y1 = a->y > b->y ? a->y : b->y;
    x2 = (a->x + a->w) < (b->x + b->w) ? (a->x + a->w) : (b->x + b->w);
    y2 = (a->y + a->h) < (b->y + b->h) ? (a->y + a->h) : (b->y + b->h);

    r.x = x1;
    r.y = y1;
    r.w = x2 - x1;
    r.h = y2 - y1;
    if (r.w < 0) {
        r.w = 0;
    }
    if (r.h < 0) {
        r.h = 0;
    }
    return r;
}

static GBar89_Rect gbar89_inner_rect(const GBar89_Meter *m)
{
    GBar89_Rect r;

    r = m->rect;
    r.x += m->style.padding_left;
    r.y += m->style.padding_top;
    r.w -= m->style.padding_left + m->style.padding_right;
    r.h -= m->style.padding_top + m->style.padding_bottom;

    if (r.w < 0) {
        r.w = 0;
    }
    if (r.h < 0) {
        r.h = 0;
    }

    return r;
}

static void gbar89_call_rect(const GBar89_RenderOps *ops,
                             const GBar89_Rect *r,
                             unsigned long rgba)
{
    if (ops == 0 || r == 0) {
        return;
    }
    if (ops->draw_rect == 0) {
        return;
    }
    if (r->w <= 0 || r->h <= 0) {
        return;
    }
    ops->draw_rect(ops->user, r->x, r->y, r->w, r->h, rgba);
}

static void gbar89_call_line(const GBar89_RenderOps *ops,
                             int x1, int y1, int x2, int y2,
                             unsigned long rgba)
{
    if (ops == 0) {
        return;
    }
    if (ops->draw_line == 0) {
        return;
    }
    ops->draw_line(ops->user, x1, y1, x2, y2, rgba);
}

static void gbar89_call_sprite(const GBar89_RenderOps *ops, int sprite_id,
                               const GBar89_Rect *src,
                               const GBar89_Rect *dst,
                               unsigned long tint)
{
    if (ops == 0 || src == 0 || dst == 0) {
        return;
    }
    if (ops->draw_sprite == 0) {
        return;
    }
    if (sprite_id == GBAR89_INVALID_SPRITE) {
        return;
    }
    if (src->w <= 0 || src->h <= 0 || dst->w <= 0 || dst->h <= 0) {
        return;
    }
    ops->draw_sprite(ops->user, sprite_id,
                     src->x, src->y, src->w, src->h,
                     dst->x, dst->y, dst->w, dst->h,
                     tint);
}

static void gbar89_draw_rect_frame(const GBar89_RenderOps *ops,
                                    const GBar89_Rect *r,
                                    int size,
                                    unsigned long color)
{
    GBar89_Rect q;

    if (ops == 0 || r == 0) {
        return;
    }
    if (size < 1) {
        size = 1;
    }

    q = gbar89_rect_make(r->x, r->y, r->w, size);
    gbar89_call_rect(ops, &q, color);
    q = gbar89_rect_make(r->x, r->y + r->h - size, r->w, size);
    gbar89_call_rect(ops, &q, color);
    q = gbar89_rect_make(r->x, r->y, size, r->h);
    gbar89_call_rect(ops, &q, color);
    q = gbar89_rect_make(r->x + r->w - size, r->y, size, r->h);
    gbar89_call_rect(ops, &q, color);
}

static void gbar89_draw_pre_fx(const GBar89_Meter *m,
                               const GBar89_RenderOps *ops)
{
    GBar89_Rect r;
    int i;
    int ox;
    int oy;
    int d;

    if (m == 0 || ops == 0) {
        return;
    }

    ox = m->style.shadow_offset_x;
    oy = m->style.shadow_offset_y;

    if ((m->style.fx_flags & GBAR89_FX_DROP_SHADOW) != 0) {
        r = m->rect;
        r.x += ox;
        r.y += oy;
        gbar89_call_rect(ops, &r, m->style.color_shadow);
    }

    if ((m->style.fx_flags & GBAR89_FX_EXTRUDE) != 0) {
        d = m->style.extrude_depth;
        if (d < 1) {
            d = 1;
        }
        for (i = d; i >= 1; --i) {
            r = m->rect;
            r.x += (ox < 0) ? -i : i;
            r.y += (oy < 0) ? -i : i;
            gbar89_call_rect(ops, &r, m->style.color_extrude);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_OUTER_OUTLINE) != 0 &&
        m->style.outline_size > 0) {
        i = m->style.outline_size;
        r.x = m->rect.x - i;
        r.y = m->rect.y - i;
        r.w = m->rect.w + i * 2;
        r.h = m->rect.h + i * 2;
        gbar89_call_rect(ops, &r, m->style.color_outline);
    }
}

static void gbar89_draw_border(const GBar89_Meter *m,
                               const GBar89_RenderOps *ops)
{
    int b;
    int gap;
    int c;
    GBar89_Rect r;
    GBar89_Rect q;
    unsigned long color;
    unsigned long top_color;
    unsigned long bottom_color;

    if (m == 0 || ops == 0) {
        return;
    }
    if (m->style.frame_kind == GBAR89_FRAME_NONE) {
        return;
    }

    b = m->style.frame_depth;
    if (b <= 0) {
        b = m->style.border_size;
    }
    if (b <= 0) {
        b = 1;
    }
    gap = m->style.frame_gap;
    if (gap < 0) {
        gap = 0;
    }
    c = m->style.frame_corner;
    if (c < b * 2) {
        c = b * 2;
    }
    if (c > m->rect.w / 2) {
        c = m->rect.w / 2;
    }
    if (c > m->rect.h / 2) {
        c = m->rect.h / 2;
    }

    color = m->style.color_border;
    if (m->state >= 0 && m->state < GBAR89_STATE_COUNT) {
        if (m->style.color_state_border[m->state] != 0UL) {
            color = m->style.color_state_border[m->state];
        }
    }

    r = m->rect;

    if (m->style.frame_kind == GBAR89_FRAME_DOUBLE) {
        gbar89_draw_rect_frame(ops, &r, b, color);
        q.x = r.x + b + gap;
        q.y = r.y + b + gap;
        q.w = r.w - (b + gap) * 2;
        q.h = r.h - (b + gap) * 2;
        if (q.w > 0 && q.h > 0) {
            gbar89_draw_rect_frame(ops, &q, 1, m->style.color_highlight);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_BEVEL_OUT ||
               m->style.frame_kind == GBAR89_FRAME_BEVEL_IN) {
        top_color = m->style.color_highlight;
        bottom_color = m->style.color_shadow;
        if (m->style.frame_kind == GBAR89_FRAME_BEVEL_IN) {
            top_color = m->style.color_shadow;
            bottom_color = m->style.color_highlight;
        }
        q = gbar89_rect_make(r.x, r.y, r.w, b);
        gbar89_call_rect(ops, &q, top_color);
        q = gbar89_rect_make(r.x, r.y, b, r.h);
        gbar89_call_rect(ops, &q, top_color);
        q = gbar89_rect_make(r.x, r.y + r.h - b, r.w, b);
        gbar89_call_rect(ops, &q, bottom_color);
        q = gbar89_rect_make(r.x + r.w - b, r.y, b, r.h);
        gbar89_call_rect(ops, &q, bottom_color);
        if (gap > 0) {
            q.x = r.x + b;
            q.y = r.y + b;
            q.w = r.w - b * 2;
            q.h = r.h - b * 2;
            if (q.w > 0 && q.h > 0) {
                gbar89_draw_rect_frame(ops, &q, 1, color);
            }
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_BRACKETS) {
        q = gbar89_rect_make(r.x, r.y, c, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x, r.y, b, c);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + r.w - c, r.y, c, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + r.w - b, r.y, b, c);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x, r.y + r.h - b, c, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x, r.y + r.h - c, b, c);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + r.w - c, r.y + r.h - b, c, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + r.w - b, r.y + r.h - c, b, c);
        gbar89_call_rect(ops, &q, color);
    } else if (m->style.frame_kind == GBAR89_FRAME_RAIL) {
        q = gbar89_rect_make(r.x, r.y, r.w, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x, r.y + r.h - b, r.w, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x - b, r.y - b, b * 2, r.h + b * 2);
        gbar89_call_rect(ops, &q, m->style.color_shadow);
        q = gbar89_rect_make(r.x + r.w - b, r.y - b, b * 2, r.h + b * 2);
        gbar89_call_rect(ops, &q, m->style.color_highlight);
    } else if (m->style.frame_kind == GBAR89_FRAME_PIXEL) {
        q = gbar89_rect_make(r.x + b, r.y, r.w - b * 2, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + b, r.y + r.h - b, r.w - b * 2, b);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x, r.y + b, b, r.h - b * 2);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + r.w - b, r.y + b, b, r.h - b * 2);
        gbar89_call_rect(ops, &q, color);
        q = gbar89_rect_make(r.x + b, r.y + b, b, b);
        gbar89_call_rect(ops, &q, m->style.color_highlight);
        q = gbar89_rect_make(r.x + r.w - b * 2,
                             r.y + r.h - b * 2, b, b);
        gbar89_call_rect(ops, &q, m->style.color_shadow);
    } else {
        gbar89_draw_rect_frame(ops, &r, b, color);
    }
}

static GBar89_Command *gbar89_command_push(GBar89_CommandBuffer *buffer,
                                           int type)
{
    GBar89_Command *cmd;

    if (buffer == 0) {
        return 0;
    }
    if (buffer->command_count >= GBAR89_MAX_COMMANDS) {
        buffer->overflow = 1;
        return 0;
    }

    cmd = &buffer->commands[buffer->command_count];
    buffer->command_count += 1;

    cmd->type = type;
    cmd->sprite_id = GBAR89_INVALID_SPRITE;
    cmd->color = 0UL;
    cmd->src = gbar89_rect_make(0, 0, 0, 0);
    cmd->dst = gbar89_rect_make(0, 0, 0, 0);
    cmd->x1 = 0;
    cmd->y1 = 0;
    cmd->x2 = 0;
    cmd->y2 = 0;
    cmd->vertex_first = 0;
    cmd->vertex_count = 0;
    cmd->index_first = 0;
    cmd->index_count = 0;
    return cmd;
}

static void gbar89_cb_rect(void *user, int x, int y, int w, int h,
                           unsigned long rgba)
{
    GBar89_CommandBuffer *buffer;
    GBar89_Command *cmd;

    buffer = (GBar89_CommandBuffer *)user;
    cmd = gbar89_command_push(buffer, GBAR89_CMD_RECT);
    if (cmd == 0) {
        return;
    }
    cmd->dst = gbar89_rect_make(x, y, w, h);
    cmd->color = rgba;
}

static void gbar89_cb_line(void *user, int x1, int y1, int x2, int y2,
                           unsigned long rgba)
{
    GBar89_CommandBuffer *buffer;
    GBar89_Command *cmd;

    buffer = (GBar89_CommandBuffer *)user;
    cmd = gbar89_command_push(buffer, GBAR89_CMD_LINE);
    if (cmd == 0) {
        return;
    }
    cmd->x1 = x1;
    cmd->y1 = y1;
    cmd->x2 = x2;
    cmd->y2 = y2;
    cmd->color = rgba;
}

static void gbar89_cb_sprite(void *user, int sprite_id,
                             int sx, int sy, int sw, int sh,
                             int dx, int dy, int dw, int dh,
                             unsigned long tint_rgba)
{
    GBar89_CommandBuffer *buffer;
    GBar89_Command *cmd;

    buffer = (GBar89_CommandBuffer *)user;
    cmd = gbar89_command_push(buffer, GBAR89_CMD_SPRITE);
    if (cmd == 0) {
        return;
    }
    cmd->sprite_id = sprite_id;
    cmd->src = gbar89_rect_make(sx, sy, sw, sh);
    cmd->dst = gbar89_rect_make(dx, dy, dw, dh);
    cmd->color = tint_rgba;
}

static void gbar89_cb_triangles(void *user,
                                const GBar89_Vertex *vertices, int vertex_count,
                                const int *indices, int index_count,
                                int sprite_id)
{
    GBar89_CommandBuffer *buffer;
    GBar89_Command *cmd;
    int i;

    buffer = (GBar89_CommandBuffer *)user;
    if (buffer == 0) {
        return;
    }
    if (vertex_count < 0 || index_count < 0) {
        buffer->overflow = 1;
        return;
    }
    if (buffer->vertex_count + vertex_count > GBAR89_MAX_COMMAND_VERTICES ||
        buffer->index_count + index_count > GBAR89_MAX_COMMAND_INDICES) {
        buffer->overflow = 1;
        return;
    }

    cmd = gbar89_command_push(buffer, GBAR89_CMD_TRIANGLES);
    if (cmd == 0) {
        return;
    }

    cmd->sprite_id = sprite_id;
    cmd->vertex_first = buffer->vertex_count;
    cmd->vertex_count = vertex_count;
    cmd->index_first = buffer->index_count;
    cmd->index_count = index_count;

    for (i = 0; i < vertex_count; ++i) {
        buffer->vertices[buffer->vertex_count + i] = vertices[i];
    }
    for (i = 0; i < index_count; ++i) {
        buffer->indices[buffer->index_count + i] = indices[i];
    }
    buffer->vertex_count += vertex_count;
    buffer->index_count += index_count;
}

static void gbar89_cb_push_clip(void *user, int x, int y, int w, int h)
{
    GBar89_CommandBuffer *buffer;
    GBar89_Command *cmd;

    buffer = (GBar89_CommandBuffer *)user;
    cmd = gbar89_command_push(buffer, GBAR89_CMD_PUSH_CLIP);
    if (cmd == 0) {
        return;
    }
    cmd->dst = gbar89_rect_make(x, y, w, h);
}

static void gbar89_cb_pop_clip(void *user)
{
    GBar89_CommandBuffer *buffer;

    buffer = (GBar89_CommandBuffer *)user;
    (void)gbar89_command_push(buffer, GBAR89_CMD_POP_CLIP);
}

void gbar89_command_buffer_init(GBar89_CommandBuffer *buffer)
{
    if (buffer == 0) {
        return;
    }
    buffer->command_count = 0;
    buffer->vertex_count = 0;
    buffer->index_count = 0;
    buffer->overflow = 0;
}

GBar89_RenderOps gbar89_command_buffer_make_ops(GBar89_CommandBuffer *buffer)
{
    GBar89_RenderOps ops;

    ops.user = buffer;
    ops.draw_rect = gbar89_cb_rect;
    ops.draw_line = gbar89_cb_line;
    ops.draw_sprite = gbar89_cb_sprite;
    ops.draw_triangles = gbar89_cb_triangles;
    ops.push_clip = gbar89_cb_push_clip;
    ops.pop_clip = gbar89_cb_pop_clip;
    return ops;
}

void gbar89_command_buffer_replay(const GBar89_CommandBuffer *buffer,
                                  const GBar89_RenderOps *ops)
{
    int i;
    const GBar89_Command *cmd;

    if (buffer == 0 || ops == 0) {
        return;
    }

    for (i = 0; i < buffer->command_count; ++i) {
        cmd = &buffer->commands[i];
        if (cmd->type == GBAR89_CMD_RECT) {
            gbar89_call_rect(ops, &cmd->dst, cmd->color);
        } else if (cmd->type == GBAR89_CMD_LINE) {
            gbar89_call_line(ops, cmd->x1, cmd->y1, cmd->x2, cmd->y2,
                             cmd->color);
        } else if (cmd->type == GBAR89_CMD_SPRITE) {
            gbar89_call_sprite(ops, cmd->sprite_id, &cmd->src, &cmd->dst,
                               cmd->color);
        } else if (cmd->type == GBAR89_CMD_TRIANGLES) {
            if (ops->draw_triangles != 0) {
                ops->draw_triangles(ops->user,
                                    &buffer->vertices[cmd->vertex_first],
                                    cmd->vertex_count,
                                    &buffer->indices[cmd->index_first],
                                    cmd->index_count,
                                    cmd->sprite_id);
            }
        } else if (cmd->type == GBAR89_CMD_PUSH_CLIP) {
            if (ops->push_clip != 0) {
                ops->push_clip(ops->user, cmd->dst.x, cmd->dst.y,
                               cmd->dst.w, cmd->dst.h);
            }
        } else if (cmd->type == GBAR89_CMD_POP_CLIP) {
            if (ops->pop_clip != 0) {
                ops->pop_clip(ops->user);
            }
        }
    }
}

int gbar89_command_buffer_overflowed(const GBar89_CommandBuffer *buffer)
{
    if (buffer == 0) {
        return 0;
    }
    return buffer->overflow;
}

void gbar89_style_default(GBar89_Style *style)
{
    int i;

    if (style == 0) {
        return;
    }

    style->color_bg = 0x202020FFUL;
    style->color_fill = 0x40C060FFUL;
    style->color_lag = 0xE0B040FFUL;
    style->color_border = 0xFFFFFFFFUL;
    style->color_overlay = 0x40A0FFFFUL;
    style->color_empty = 0x303030FFUL;
    style->color_marker = 0xFFFFFFFFUL;
    style->color_pattern = 0x101010FFUL;
    style->color_mid = 0x80D8FFFFUL;
    style->color_outline = 0x080A10FFUL;
    style->color_highlight = 0xFFFFFFFFUL;
    style->color_shadow = 0x101820FFUL;
    style->color_extrude = 0x080C14FFUL;
    style->color_gloss = 0xFFFFFF50UL;
    style->color_bg_detail = 0x40506080UL;
    style->color_unit_fill = 0xFFFFFFFFUL;
    style->color_unit_empty = 0x405060FFUL;
    style->color_unit_outline = 0x080A10FFUL;

    for (i = 0; i < GBAR89_STATE_COUNT; ++i) {
        style->color_state_fill[i] = 0UL;
        style->color_state_border[i] = 0UL;
    }
    style->color_state_fill[GBAR89_STATE_LOW] = 0xE0D040FFUL;
    style->color_state_fill[GBAR89_STATE_CRITICAL] = 0xD03030FFUL;
    style->color_state_fill[GBAR89_STATE_POISONED] = 0xB040D0FFUL;
    style->color_state_fill[GBAR89_STATE_REGENERATING] = 0x60E080FFUL;
    style->color_state_fill[GBAR89_STATE_SHIELDED] = 0x40A0FFFFUL;
    style->color_state_fill[GBAR89_STATE_OVERHEAT] = 0xE07020FFUL;
    style->color_state_fill[GBAR89_STATE_LOCKED] = 0x808080FFUL;
    style->color_state_fill[GBAR89_STATE_BROKEN] = 0x602020FFUL;
    style->color_state_fill[GBAR89_STATE_CUSTOM] = 0xFFFFFFFFUL;

    for (i = 0; i < GBAR89_MAX_LAYERS; ++i) {
        style->color_layer_fill[i] = 0UL;
        style->color_layer_lag[i] = 0UL;
    }
    style->color_layer_fill[0] = 0x40C060FFUL;
    style->color_layer_fill[1] = 0x40A0FFFFUL;
    style->color_layer_fill[2] = 0xE0D040FFUL;
    style->color_layer_fill[3] = 0xE07020FFUL;
    style->color_layer_fill[4] = 0xD050D0FFUL;
    style->color_layer_fill[5] = 0xF0F0F0FFUL;
    style->color_layer_fill[6] = 0x90D0FFFFUL;
    style->color_layer_fill[7] = 0xFF9090FFUL;

    for (i = 0; i < GBAR89_MAX_LAYERS; ++i) {
        style->color_layer_lag[i] = style->color_lag;
    }

    style->sprite_bg = GBAR89_INVALID_SPRITE;
    style->sprite_fill = GBAR89_INVALID_SPRITE;
    style->sprite_lag = GBAR89_INVALID_SPRITE;
    style->sprite_overlay = GBAR89_INVALID_SPRITE;
    style->sprite_empty = GBAR89_INVALID_SPRITE;
    style->sprite_full = GBAR89_INVALID_SPRITE;
    style->sprite_partial = GBAR89_INVALID_SPRITE;

    style->src_bg = gbar89_rect_make(0, 0, 0, 0);
    style->src_fill = gbar89_rect_make(0, 0, 0, 0);
    style->src_lag = gbar89_rect_make(0, 0, 0, 0);
    style->src_overlay = gbar89_rect_make(0, 0, 0, 0);
    style->src_empty = gbar89_rect_make(0, 0, 0, 0);
    style->src_full = gbar89_rect_make(0, 0, 0, 0);
    style->src_partial = gbar89_rect_make(0, 0, 0, 0);

    style->margin_left = 0;
    style->margin_top = 0;
    style->margin_right = 0;
    style->margin_bottom = 0;

    style->padding_left = 0;
    style->padding_top = 0;
    style->padding_right = 0;
    style->padding_bottom = 0;

    style->border_size = 1;
    style->segment_gap = 1;
    style->marker_size = 1;

    style->pattern_kind = GBAR89_PATTERN_NONE;
    style->pattern_step = 6;
    style->pattern_size = 1;

    style->frame_kind = GBAR89_FRAME_SIMPLE;
    style->frame_depth = 2;
    style->frame_gap = 1;
    style->frame_corner = 8;
    style->outline_size = 1;
    style->shadow_offset_x = 3;
    style->shadow_offset_y = 3;
    style->extrude_depth = 4;
    style->bg_kind = GBAR89_BG_SOLID;
    style->bg_step = 6;
    style->bg_size = 1;
    style->pixel_size = 4;
    style->gloss_percent = 45;

    style->radial_segments = 1;
    style->radial_gap_deg = 2;
    style->radial_cap_kind = GBAR89_RADIAL_CAP_BUTT;
    style->radial_detail_rings = 3;
    style->radial_unit_radius_percent = 82;
    style->radial_marker_length_percent = 18;
    style->radial_phase_deg = 0;

    style->fx_flags = GBAR89_FX_NONE;
}

void gbar89_init(GBar89_Meter *meter)
{
    int i;

    if (meter == 0) {
        return;
    }

    meter->kind = GBAR89_KIND_LINEAR;
    meter->direction = GBAR89_DIR_LEFT_TO_RIGHT;
    meter->flags = GBAR89_FLAG_CLAMP_VALUE |
                   GBAR89_FLAG_DRAW_BG |
                   GBAR89_FLAG_DRAW_BORDER |
                   GBAR89_FLAG_USE_VISUAL;

    meter->min_value = 0;
    meter->max_value = 100;
    meter->value = 100;
    meter->visual_value = 100;
    meter->lag_value = 100;

    meter->smooth_speed = 300;
    meter->lag_speed = 120;

    meter->overlay_min_value = 0;
    meter->overlay_max_value = 100;
    meter->overlay_value = 0;
    meter->overlay_visual_value = 0;
    meter->overlay_speed = 300;
    meter->overlay_direction = GBAR89_DIR_LEFT_TO_RIGHT;

    meter->mid_value = 100;
    meter->mid_visual_value = 100;
    meter->mid_speed = 300;
    meter->mid_direction = GBAR89_DIR_LEFT_TO_RIGHT;

    meter->rect = gbar89_rect_make(0, 0, 100, 10);
    gbar89_style_default(&meter->style);

    meter->segments = 10;

    meter->radial_start_deg = -90;
    meter->radial_sweep_deg = 360;
    meter->radial_inner_percent = 65;
    meter->radial_steps = 48;

    meter->layer_count = 1;
    meter->layer_size = 100;

    meter->marker_count = 0;
    for (i = 0; i < GBAR89_MAX_MARKERS; ++i) {
        meter->markers[i] = 0;
    }

    meter->state = GBAR89_STATE_NORMAL;
    meter->low_ratio = GBAR89_FIX_ONE / 3;
    meter->critical_ratio = GBAR89_FIX_ONE / 6;
    meter->state_timer = 0;
    meter->blink_period = GBAR89_FIX_HALF;

    meter->mask_kind = GBAR89_MASK_NONE;
    meter->mask_amount = 8;
    meter->mask_steps = 8;
    meter->mask_slice_count = 0;
    for (i = 0; i < GBAR89_MAX_MASK_SLICES; ++i) {
        meter->mask_slices[i].y_percent = 0;
        meter->mask_slices[i].h_percent = 0;
        meter->mask_slices[i].inset_left = 0;
        meter->mask_slices[i].inset_right = 0;
    }

    meter->ease_kind = GBAR89_EASE_LINEAR;
    meter->ease_duration = 0;
    meter->ease_elapsed = 0;
    meter->ease_start_value = meter->visual_value;
    meter->ease_target_value = meter->value;
    meter->ease_active = 0;

    meter->unit_points = 0;
    meter->unit_point_count = 0;
    meter->unit_closed = 1;
    meter->unit_filled = 1;
    meter->unit_count = 0;
    meter->unit_padding = 2;
    meter->unit_scale_percent = 80;
    meter->unit_renderer_user = 0;
    meter->unit_renderer = 0;
}

void gbar89_set_rect(GBar89_Meter *meter, int x, int y, int w, int h)
{
    if (meter == 0) {
        return;
    }
    meter->rect.x = x;
    meter->rect.y = y;
    meter->rect.w = w;
    meter->rect.h = h;
}

void gbar89_set_range(GBar89_Meter *meter, long min_value, long max_value)
{
    if (meter == 0) {
        return;
    }
    meter->min_value = min_value;
    meter->max_value = max_value;
    if (meter->flags & GBAR89_FLAG_CLAMP_VALUE) {
        meter->value = gbar89_clamp_value(meter, meter->value);
        meter->visual_value = gbar89_clamp_value(meter, meter->visual_value);
        meter->lag_value = gbar89_clamp_value(meter, meter->lag_value);
        meter->mid_value = gbar89_clamp_value(meter, meter->mid_value);
        meter->mid_visual_value = gbar89_clamp_value(meter, meter->mid_visual_value);
    }
}

void gbar89_set_value(GBar89_Meter *meter, long value)
{
    long old_target;

    if (meter == 0) {
        return;
    }

    old_target = meter->value;
    if (meter->flags & GBAR89_FLAG_CLAMP_VALUE) {
        meter->value = gbar89_clamp_value(meter, value);
    } else {
        meter->value = value;
    }

    if ((meter->flags & GBAR89_FLAG_EASE_VALUE) != 0 &&
        meter->ease_duration > 0 && meter->value != old_target) {
        meter->ease_start_value = meter->visual_value;
        meter->ease_target_value = meter->value;
        meter->ease_elapsed = 0;
        meter->ease_active = 1;
    }
}

void gbar89_set_kind(GBar89_Meter *meter, int kind)
{
    if (meter == 0) {
        return;
    }
    meter->kind = kind;
}

void gbar89_set_direction(GBar89_Meter *meter, int direction)
{
    if (meter == 0) {
        return;
    }
    meter->direction = direction;
}

void gbar89_set_segments(GBar89_Meter *meter, int segments, int gap)
{
    if (meter == 0) {
        return;
    }
    if (segments < 1) {
        segments = 1;
    }
    meter->segments = segments;
    if (gap < 0) {
        gap = 0;
    }
    meter->style.segment_gap = gap;
}

void gbar89_set_speeds(GBar89_Meter *meter, long smooth_speed, long lag_speed)
{
    if (meter == 0) {
        return;
    }
    if (smooth_speed < 0) {
        smooth_speed = 0;
    }
    if (lag_speed < 0) {
        lag_speed = 0;
    }
    meter->smooth_speed = smooth_speed;
    meter->lag_speed = lag_speed;
}

void gbar89_set_radial(GBar89_Meter *meter, int start_deg, int sweep_deg,
                       int inner_percent, int steps)
{
    if (meter == 0) {
        return;
    }

    if (inner_percent < 0) {
        inner_percent = 0;
    }
    if (inner_percent > 99) {
        inner_percent = 99;
    }
    if (steps < 3) {
        steps = 3;
    }
    if (steps > GBAR89_MAX_RADIAL_STEPS) {
        steps = GBAR89_MAX_RADIAL_STEPS;
    }

    meter->radial_start_deg = start_deg;
    meter->radial_sweep_deg = sweep_deg;
    meter->radial_inner_percent = inner_percent;
    meter->radial_steps = steps;
}

void gbar89_set_radial_style(GBar89_Meter *meter, int segments,
                             int gap_deg, int cap_kind,
                             int detail_rings, int unit_radius_percent)
{
    if (meter == 0) {
        return;
    }
    if (segments < 1) {
        segments = 1;
    }
    if (segments > GBAR89_MAX_RADIAL_STEPS) {
        segments = GBAR89_MAX_RADIAL_STEPS;
    }
    if (gap_deg < 0) {
        gap_deg = 0;
    }
    if (gap_deg > 90) {
        gap_deg = 90;
    }
    if (cap_kind < GBAR89_RADIAL_CAP_BUTT ||
        cap_kind > GBAR89_RADIAL_CAP_SQUARE) {
        cap_kind = GBAR89_RADIAL_CAP_BUTT;
    }
    if (detail_rings < 1) {
        detail_rings = 1;
    }
    if (detail_rings > 16) {
        detail_rings = 16;
    }
    if (unit_radius_percent < 0) {
        unit_radius_percent = 0;
    }
    if (unit_radius_percent > 150) {
        unit_radius_percent = 150;
    }

    meter->style.radial_segments = segments;
    meter->style.radial_gap_deg = gap_deg;
    meter->style.radial_cap_kind = cap_kind;
    meter->style.radial_detail_rings = detail_rings;
    meter->style.radial_unit_radius_percent = unit_radius_percent;
}

void gbar89_set_radial_phase(GBar89_Meter *meter, int phase_deg)
{
    if (meter == 0) {
        return;
    }
    meter->style.radial_phase_deg = phase_deg;
}

void gbar89_set_nineslice(GBar89_Meter *meter,
                          int left, int top, int right, int bottom)
{
    if (meter == 0) {
        return;
    }

    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right < 0) {
        right = 0;
    }
    if (bottom < 0) {
        bottom = 0;
    }

    meter->style.margin_left = left;
    meter->style.margin_top = top;
    meter->style.margin_right = right;
    meter->style.margin_bottom = bottom;
}

int gbar89_add_marker(GBar89_Meter *meter, long value)
{
    if (meter == 0) {
        return GBAR89_ERR_NULL;
    }
    if (meter->marker_count >= GBAR89_MAX_MARKERS) {
        return GBAR89_ERR_FULL;
    }
    meter->markers[meter->marker_count] = value;
    meter->marker_count += 1;
    return GBAR89_OK;
}

void gbar89_set_layers(GBar89_Meter *meter, int layer_count, long layer_size)
{
    if (meter == 0) {
        return;
    }
    if (layer_count < 1) {
        layer_count = 1;
    }
    if (layer_count > GBAR89_MAX_LAYERS) {
        layer_count = GBAR89_MAX_LAYERS;
    }
    if (layer_size <= 0) {
        layer_size = meter->max_value - meter->min_value;
        if (layer_size <= 0) {
            layer_size = 1;
        }
    }
    meter->layer_count = layer_count;
    meter->layer_size = layer_size;
}

void gbar89_set_layer_color(GBar89_Meter *meter, int layer_index,
                            unsigned long fill_rgba,
                            unsigned long lag_rgba)
{
    if (meter == 0) {
        return;
    }
    if (layer_index < 0 || layer_index >= GBAR89_MAX_LAYERS) {
        return;
    }
    meter->style.color_layer_fill[layer_index] = fill_rgba;
    meter->style.color_layer_lag[layer_index] = lag_rgba;
}

int gbar89_layer_index_from_value(const GBar89_Meter *meter, long value)
{
    long rel;
    int idx;

    if (meter == 0 || meter->layer_size <= 0) {
        return 0;
    }
    value = gbar89_clamp_value(meter, value);
    rel = value - meter->min_value;
    if (rel <= 0) {
        return 0;
    }
    idx = (int)((rel - 1) / meter->layer_size);
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= meter->layer_count) {
        idx = meter->layer_count - 1;
    }
    return idx;
}

long gbar89_layer_value_from_value(const GBar89_Meter *meter, long value)
{
    long rel;
    long v;

    if (meter == 0 || meter->layer_size <= 0) {
        return value;
    }
    value = gbar89_clamp_value(meter, value);
    rel = value - meter->min_value;
    if (rel <= 0) {
        return 0;
    }
    v = ((rel - 1) % meter->layer_size) + 1;
    if (v < 0) {
        v = 0;
    }
    if (v > meter->layer_size) {
        v = meter->layer_size;
    }
    return v;
}

GBar89_Fix gbar89_layer_ratio_from_value(const GBar89_Meter *meter,
                                         long value)
{
    long v;

    if (meter == 0 || meter->layer_size <= 0) {
        return 0;
    }
    v = gbar89_layer_value_from_value(meter, value);
    if (v <= 0) {
        return 0;
    }
    if (v >= meter->layer_size) {
        return GBAR89_FIX_ONE;
    }
    return (GBar89_Fix)((v << GBAR89_FIX_SHIFT) / meter->layer_size);
}

int gbar89_layer_count_filled(const GBar89_Meter *meter, long value)
{
    long rel;
    int count;

    if (meter == 0 || meter->layer_size <= 0) {
        return 0;
    }
    value = gbar89_clamp_value(meter, value);
    rel = value - meter->min_value;
    if (rel <= 0) {
        return 0;
    }
    count = (int)((rel + meter->layer_size - 1) / meter->layer_size);
    if (count < 0) {
        count = 0;
    }
    if (count > meter->layer_count) {
        count = meter->layer_count;
    }
    return count;
}

void gbar89_set_overlay_range(GBar89_Meter *meter,
                              long min_value, long max_value)
{
    if (meter == 0) {
        return;
    }
    meter->overlay_min_value = min_value;
    meter->overlay_max_value = max_value;
    if (meter->overlay_value < min_value) {
        meter->overlay_value = min_value;
    }
    if (meter->overlay_value > max_value) {
        meter->overlay_value = max_value;
    }
    if (meter->overlay_visual_value < min_value) {
        meter->overlay_visual_value = min_value;
    }
    if (meter->overlay_visual_value > max_value) {
        meter->overlay_visual_value = max_value;
    }
}

void gbar89_set_overlay_value(GBar89_Meter *meter, long value)
{
    if (meter == 0) {
        return;
    }
    if (value < meter->overlay_min_value) {
        value = meter->overlay_min_value;
    }
    if (value > meter->overlay_max_value) {
        value = meter->overlay_max_value;
    }
    meter->overlay_value = value;
}

void gbar89_set_overlay_direction(GBar89_Meter *meter, int direction)
{
    if (meter == 0) {
        return;
    }
    meter->overlay_direction = direction;
}

GBar89_Fix gbar89_overlay_ratio(const GBar89_Meter *meter)
{
    long den;
    long num;

    if (meter == 0) {
        return 0;
    }
    den = meter->overlay_max_value - meter->overlay_min_value;
    num = meter->overlay_visual_value - meter->overlay_min_value;
    if (den <= 0 || num <= 0) {
        return 0;
    }
    if (num >= den) {
        return GBAR89_FIX_ONE;
    }
    return (GBar89_Fix)((num << GBAR89_FIX_SHIFT) / den);
}

void gbar89_set_mid_value(GBar89_Meter *meter, long value)
{
    if (meter == 0) {
        return;
    }
    if ((meter->flags & GBAR89_FLAG_CLAMP_VALUE) != 0) {
        value = gbar89_clamp_value(meter, value);
    }
    meter->mid_value = value;
}

void gbar89_set_mid_speed(GBar89_Meter *meter, long speed)
{
    if (meter == 0) {
        return;
    }
    if (speed < 0) {
        speed = 0;
    }
    meter->mid_speed = speed;
}

void gbar89_set_mid_direction(GBar89_Meter *meter, int direction)
{
    if (meter == 0) {
        return;
    }
    meter->mid_direction = direction;
}

GBar89_Fix gbar89_mid_ratio(const GBar89_Meter *meter)
{
    if (meter == 0) {
        return 0;
    }
    return gbar89_display_ratio_from_value(meter, meter->mid_visual_value);
}

void gbar89_set_frame(GBar89_Meter *meter, int frame_kind,
                      int depth, int gap, int corner_length)
{
    if (meter == 0) {
        return;
    }
    if (frame_kind < GBAR89_FRAME_SIMPLE || frame_kind > GBAR89_FRAME_NONE) {
        frame_kind = GBAR89_FRAME_SIMPLE;
    }
    if (depth < 1) {
        depth = 1;
    }
    if (gap < 0) {
        gap = 0;
    }
    if (corner_length < 1) {
        corner_length = 1;
    }
    meter->style.frame_kind = frame_kind;
    meter->style.frame_depth = depth;
    meter->style.frame_gap = gap;
    meter->style.frame_corner = corner_length;
}

void gbar89_set_outline(GBar89_Meter *meter, int size,
                        unsigned long color_rgba)
{
    if (meter == 0) {
        return;
    }
    if (size < 0) {
        size = 0;
    }
    meter->style.outline_size = size;
    meter->style.color_outline = color_rgba;
}

void gbar89_set_background(GBar89_Meter *meter, int bg_kind,
                           int step, int size,
                           unsigned long detail_rgba)
{
    if (meter == 0) {
        return;
    }
    if (bg_kind < GBAR89_BG_SOLID || bg_kind > GBAR89_BG_DITHER) {
        bg_kind = GBAR89_BG_SOLID;
    }
    if (step < 1) {
        step = 1;
    }
    if (size < 1) {
        size = 1;
    }
    meter->style.bg_kind = bg_kind;
    meter->style.bg_step = step;
    meter->style.bg_size = size;
    meter->style.color_bg_detail = detail_rgba;
}

void gbar89_set_fx_flags(GBar89_Meter *meter, unsigned long fx_flags)
{
    if (meter == 0) {
        return;
    }
    meter->style.fx_flags = fx_flags;
}

void gbar89_set_pixel_size(GBar89_Meter *meter, int pixel_size)
{
    if (meter == 0) {
        return;
    }
    if (pixel_size < 1) {
        pixel_size = 1;
    }
    meter->style.pixel_size = pixel_size;
}

void gbar89_set_unit_vector(GBar89_Meter *meter,
                            const GBar89_VectorPoint *points,
                            int point_count, int closed, int filled,
                            int unit_count, int padding,
                            int scale_percent)
{
    if (meter == 0) {
        return;
    }
    if (point_count < 0) {
        point_count = 0;
    }
    if (unit_count < 0) {
        unit_count = 0;
    }
    if (padding < 0) {
        padding = 0;
    }
    if (scale_percent < 1) {
        scale_percent = 1;
    }
    if (scale_percent > 100) {
        scale_percent = 100;
    }
    meter->unit_points = points;
    meter->unit_point_count = point_count;
    meter->unit_closed = closed ? 1 : 0;
    meter->unit_filled = filled ? 1 : 0;
    meter->unit_count = unit_count;
    meter->unit_padding = padding;
    meter->unit_scale_percent = scale_percent;
}

void gbar89_set_unit_renderer(GBar89_Meter *meter,
                              GBar89_UnitRenderFn renderer,
                              void *user)
{
    if (meter == 0) {
        return;
    }
    meter->unit_renderer = renderer;
    meter->unit_renderer_user = user;
}

void gbar89_set_state(GBar89_Meter *meter, int state)
{
    if (meter == 0) {
        return;
    }
    if (state < 0) {
        state = GBAR89_STATE_NORMAL;
    }
    if (state >= GBAR89_STATE_COUNT) {
        state = GBAR89_STATE_CUSTOM;
    }
    meter->state = state;
}

void gbar89_set_state_thresholds(GBar89_Meter *meter,
                                 GBar89_Fix low_ratio,
                                 GBar89_Fix critical_ratio)
{
    if (meter == 0) {
        return;
    }
    meter->low_ratio = gbar89_fix_clamp01(low_ratio);
    meter->critical_ratio = gbar89_fix_clamp01(critical_ratio);
}

void gbar89_set_state_color(GBar89_Meter *meter, int state,
                            unsigned long fill_rgba,
                            unsigned long border_rgba)
{
    if (meter == 0) {
        return;
    }
    if (state < 0 || state >= GBAR89_STATE_COUNT) {
        return;
    }
    meter->style.color_state_fill[state] = fill_rgba;
    meter->style.color_state_border[state] = border_rgba;
}

int gbar89_get_state(const GBar89_Meter *meter)
{
    if (meter == 0) {
        return GBAR89_STATE_NORMAL;
    }
    return meter->state;
}

void gbar89_set_pattern(GBar89_Meter *meter, int pattern_kind,
                        int step, int size)
{
    if (meter == 0) {
        return;
    }
    if (step <= 0) {
        step = 6;
    }
    if (size <= 0) {
        size = 1;
    }
    meter->style.pattern_kind = pattern_kind;
    meter->style.pattern_step = step;
    meter->style.pattern_size = size;
}

void gbar89_set_mask(GBar89_Meter *meter, int mask_kind,
                     int amount_px, int steps)
{
    if (meter == 0) {
        return;
    }
    if (amount_px < 0) {
        amount_px = -amount_px;
    }
    if (steps < 1) {
        steps = 1;
    }
    if (steps > GBAR89_MAX_MASK_SLICES) {
        steps = GBAR89_MAX_MASK_SLICES;
    }
    meter->mask_kind = mask_kind;
    meter->mask_amount = amount_px;
    meter->mask_steps = steps;
}

void gbar89_clear_mask_slices(GBar89_Meter *meter)
{
    if (meter == 0) {
        return;
    }
    meter->mask_slice_count = 0;
}

int gbar89_add_mask_slice(GBar89_Meter *meter, int y_percent,
                          int h_percent, int inset_left,
                          int inset_right)
{
    GBar89_MaskSlice *s;

    if (meter == 0) {
        return GBAR89_ERR_NULL;
    }
    if (meter->mask_slice_count >= GBAR89_MAX_MASK_SLICES) {
        return GBAR89_ERR_FULL;
    }
    if (y_percent < 0) {
        y_percent = 0;
    }
    if (y_percent > 100) {
        y_percent = 100;
    }
    if (h_percent < 0) {
        h_percent = 0;
    }
    if (h_percent > 100) {
        h_percent = 100;
    }
    if (inset_left < 0) {
        inset_left = 0;
    }
    if (inset_right < 0) {
        inset_right = 0;
    }
    s = &meter->mask_slices[meter->mask_slice_count];
    s->y_percent = y_percent;
    s->h_percent = h_percent;
    s->inset_left = inset_left;
    s->inset_right = inset_right;
    meter->mask_slice_count += 1;
    return GBAR89_OK;
}

void gbar89_set_easing(GBar89_Meter *meter, int ease_kind,
                       GBar89_Fix duration_seconds)
{
    if (meter == 0) {
        return;
    }
    if (duration_seconds < 0) {
        duration_seconds = 0;
    }
    meter->ease_kind = ease_kind;
    meter->ease_duration = duration_seconds;
    if (duration_seconds > 0) {
        meter->flags |= GBAR89_FLAG_EASE_VALUE;
    } else {
        meter->flags &= ~GBAR89_FLAG_EASE_VALUE;
        meter->ease_active = 0;
    }
}

long gbar89_clamp_value(const GBar89_Meter *meter, long value)
{
    long a;
    long b;

    if (meter == 0) {
        return value;
    }

    a = meter->min_value;
    b = meter->max_value;

    if (a > b) {
        if (value < b) {
            return b;
        }
        if (value > a) {
            return a;
        }
        return value;
    }

    if (value < a) {
        return a;
    }
    if (value > b) {
        return b;
    }
    return value;
}

GBar89_Fix gbar89_ratio_from_value(const GBar89_Meter *meter, long value)
{
    long minv;
    long maxv;
    long den;
    long num;
    GBar89_Fix r;

    if (meter == 0) {
        return 0;
    }

    minv = meter->min_value;
    maxv = meter->max_value;

    if (maxv == minv) {
        return 0;
    }

    if (meter->flags & GBAR89_FLAG_CLAMP_VALUE) {
        value = gbar89_clamp_value(meter, value);
    }

    den = maxv - minv;
    num = value - minv;

    if (den < 0) {
        den = -den;
        num = -num;
    }

    if (num <= 0) {
        r = 0;
    } else if (num >= den) {
        r = GBAR89_FIX_ONE;
    } else {
        while (num > 32767L || den > 32767L) {
            num >>= 1;
            den >>= 1;
            if (den <= 0) {
                return 0;
            }
        }
        r = (GBar89_Fix)((num << GBAR89_FIX_SHIFT) / den);
    }

    if (meter->flags & GBAR89_FLAG_INVERT_RATIO) {
        r = GBAR89_FIX_ONE - r;
    }

    return gbar89_fix_clamp01(r);
}

GBar89_Fix gbar89_display_ratio_from_value(const GBar89_Meter *meter,
                                           long value)
{
    GBar89_Fix r;

    if (meter == 0) {
        return 0;
    }
    if ((meter->flags & GBAR89_FLAG_LAYERED) != 0 &&
        meter->layer_size > 0) {
        r = gbar89_layer_ratio_from_value(meter, value);
        if (meter->flags & GBAR89_FLAG_INVERT_RATIO) {
            r = GBAR89_FIX_ONE - r;
        }
        return gbar89_fix_clamp01(r);
    }
    return gbar89_ratio_from_value(meter, value);
}

GBar89_Fix gbar89_current_ratio(const GBar89_Meter *meter)
{
    long v;

    if (meter == 0) {
        return 0;
    }

    if ((meter->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        v = meter->visual_value;
    } else {
        v = meter->value;
    }

    return gbar89_display_ratio_from_value(meter, v);
}

int gbar89_ratio_to_pixels(GBar89_Fix ratio, int pixels)
{
    if (pixels <= 0) {
        return 0;
    }
    if (ratio <= 0) {
        return 0;
    }
    if (ratio >= GBAR89_FIX_ONE) {
        return pixels;
    }
    return (int)(((long)pixels * ratio) >> GBAR89_FIX_SHIFT);
}

static long gbar89_approach_long(long current, long target, long max_delta)
{
    if (max_delta < 0) {
        max_delta = -max_delta;
    }

    if (current < target) {
        if (target - current <= max_delta) {
            return target;
        }
        return current + max_delta;
    }

    if (current > target) {
        if (current - target <= max_delta) {
            return target;
        }
        return current - max_delta;
    }

    return current;
}

static void gbar89_update_auto_state(GBar89_Meter *meter)
{
    GBar89_Fix ratio;

    if (meter == 0) {
        return;
    }
    if ((meter->flags & GBAR89_FLAG_AUTO_STATE) == 0) {
        return;
    }

    ratio = gbar89_display_ratio_from_value(meter, meter->value);
    if (ratio <= meter->critical_ratio) {
        meter->state = GBAR89_STATE_CRITICAL;
    } else if (ratio <= meter->low_ratio) {
        meter->state = GBAR89_STATE_LOW;
    } else {
        meter->state = GBAR89_STATE_NORMAL;
    }
}

void gbar89_tick(GBar89_Meter *meter, GBar89_Fix dt_seconds)
{
    long target;
    long smooth_delta;
    long lag_delta;
    long overlay_delta;
    long mid_delta;
    GBar89_Fix t;
    GBar89_Fix eased;

    if (meter == 0) {
        return;
    }

    if (dt_seconds < 0) {
        dt_seconds = 0;
    }

    meter->state_timer += dt_seconds;

    target = meter->value;
    if (meter->flags & GBAR89_FLAG_CLAMP_VALUE) {
        target = gbar89_clamp_value(meter, target);
    }

    if ((meter->flags & GBAR89_FLAG_EASE_VALUE) != 0 &&
        meter->ease_duration > 0 && meter->ease_active != 0) {
        meter->ease_elapsed += dt_seconds;
        if (meter->ease_elapsed >= meter->ease_duration) {
            meter->visual_value = meter->ease_target_value;
            meter->ease_elapsed = meter->ease_duration;
            meter->ease_active = 0;
        } else {
            t = gbar89_fix_div(meter->ease_elapsed, meter->ease_duration);
            eased = gbar89_ease(meter->ease_kind, t);
            meter->visual_value = gbar89_lerp_long(meter->ease_start_value,
                                                   meter->ease_target_value,
                                                   eased);
        }
    } else if (meter->flags & GBAR89_FLAG_SMOOTH_VALUE) {
        smooth_delta = (long)((meter->smooth_speed * dt_seconds) >> GBAR89_FIX_SHIFT);
        if (smooth_delta <= 0 && meter->visual_value != target) {
            smooth_delta = 1;
        }
        meter->visual_value = gbar89_approach_long(meter->visual_value,
                                                   target,
                                                   smooth_delta);
    } else {
        meter->visual_value = target;
    }

    if (meter->flags & GBAR89_FLAG_DAMAGE_LAG) {
        if (meter->lag_value < meter->visual_value) {
            meter->lag_value = meter->visual_value;
        } else {
            lag_delta = (long)((meter->lag_speed * dt_seconds) >> GBAR89_FIX_SHIFT);
            if (lag_delta <= 0 && meter->lag_value != meter->visual_value) {
                lag_delta = 1;
            }
            meter->lag_value = gbar89_approach_long(meter->lag_value,
                                                    meter->visual_value,
                                                    lag_delta);
        }
    } else {
        meter->lag_value = meter->visual_value;
    }

    overlay_delta = (long)((meter->overlay_speed * dt_seconds) >> GBAR89_FIX_SHIFT);
    if (overlay_delta <= 0 && meter->overlay_visual_value != meter->overlay_value) {
        overlay_delta = 1;
    }
    meter->overlay_visual_value = gbar89_approach_long(meter->overlay_visual_value,
                                                       meter->overlay_value,
                                                       overlay_delta);

    mid_delta = (long)((meter->mid_speed * dt_seconds) >> GBAR89_FIX_SHIFT);
    if (mid_delta <= 0 && meter->mid_visual_value != meter->mid_value) {
        mid_delta = 1;
    }
    meter->mid_visual_value = gbar89_approach_long(meter->mid_visual_value,
                                                   meter->mid_value,
                                                   mid_delta);

    gbar89_update_auto_state(meter);
}

static GBar89_Rect gbar89_fill_rect_for_ratio(const GBar89_Rect *base,
                                              int direction,
                                              GBar89_Fix ratio)
{
    GBar89_Rect r;
    int n;
    int cx;
    int cy;

    r = *base;

    if (ratio <= 0) {
        r.w = 0;
        r.h = 0;
        return r;
    }
    if (ratio > GBAR89_FIX_ONE) {
        ratio = GBAR89_FIX_ONE;
    }

    if (direction == GBAR89_DIR_LEFT_TO_RIGHT) {
        r.w = gbar89_ratio_to_pixels(ratio, base->w);
    } else if (direction == GBAR89_DIR_RIGHT_TO_LEFT) {
        n = gbar89_ratio_to_pixels(ratio, base->w);
        r.x = base->x + base->w - n;
        r.w = n;
    } else if (direction == GBAR89_DIR_TOP_TO_BOTTOM) {
        r.h = gbar89_ratio_to_pixels(ratio, base->h);
    } else if (direction == GBAR89_DIR_BOTTOM_TO_TOP) {
        n = gbar89_ratio_to_pixels(ratio, base->h);
        r.y = base->y + base->h - n;
        r.h = n;
    } else if (direction == GBAR89_DIR_CENTER_HORIZONTAL) {
        n = gbar89_ratio_to_pixels(ratio, base->w);
        cx = base->x + base->w / 2;
        r.x = cx - n / 2;
        r.w = n;
    } else if (direction == GBAR89_DIR_CENTER_VERTICAL) {
        n = gbar89_ratio_to_pixels(ratio, base->h);
        cy = base->y + base->h / 2;
        r.y = cy - n / 2;
        r.h = n;
    }

    if (r.w < 0) {
        r.w = 0;
    }
    if (r.h < 0) {
        r.h = 0;
    }

    return r;
}


static GBar89_Fix gbar89_quantize_ratio(const GBar89_Meter *m,
                                        const GBar89_Rect *base,
                                        int direction,
                                        GBar89_Fix ratio)
{
    int axis;
    int px;
    int q;
    int cell;

    if (m == 0 || base == 0) {
        return ratio;
    }
    if ((m->flags & GBAR89_FLAG_PIXEL_QUANTIZE) == 0) {
        return ratio;
    }

    axis = base->w;
    if (direction == GBAR89_DIR_TOP_TO_BOTTOM ||
        direction == GBAR89_DIR_BOTTOM_TO_TOP ||
        direction == GBAR89_DIR_CENTER_VERTICAL) {
        axis = base->h;
    }
    if (axis <= 0) {
        return 0;
    }

    cell = m->style.pixel_size;
    if (cell < 1) {
        cell = 1;
    }
    px = gbar89_ratio_to_pixels(ratio, axis);
    q = (px / cell) * cell;
    if (px > 0 && q == 0) {
        q = cell;
    }
    if (q > axis) {
        q = axis;
    }
    return (GBar89_Fix)(((long)q << GBAR89_FIX_SHIFT) / axis);
}

static void gbar89_draw_background_detail(const GBar89_Meter *m,
                                          const GBar89_RenderOps *ops,
                                          const GBar89_Rect *rect)
{
    int step;
    int size;
    int x;
    int y;
    int row;
    GBar89_Rect q;

    if (m == 0 || ops == 0 || rect == 0) {
        return;
    }
    if (m->style.bg_kind == GBAR89_BG_SOLID) {
        return;
    }
    if (rect->w <= 0 || rect->h <= 0) {
        return;
    }

    step = m->style.bg_step;
    size = m->style.bg_size;
    if (step < 1) {
        step = 1;
    }
    if (size < 1) {
        size = 1;
    }

    if (ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->push_clip(ops->user, rect->x, rect->y, rect->w, rect->h);
    }

    if (m->style.bg_kind == GBAR89_BG_GRID) {
        for (x = rect->x; x < rect->x + rect->w; x += step) {
            q = gbar89_rect_make(x, rect->y, size, rect->h);
            gbar89_call_rect(ops, &q, m->style.color_bg_detail);
        }
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            q = gbar89_rect_make(rect->x, y, rect->w, size);
            gbar89_call_rect(ops, &q, m->style.color_bg_detail);
        }
    } else if (m->style.bg_kind == GBAR89_BG_CHECKER) {
        row = 0;
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            for (x = rect->x; x < rect->x + rect->w; x += step) {
                if ((((x - rect->x) / step) + row) % 2 == 0) {
                    q = gbar89_rect_make(x, y, step, step);
                    gbar89_call_rect(ops, &q, m->style.color_bg_detail);
                }
            }
            row += 1;
        }
    } else if (m->style.bg_kind == GBAR89_BG_SCANLINES) {
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            q = gbar89_rect_make(rect->x, y, rect->w, size);
            gbar89_call_rect(ops, &q, m->style.color_bg_detail);
        }
    } else if (m->style.bg_kind == GBAR89_BG_DIAGONAL) {
        for (x = rect->x - rect->h; x < rect->x + rect->w; x += step) {
            int k;
            for (k = 0; k < rect->h; k += size) {
                q = gbar89_rect_make(x + k, rect->y + k, size, size);
                gbar89_call_rect(ops, &q, m->style.color_bg_detail);
            }
        }
    } else if (m->style.bg_kind == GBAR89_BG_DITHER) {
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            for (x = rect->x; x < rect->x + rect->w; x += step) {
                if ((((x * 3) + (y * 5)) & 7) < 3) {
                    q = gbar89_rect_make(x, y, size, size);
                    gbar89_call_rect(ops, &q, m->style.color_bg_detail);
                }
            }
        }
    }

    if (ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->pop_clip(ops->user);
    }
}

static void gbar89_draw_fill_fx(const GBar89_Meter *m,
                                const GBar89_RenderOps *ops,
                                const GBar89_Rect *fill)
{
    int step;
    int cell;
    int x;
    int y;
    int gh;
    int vertical;
    GBar89_Rect q;

    if (m == 0 || ops == 0 || fill == 0) {
        return;
    }
    if (fill->w <= 0 || fill->h <= 0) {
        return;
    }

    if (ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->push_clip(ops->user, fill->x, fill->y, fill->w, fill->h);
    }

    vertical = (m->direction == GBAR89_DIR_TOP_TO_BOTTOM ||
                m->direction == GBAR89_DIR_BOTTOM_TO_TOP ||
                m->direction == GBAR89_DIR_CENTER_VERTICAL);

    if ((m->style.fx_flags & GBAR89_FX_GLOSS) != 0) {
        if (vertical) {
            gh = (fill->w * m->style.gloss_percent) / 100;
            if (gh < 1) {
                gh = 1;
            }
            q = gbar89_rect_make(fill->x, fill->y, gh, fill->h);
        } else {
            gh = (fill->h * m->style.gloss_percent) / 100;
            if (gh < 1) {
                gh = 1;
            }
            q = gbar89_rect_make(fill->x, fill->y, fill->w, gh);
        }
        gbar89_call_rect(ops, &q, m->style.color_gloss);
    }

    step = m->style.bg_step;
    if (step < 2) {
        step = 2;
    }

    if ((m->style.fx_flags & GBAR89_FX_FILL_GRID) != 0) {
        for (x = fill->x; x < fill->x + fill->w; x += step) {
            q = gbar89_rect_make(x, fill->y, 1, fill->h);
            gbar89_call_rect(ops, &q, m->style.color_pattern);
        }
        for (y = fill->y; y < fill->y + fill->h; y += step) {
            q = gbar89_rect_make(fill->x, y, fill->w, 1);
            gbar89_call_rect(ops, &q, m->style.color_pattern);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_FILL_SCANLINES) != 0) {
        for (y = fill->y + 1; y < fill->y + fill->h; y += step) {
            q = gbar89_rect_make(fill->x, y, fill->w, 1);
            gbar89_call_rect(ops, &q, m->style.color_pattern);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_PIXEL_CELLS) != 0) {
        cell = m->style.pixel_size;
        if (cell < 2) {
            cell = 2;
        }
        for (x = fill->x + cell; x < fill->x + fill->w; x += cell) {
            q = gbar89_rect_make(x - 1, fill->y, 1, fill->h);
            gbar89_call_rect(ops, &q, m->style.color_shadow);
        }
        for (y = fill->y + cell; y < fill->y + fill->h; y += cell) {
            q = gbar89_rect_make(fill->x, y - 1, fill->w, 1);
            gbar89_call_rect(ops, &q, m->style.color_shadow);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_INNER_SHADOW) != 0) {
        q = gbar89_rect_make(fill->x, fill->y + fill->h - 2, fill->w, 2);
        gbar89_call_rect(ops, &q, m->style.color_shadow);
        q = gbar89_rect_make(fill->x + fill->w - 2, fill->y, 2, fill->h);
        gbar89_call_rect(ops, &q, m->style.color_shadow);
    }

    if (ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->pop_clip(ops->user);
    }
}

static unsigned long gbar89_fill_color_for_value(const GBar89_Meter *m,
                                                 long value)
{
    int idx;
    unsigned long color;

    color = m->style.color_fill;

    if ((m->flags & GBAR89_FLAG_LAYERED) != 0) {
        idx = gbar89_layer_index_from_value(m, value);
        if (idx >= 0 && idx < GBAR89_MAX_LAYERS &&
            m->style.color_layer_fill[idx] != 0UL) {
            color = m->style.color_layer_fill[idx];
        }
    }

    if (m->state >= 0 && m->state < GBAR89_STATE_COUNT) {
        if (m->style.color_state_fill[m->state] != 0UL) {
            color = m->style.color_state_fill[m->state];
        }
    }

    if ((m->flags & GBAR89_FLAG_STATE_BLINK) != 0 &&
        m->blink_period > 0 &&
        (m->state == GBAR89_STATE_CRITICAL || m->state == GBAR89_STATE_LOW)) {
        long n;

        n = m->state_timer / m->blink_period;
        if ((n % 2L) != 0L) {
            color = m->style.color_empty;
        }
    }

    return color;
}

static unsigned long gbar89_lag_color_for_value(const GBar89_Meter *m,
                                                long value)
{
    int idx;
    unsigned long color;

    color = m->style.color_lag;
    if ((m->flags & GBAR89_FLAG_LAYERED) != 0) {
        idx = gbar89_layer_index_from_value(m, value);
        if (idx >= 0 && idx < GBAR89_MAX_LAYERS &&
            m->style.color_layer_lag[idx] != 0UL) {
            color = m->style.color_layer_lag[idx];
        }
    }
    return color;
}

static void gbar89_draw_pattern_rect(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *rect)
{
    int step;
    int size;
    int x;
    int y;
    GBar89_Rect p;

    if (m == 0 || ops == 0 || rect == 0) {
        return;
    }
    if ((m->flags & GBAR89_FLAG_DRAW_PATTERN) == 0) {
        return;
    }
    if (m->style.pattern_kind == GBAR89_PATTERN_NONE) {
        return;
    }
    if (rect->w <= 0 || rect->h <= 0) {
        return;
    }

    step = m->style.pattern_step;
    size = m->style.pattern_size;
    if (step <= 0) {
        step = 6;
    }
    if (size <= 0) {
        size = 1;
    }

    if (m->style.pattern_kind == GBAR89_PATTERN_VERTICAL_STRIPES ||
        m->style.pattern_kind == GBAR89_PATTERN_CROSSHATCH ||
        m->style.pattern_kind == GBAR89_PATTERN_TICKS) {
        for (x = rect->x; x < rect->x + rect->w; x += step) {
            p = gbar89_rect_make(x, rect->y, size, rect->h);
            gbar89_call_rect(ops, &p, m->style.color_pattern);
        }
    }

    if (m->style.pattern_kind == GBAR89_PATTERN_HORIZONTAL_STRIPES ||
        m->style.pattern_kind == GBAR89_PATTERN_CROSSHATCH) {
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            p = gbar89_rect_make(rect->x, y, rect->w, size);
            gbar89_call_rect(ops, &p, m->style.color_pattern);
        }
    }

    if (m->style.pattern_kind == GBAR89_PATTERN_DOTS) {
        for (y = rect->y; y < rect->y + rect->h; y += step) {
            for (x = rect->x; x < rect->x + rect->w; x += step) {
                p = gbar89_rect_make(x, y, size, size);
                gbar89_call_rect(ops, &p, m->style.color_pattern);
            }
        }
    }
}

static void gbar89_draw_poly_color(const GBar89_RenderOps *ops,
                                   const int *xy,
                                   int point_count,
                                   unsigned long color)
{
    GBar89_Vertex vertices[8];
    int indices[18];
    int i;

    if (ops == 0 || xy == 0) {
        return;
    }
    if (ops->draw_triangles == 0) {
        return;
    }
    if (point_count < 3 || point_count > 8) {
        return;
    }

    for (i = 0; i < point_count; ++i) {
        vertices[i].x = xy[i * 2];
        vertices[i].y = xy[i * 2 + 1];
        vertices[i].u = 0;
        vertices[i].v = 0;
        vertices[i].color = color;
    }

    for (i = 0; i < point_count - 2; ++i) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    ops->draw_triangles(ops->user, vertices, point_count, indices,
                        (point_count - 2) * 3, GBAR89_INVALID_SPRITE);
}

static void gbar89_draw_custom_slices(const GBar89_Meter *m,
                                      const GBar89_RenderOps *ops,
                                      const GBar89_Rect *base,
                                      const GBar89_Rect *clip,
                                      unsigned long color,
                                      int draw_pattern)
{
    int i;
    GBar89_Rect s;
    GBar89_Rect clipped;
    const GBar89_MaskSlice *slice;

    if (m == 0 || ops == 0 || base == 0 || clip == 0) {
        return;
    }

    if (m->mask_slice_count <= 0) {
        GBar89_Rect clipped_default;

        clipped_default = gbar89_rect_intersect(base, clip);
        gbar89_call_rect(ops, &clipped_default, color);
        if (draw_pattern) {
            gbar89_draw_pattern_rect(m, ops, &clipped_default);
        }
        return;
    }

    for (i = 0; i < m->mask_slice_count; ++i) {
        slice = &m->mask_slices[i];
        s.x = base->x + slice->inset_left;
        s.y = base->y + (base->h * slice->y_percent) / 100;
        s.w = base->w - slice->inset_left - slice->inset_right;
        s.h = (base->h * slice->h_percent) / 100;
        if (s.h <= 0) {
            s.h = 1;
        }
        if (s.w < 0) {
            s.w = 0;
        }
        clipped = gbar89_rect_intersect(&s, clip);
        gbar89_call_rect(ops, &clipped, color);
        if (draw_pattern) {
            gbar89_draw_pattern_rect(m, ops, &clipped);
        }
    }
}

static void gbar89_draw_mask_shape(const GBar89_Meter *m,
                                   const GBar89_RenderOps *ops,
                                   const GBar89_Rect *base,
                                   unsigned long color)
{
    int xy[16];
    int a;
    int cx;
    int cy;

    if (m == 0 || ops == 0 || base == 0) {
        return;
    }

    a = m->mask_amount;
    if (a < 0) {
        a = -a;
    }
    if (a > base->w / 2) {
        a = base->w / 2;
    }
    if (a > base->h / 2 &&
        (m->mask_kind == GBAR89_MASK_DIAMOND ||
         m->mask_kind == GBAR89_MASK_HEXAGON)) {
        a = base->h / 2;
    }

    if (m->mask_kind == GBAR89_MASK_SLANT_RIGHT) {
        xy[0] = base->x + a;
        xy[1] = base->y;
        xy[2] = base->x + base->w;
        xy[3] = base->y;
        xy[4] = base->x + base->w - a;
        xy[5] = base->y + base->h;
        xy[6] = base->x;
        xy[7] = base->y + base->h;
        gbar89_draw_poly_color(ops, xy, 4, color);
    } else if (m->mask_kind == GBAR89_MASK_SLANT_LEFT) {
        xy[0] = base->x;
        xy[1] = base->y;
        xy[2] = base->x + base->w - a;
        xy[3] = base->y;
        xy[4] = base->x + base->w;
        xy[5] = base->y + base->h;
        xy[6] = base->x + a;
        xy[7] = base->y + base->h;
        gbar89_draw_poly_color(ops, xy, 4, color);
    } else if (m->mask_kind == GBAR89_MASK_HEXAGON) {
        cy = base->y + base->h / 2;
        xy[0] = base->x + a;
        xy[1] = base->y;
        xy[2] = base->x + base->w - a;
        xy[3] = base->y;
        xy[4] = base->x + base->w;
        xy[5] = cy;
        xy[6] = base->x + base->w - a;
        xy[7] = base->y + base->h;
        xy[8] = base->x + a;
        xy[9] = base->y + base->h;
        xy[10] = base->x;
        xy[11] = cy;
        gbar89_draw_poly_color(ops, xy, 6, color);
    } else if (m->mask_kind == GBAR89_MASK_DIAMOND) {
        cx = base->x + base->w / 2;
        cy = base->y + base->h / 2;
        xy[0] = cx;
        xy[1] = base->y;
        xy[2] = base->x + base->w;
        xy[3] = cy;
        xy[4] = cx;
        xy[5] = base->y + base->h;
        xy[6] = base->x;
        xy[7] = cy;
        gbar89_draw_poly_color(ops, xy, 4, color);
    } else {
        gbar89_call_rect(ops, base, color);
    }
}

static void gbar89_draw_masked_color(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *base,
                                     int direction,
                                     GBar89_Fix ratio,
                                     unsigned long color,
                                     int draw_pattern)
{
    GBar89_Rect fill;

    if (m == 0 || ops == 0 || base == 0) {
        return;
    }

    fill = gbar89_fill_rect_for_ratio(base, direction, ratio);
    if (fill.w <= 0 || fill.h <= 0) {
        return;
    }

    if ((m->flags & GBAR89_FLAG_USE_MASK) == 0 ||
        m->mask_kind == GBAR89_MASK_NONE) {
        gbar89_call_rect(ops, &fill, color);
        if (draw_pattern) {
            gbar89_draw_pattern_rect(m, ops, &fill);
        }
        return;
    }

    if (m->mask_kind == GBAR89_MASK_CUSTOM_SLICES) {
        gbar89_draw_custom_slices(m, ops, base, &fill, color, draw_pattern);
        return;
    }

    if (ops->draw_triangles != 0 && ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->push_clip(ops->user, fill.x, fill.y, fill.w, fill.h);
        gbar89_draw_mask_shape(m, ops, base, color);
        ops->pop_clip(ops->user);
        if (draw_pattern) {
            gbar89_draw_pattern_rect(m, ops, &fill);
        }
    } else {
        gbar89_call_rect(ops, &fill, color);
        if (draw_pattern) {
            gbar89_draw_pattern_rect(m, ops, &fill);
        }
    }
}


static void gbar89_draw_band_piece(const GBar89_Meter *m,
                                   const GBar89_RenderOps *ops,
                                   const GBar89_Rect *base,
                                   const GBar89_Rect *piece,
                                   unsigned long color)
{
    if (m == 0 || ops == 0 || base == 0 || piece == 0) {
        return;
    }
    if (piece->w <= 0 || piece->h <= 0) {
        return;
    }

    if ((m->flags & GBAR89_FLAG_USE_MASK) != 0 &&
        m->mask_kind != GBAR89_MASK_NONE &&
        m->mask_kind != GBAR89_MASK_CUSTOM_SLICES &&
        ops->draw_triangles != 0 &&
        ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->push_clip(ops->user, piece->x, piece->y, piece->w, piece->h);
        gbar89_draw_mask_shape(m, ops, base, color);
        ops->pop_clip(ops->user);
    } else {
        gbar89_call_rect(ops, piece, color);
    }
}

static void gbar89_draw_ratio_band(const GBar89_Meter *m,
                                   const GBar89_RenderOps *ops,
                                   const GBar89_Rect *base,
                                   int direction,
                                   GBar89_Fix a,
                                   GBar89_Fix b,
                                   unsigned long color)
{
    GBar89_Fix lo;
    GBar89_Fix hi;
    int p0;
    int p1;
    int n0;
    int n1;
    int cx;
    int cy;
    GBar89_Rect q;

    if (m == 0 || ops == 0 || base == 0) {
        return;
    }
    lo = a;
    hi = b;
    if (lo > hi) {
        GBar89_Fix t;
        t = lo;
        lo = hi;
        hi = t;
    }
    lo = gbar89_fix_clamp01(lo);
    hi = gbar89_fix_clamp01(hi);
    if (hi <= lo) {
        return;
    }

    if (direction == GBAR89_DIR_TOP_TO_BOTTOM ||
        direction == GBAR89_DIR_BOTTOM_TO_TOP ||
        direction == GBAR89_DIR_CENTER_VERTICAL) {
        p0 = gbar89_ratio_to_pixels(lo, base->h);
        p1 = gbar89_ratio_to_pixels(hi, base->h);
    } else {
        p0 = gbar89_ratio_to_pixels(lo, base->w);
        p1 = gbar89_ratio_to_pixels(hi, base->w);
    }

    if (direction == GBAR89_DIR_LEFT_TO_RIGHT) {
        q = gbar89_rect_make(base->x + p0, base->y, p1 - p0, base->h);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    } else if (direction == GBAR89_DIR_RIGHT_TO_LEFT) {
        q = gbar89_rect_make(base->x + base->w - p1, base->y,
                             p1 - p0, base->h);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    } else if (direction == GBAR89_DIR_TOP_TO_BOTTOM) {
        q = gbar89_rect_make(base->x, base->y + p0, base->w, p1 - p0);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    } else if (direction == GBAR89_DIR_BOTTOM_TO_TOP) {
        q = gbar89_rect_make(base->x, base->y + base->h - p1,
                             base->w, p1 - p0);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    } else if (direction == GBAR89_DIR_CENTER_HORIZONTAL) {
        cx = base->x + base->w / 2;
        n0 = p0 / 2;
        n1 = p1 / 2;
        q = gbar89_rect_make(cx - n1, base->y, n1 - n0, base->h);
        gbar89_draw_band_piece(m, ops, base, &q, color);
        q = gbar89_rect_make(cx + n0, base->y, n1 - n0, base->h);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    } else if (direction == GBAR89_DIR_CENTER_VERTICAL) {
        cy = base->y + base->h / 2;
        n0 = p0 / 2;
        n1 = p1 / 2;
        q = gbar89_rect_make(base->x, cy - n1, base->w, n1 - n0);
        gbar89_draw_band_piece(m, ops, base, &q, color);
        q = gbar89_rect_make(base->x, cy + n0, base->w, n1 - n0);
        gbar89_draw_band_piece(m, ops, base, &q, color);
    }
}

static void gbar89_draw_vector_glyph(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *slot,
                                     unsigned long fill_color)
{
    GBar89_Vertex vertices[GBAR89_MAX_VECTOR_POINTS];
    int indices[(GBAR89_MAX_VECTOR_POINTS - 2) * 3];
    int px[GBAR89_MAX_VECTOR_POINTS];
    int py[GBAR89_MAX_VECTOR_POINTS];
    int n;
    int i;
    int idx;
    int gw;
    int gh;
    int gx;
    int gy;
    int scale;

    if (m == 0 || ops == 0 || slot == 0) {
        return;
    }
    if (m->unit_renderer != 0) {
        m->unit_renderer(m->unit_renderer_user, ops, slot, fill_color,
                         m->style.color_unit_outline,
                         m->unit_scale_percent);
        return;
    }
    if (m->unit_points == 0) {
        return;
    }
    n = m->unit_point_count;
    if (n < 2) {
        return;
    }
    if (n > GBAR89_MAX_VECTOR_POINTS) {
        n = GBAR89_MAX_VECTOR_POINTS;
    }

    scale = m->unit_scale_percent;
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 100) {
        scale = 100;
    }
    gw = (slot->w * scale) / 100;
    gh = (slot->h * scale) / 100;
    if (gw < 1) {
        gw = 1;
    }
    if (gh < 1) {
        gh = 1;
    }
    gx = slot->x + (slot->w - gw) / 2;
    gy = slot->y + (slot->h - gh) / 2;

    for (i = 0; i < n; ++i) {
        px[i] = gx + (m->unit_points[i].x * gw) / 1000;
        py[i] = gy + (m->unit_points[i].y * gh) / 1000;
        vertices[i].x = px[i];
        vertices[i].y = py[i];
        vertices[i].u = 0;
        vertices[i].v = 0;
        vertices[i].color = fill_color;
    }

    if (m->unit_filled != 0 && n >= 3 && ops->draw_triangles != 0) {
        idx = 0;
        for (i = 1; i < n - 1; ++i) {
            indices[idx++] = 0;
            indices[idx++] = i;
            indices[idx++] = i + 1;
        }
        ops->draw_triangles(ops->user, vertices, n, indices, idx,
                            GBAR89_INVALID_SPRITE);
    }

    if (ops->draw_line != 0) {
        for (i = 0; i < n - 1; ++i) {
            gbar89_call_line(ops, px[i], py[i], px[i + 1], py[i + 1],
                             m->style.color_unit_outline);
        }
        if (m->unit_closed != 0) {
            gbar89_call_line(ops, px[n - 1], py[n - 1], px[0], py[0],
                             m->style.color_unit_outline);
        }
    }
}

static void gbar89_draw_vector_units(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *inner)
{
    int count;
    int i;
    int logical;
    int reverse;
    int vertical;
    int axis;
    int each;
    int extra;
    int pos;
    int add;
    GBar89_Fix ratio;
    GBar89_Fix center;
    GBar89_Rect slot;
    unsigned long color;

    if (m == 0 || ops == 0 || inner == 0) {
        return;
    }
    if ((m->flags & GBAR89_FLAG_DRAW_VECTOR_UNITS) == 0 ||
        (m->unit_renderer == 0 &&
         (m->unit_points == 0 || m->unit_point_count < 2))) {
        return;
    }
    count = m->unit_count;
    if (count < 1) {
        return;
    }

    vertical = (m->direction == GBAR89_DIR_TOP_TO_BOTTOM ||
                m->direction == GBAR89_DIR_BOTTOM_TO_TOP ||
                m->direction == GBAR89_DIR_CENTER_VERTICAL);
    reverse = (m->direction == GBAR89_DIR_RIGHT_TO_LEFT ||
               m->direction == GBAR89_DIR_BOTTOM_TO_TOP);
    axis = vertical ? inner->h : inner->w;
    if (axis < count) {
        count = axis;
    }
    if (count < 1) {
        return;
    }
    each = axis / count;
    extra = axis % count;
    pos = vertical ? inner->y : inner->x;
    ratio = gbar89_current_ratio(m);

    for (i = 0; i < count; ++i) {
        add = each + ((i < extra) ? 1 : 0);
        if (vertical) {
            slot = gbar89_rect_make(inner->x + m->unit_padding,
                                    pos + m->unit_padding,
                                    inner->w - m->unit_padding * 2,
                                    add - m->unit_padding * 2);
        } else {
            slot = gbar89_rect_make(pos + m->unit_padding,
                                    inner->y + m->unit_padding,
                                    add - m->unit_padding * 2,
                                    inner->h - m->unit_padding * 2);
        }
        logical = reverse ? (count - 1 - i) : i;
        center = (GBar89_Fix)(((long)(logical * 2 + 1) * GBAR89_FIX_ONE) /
                              (count * 2));
        color = (ratio >= center) ? m->style.color_unit_fill
                                  : m->style.color_unit_empty;
        gbar89_draw_vector_glyph(m, ops, &slot, color);
        pos += add;
    }
}

static void gbar89_draw_marker_lines(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *inner)
{
    int i;
    int p;
    int s;
    GBar89_Fix ratio;
    GBar89_Rect r;

    if (m == 0 || ops == 0 || inner == 0) {
        return;
    }
    if ((m->flags & GBAR89_FLAG_DRAW_MARKERS) == 0) {
        return;
    }

    s = m->style.marker_size;
    if (s <= 0) {
        s = 1;
    }

    for (i = 0; i < m->marker_count; ++i) {
        ratio = gbar89_ratio_from_value(m, m->markers[i]);

        if (m->direction == GBAR89_DIR_TOP_TO_BOTTOM ||
            m->direction == GBAR89_DIR_BOTTOM_TO_TOP ||
            m->direction == GBAR89_DIR_CENTER_VERTICAL) {
            p = gbar89_ratio_to_pixels(ratio, inner->h);
            if (m->direction == GBAR89_DIR_BOTTOM_TO_TOP) {
                p = inner->h - p;
            }
            r = gbar89_rect_make(inner->x, inner->y + p - s / 2,
                                 inner->w, s);
        } else {
            p = gbar89_ratio_to_pixels(ratio, inner->w);
            if (m->direction == GBAR89_DIR_RIGHT_TO_LEFT) {
                p = inner->w - p;
            }
            r = gbar89_rect_make(inner->x + p - s / 2, inner->y,
                                 s, inner->h);
        }

        gbar89_call_rect(ops, &r, m->style.color_marker);
    }
}

static void gbar89_draw_layer_pips(const GBar89_Meter *m,
                                   const GBar89_RenderOps *ops)
{
    int filled;
    int i;
    int s;
    int gap;
    GBar89_Rect r;
    unsigned long color;

    if (m == 0 || ops == 0) {
        return;
    }
    if ((m->flags & GBAR89_FLAG_DRAW_LAYER_PIPS) == 0) {
        return;
    }
    if ((m->flags & GBAR89_FLAG_LAYERED) == 0) {
        return;
    }

    filled = gbar89_layer_count_filled(m, m->value);
    s = m->rect.h;
    if (s < 4) {
        s = 4;
    }
    gap = 2;

    for (i = 0; i < m->layer_count; ++i) {
        r.x = m->rect.x + m->rect.w + gap + i * (s + gap);
        r.y = m->rect.y;
        r.w = s;
        r.h = m->rect.h;
        if (i < filled) {
            color = m->style.color_layer_fill[i];
            if (color == 0UL) {
                color = m->style.color_fill;
            }
        } else {
            color = m->style.color_empty;
        }
        gbar89_call_rect(ops, &r, color);
    }
}

static void gbar89_draw_linear_rects(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops)
{
    GBar89_Rect inner;
    GBar89_Rect fill_rect;
    GBar89_Fix ratio;
    GBar89_Fix lag_ratio;
    GBar89_Fix overlay_ratio;
    GBar89_Fix mid_ratio;
    long fill_value;
    unsigned long fill_color;
    unsigned long lag_color;

    if (m == 0 || ops == 0) {
        return;
    }

    inner = gbar89_inner_rect(m);

    if (m->flags & GBAR89_FLAG_DRAW_BG) {
        gbar89_draw_masked_color(m, ops, &inner, m->direction,
                                 GBAR89_FIX_ONE, m->style.color_bg, 0);
        gbar89_draw_background_detail(m, ops, &inner);
    }

    if (m->flags & GBAR89_FLAG_DAMAGE_LAG) {
        lag_ratio = gbar89_display_ratio_from_value(m, m->lag_value);
        lag_ratio = gbar89_quantize_ratio(m, &inner, m->direction, lag_ratio);
        lag_color = gbar89_lag_color_for_value(m, m->lag_value);
        gbar89_draw_masked_color(m, ops, &inner, m->direction,
                                 lag_ratio, lag_color, 0);
    }

    if ((m->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        fill_value = m->visual_value;
    } else {
        fill_value = m->value;
    }
    ratio = gbar89_current_ratio(m);
    ratio = gbar89_quantize_ratio(m, &inner, m->direction, ratio);
    fill_color = gbar89_fill_color_for_value(m, fill_value);
    gbar89_draw_masked_color(m, ops, &inner, m->direction,
                             ratio, fill_color, 1);
    fill_rect = gbar89_fill_rect_for_ratio(&inner, m->direction, ratio);
    gbar89_draw_fill_fx(m, ops, &fill_rect);

    if ((m->flags & GBAR89_FLAG_DRAW_MID_VALUE) != 0) {
        mid_ratio = gbar89_mid_ratio(m);
        mid_ratio = gbar89_quantize_ratio(m, &inner, m->mid_direction,
                                          mid_ratio);
        gbar89_draw_ratio_band(m, ops, &inner, m->mid_direction,
                               ratio, mid_ratio, m->style.color_mid);
    }

    if (m->flags & GBAR89_FLAG_DRAW_VALUE_OVERLAY) {
        overlay_ratio = gbar89_overlay_ratio(m);
        overlay_ratio = gbar89_quantize_ratio(m, &inner,
                                              m->overlay_direction,
                                              overlay_ratio);
        gbar89_draw_masked_color(m, ops, &inner, m->overlay_direction,
                                 overlay_ratio, m->style.color_overlay, 1);
    }

    gbar89_draw_vector_units(m, ops, &inner);
    gbar89_draw_marker_lines(m, ops, &inner);
    gbar89_draw_layer_pips(m, ops);

    if (m->flags & GBAR89_FLAG_DRAW_BORDER) {
        gbar89_draw_border(m, ops);
    }
}

static GBar89_Rect gbar89_sprite_src_for_ratio(const GBar89_Rect *src,
                                               int direction,
                                               GBar89_Fix ratio)
{
    GBar89_Rect r;
    int n;
    int cx;
    int cy;

    r = *src;

    if (ratio <= 0) {
        r.w = 0;
        r.h = 0;
        return r;
    }
    if (ratio > GBAR89_FIX_ONE) {
        ratio = GBAR89_FIX_ONE;
    }

    if (direction == GBAR89_DIR_LEFT_TO_RIGHT) {
        r.w = gbar89_ratio_to_pixels(ratio, src->w);
    } else if (direction == GBAR89_DIR_RIGHT_TO_LEFT) {
        n = gbar89_ratio_to_pixels(ratio, src->w);
        r.x = src->x + src->w - n;
        r.w = n;
    } else if (direction == GBAR89_DIR_TOP_TO_BOTTOM) {
        r.h = gbar89_ratio_to_pixels(ratio, src->h);
    } else if (direction == GBAR89_DIR_BOTTOM_TO_TOP) {
        n = gbar89_ratio_to_pixels(ratio, src->h);
        r.y = src->y + src->h - n;
        r.h = n;
    } else if (direction == GBAR89_DIR_CENTER_HORIZONTAL) {
        n = gbar89_ratio_to_pixels(ratio, src->w);
        cx = src->x + src->w / 2;
        r.x = cx - n / 2;
        r.w = n;
    } else if (direction == GBAR89_DIR_CENTER_VERTICAL) {
        n = gbar89_ratio_to_pixels(ratio, src->h);
        cy = src->y + src->h / 2;
        r.y = cy - n / 2;
        r.h = n;
    }

    return r;
}

static void gbar89_draw_sprite_fill_dir(const GBar89_Meter *m,
                                        const GBar89_RenderOps *ops,
                                        int sprite_id,
                                        const GBar89_Rect *src,
                                        const GBar89_Rect *dst,
                                        int direction,
                                        GBar89_Fix ratio,
                                        unsigned long tint)
{
    GBar89_Rect src_clip;
    GBar89_Rect dst_clip;

    if (m == 0 || ops == 0 || src == 0 || dst == 0) {
        return;
    }

    if (sprite_id == GBAR89_INVALID_SPRITE || ops->draw_sprite == 0) {
        return;
    }

    src_clip = gbar89_sprite_src_for_ratio(src, direction, ratio);
    dst_clip = gbar89_fill_rect_for_ratio(dst, direction, ratio);

    gbar89_call_sprite(ops, sprite_id, &src_clip, &dst_clip, tint);
}

static void gbar89_draw_sprite_clip(const GBar89_Meter *m,
                                    const GBar89_RenderOps *ops)
{
    GBar89_Rect inner;
    GBar89_Fix ratio;
    GBar89_Fix lag_ratio;
    GBar89_Fix overlay_ratio;
    long fill_value;
    unsigned long fill_color;

    if (m == 0 || ops == 0) {
        return;
    }

    inner = gbar89_inner_rect(m);

    if ((m->flags & GBAR89_FLAG_DRAW_BG) != 0 &&
        m->style.sprite_bg != GBAR89_INVALID_SPRITE) {
        gbar89_call_sprite(ops, m->style.sprite_bg,
                           &m->style.src_bg, &inner,
                           0xFFFFFFFFUL);
    } else if (m->flags & GBAR89_FLAG_DRAW_BG) {
        gbar89_call_rect(ops, &inner, m->style.color_bg);
    }

    if ((m->flags & GBAR89_FLAG_DAMAGE_LAG) != 0 &&
        m->style.sprite_lag != GBAR89_INVALID_SPRITE) {
        lag_ratio = gbar89_display_ratio_from_value(m, m->lag_value);
        gbar89_draw_sprite_fill_dir(m, ops, m->style.sprite_lag,
                                    &m->style.src_lag, &inner,
                                    m->direction, lag_ratio, 0xFFFFFFFFUL);
    }

    ratio = gbar89_current_ratio(m);
    if ((m->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        fill_value = m->visual_value;
    } else {
        fill_value = m->value;
    }
    fill_color = gbar89_fill_color_for_value(m, fill_value);

    if (m->style.sprite_fill != GBAR89_INVALID_SPRITE) {
        gbar89_draw_sprite_fill_dir(m, ops, m->style.sprite_fill,
                                    &m->style.src_fill, &inner,
                                    m->direction, ratio, fill_color);
    } else {
        gbar89_draw_masked_color(m, ops, &inner, m->direction,
                                 ratio, fill_color, 1);
    }

    if (m->flags & GBAR89_FLAG_DRAW_VALUE_OVERLAY) {
        overlay_ratio = gbar89_overlay_ratio(m);
        if (m->style.sprite_overlay != GBAR89_INVALID_SPRITE) {
            gbar89_draw_sprite_fill_dir(m, ops, m->style.sprite_overlay,
                                        &m->style.src_overlay, &inner,
                                        m->overlay_direction, overlay_ratio,
                                        0xFFFFFFFFUL);
        } else {
            gbar89_draw_masked_color(m, ops, &inner, m->overlay_direction,
                                     overlay_ratio, m->style.color_overlay, 1);
        }
    }

    if ((m->flags & GBAR89_FLAG_DRAW_OVERLAY) != 0 &&
        m->style.sprite_overlay != GBAR89_INVALID_SPRITE) {
        gbar89_call_sprite(ops, m->style.sprite_overlay,
                           &m->style.src_overlay, &inner,
                           0xFFFFFFFFUL);
    }

    gbar89_draw_marker_lines(m, ops, &inner);
    gbar89_draw_layer_pips(m, ops);

    if (m->flags & GBAR89_FLAG_DRAW_BORDER) {
        gbar89_draw_border(m, ops);
    }
}

static void gbar89_draw_nineslice_sprite(const GBar89_RenderOps *ops,
                                         int sprite_id,
                                         const GBar89_Rect *src,
                                         const GBar89_Rect *dst,
                                         const GBar89_Style *style,
                                         unsigned long tint)
{
    int sx[3];
    int sy[3];
    int sw[3];
    int sh[3];
    int dx[3];
    int dy[3];
    int dw[3];
    int dh[3];
    int i;
    int j;
    GBar89_Rect s;
    GBar89_Rect d;

    if (ops == 0 || src == 0 || dst == 0 || style == 0) {
        return;
    }
    if (sprite_id == GBAR89_INVALID_SPRITE) {
        return;
    }
    if (ops->draw_sprite == 0) {
        return;
    }

    sx[0] = src->x;
    sx[1] = src->x + style->margin_left;
    sx[2] = src->x + src->w - style->margin_right;

    sy[0] = src->y;
    sy[1] = src->y + style->margin_top;
    sy[2] = src->y + src->h - style->margin_bottom;

    sw[0] = style->margin_left;
    sw[1] = src->w - style->margin_left - style->margin_right;
    sw[2] = style->margin_right;

    sh[0] = style->margin_top;
    sh[1] = src->h - style->margin_top - style->margin_bottom;
    sh[2] = style->margin_bottom;

    dx[0] = dst->x;
    dx[1] = dst->x + style->margin_left;
    dx[2] = dst->x + dst->w - style->margin_right;

    dy[0] = dst->y;
    dy[1] = dst->y + style->margin_top;
    dy[2] = dst->y + dst->h - style->margin_bottom;

    dw[0] = style->margin_left;
    dw[1] = dst->w - style->margin_left - style->margin_right;
    dw[2] = style->margin_right;

    dh[0] = style->margin_top;
    dh[1] = dst->h - style->margin_top - style->margin_bottom;
    dh[2] = style->margin_bottom;

    for (i = 0; i < 3; ++i) {
        if (sw[i] < 0) {
            sw[i] = 0;
        }
        if (dw[i] < 0) {
            dw[i] = 0;
        }
        if (sh[i] < 0) {
            sh[i] = 0;
        }
        if (dh[i] < 0) {
            dh[i] = 0;
        }
    }

    for (j = 0; j < 3; ++j) {
        for (i = 0; i < 3; ++i) {
            s = gbar89_rect_make(sx[i], sy[j], sw[i], sh[j]);
            d = gbar89_rect_make(dx[i], dy[j], dw[i], dh[j]);
            gbar89_call_sprite(ops, sprite_id, &s, &d, tint);
        }
    }
}

static void gbar89_draw_nineslice(const GBar89_Meter *m,
                                  const GBar89_RenderOps *ops)
{
    GBar89_Rect inner;
    GBar89_Rect fill;
    GBar89_Rect lag;
    GBar89_Rect overlay;
    GBar89_Fix ratio;
    GBar89_Fix lag_ratio;
    GBar89_Fix overlay_ratio;
    long fill_value;
    unsigned long fill_color;
    unsigned long lag_color;

    if (m == 0 || ops == 0) {
        return;
    }

    inner = gbar89_inner_rect(m);

    if ((m->flags & GBAR89_FLAG_DRAW_BG) != 0 &&
        m->style.sprite_bg != GBAR89_INVALID_SPRITE) {
        gbar89_draw_nineslice_sprite(ops, m->style.sprite_bg,
                                     &m->style.src_bg, &inner,
                                     &m->style, 0xFFFFFFFFUL);
    } else if (m->flags & GBAR89_FLAG_DRAW_BG) {
        gbar89_call_rect(ops, &inner, m->style.color_bg);
    }

    if ((m->flags & GBAR89_FLAG_DAMAGE_LAG) != 0) {
        lag_ratio = gbar89_display_ratio_from_value(m, m->lag_value);
        lag = gbar89_fill_rect_for_ratio(&inner, m->direction, lag_ratio);
        lag_color = gbar89_lag_color_for_value(m, m->lag_value);
        if (m->style.sprite_lag != GBAR89_INVALID_SPRITE &&
            ops->push_clip != 0 && ops->pop_clip != 0) {
            ops->push_clip(ops->user, lag.x, lag.y, lag.w, lag.h);
            gbar89_draw_nineslice_sprite(ops, m->style.sprite_lag,
                                         &m->style.src_lag, &inner,
                                         &m->style, 0xFFFFFFFFUL);
            ops->pop_clip(ops->user);
        } else {
            gbar89_draw_masked_color(m, ops, &inner, m->direction,
                                     lag_ratio, lag_color, 0);
        }
    }

    ratio = gbar89_current_ratio(m);
    fill = gbar89_fill_rect_for_ratio(&inner, m->direction, ratio);
    if ((m->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        fill_value = m->visual_value;
    } else {
        fill_value = m->value;
    }
    fill_color = gbar89_fill_color_for_value(m, fill_value);

    if (m->style.sprite_fill != GBAR89_INVALID_SPRITE &&
        ops->push_clip != 0 && ops->pop_clip != 0) {
        ops->push_clip(ops->user, fill.x, fill.y, fill.w, fill.h);
        gbar89_draw_nineslice_sprite(ops, m->style.sprite_fill,
                                     &m->style.src_fill, &inner,
                                     &m->style, fill_color);
        ops->pop_clip(ops->user);
        gbar89_draw_pattern_rect(m, ops, &fill);
    } else {
        gbar89_draw_masked_color(m, ops, &inner, m->direction,
                                 ratio, fill_color, 1);
    }

    if (m->flags & GBAR89_FLAG_DRAW_VALUE_OVERLAY) {
        overlay_ratio = gbar89_overlay_ratio(m);
        overlay = gbar89_fill_rect_for_ratio(&inner, m->overlay_direction,
                                             overlay_ratio);
        if (m->style.sprite_overlay != GBAR89_INVALID_SPRITE &&
            ops->push_clip != 0 && ops->pop_clip != 0) {
            ops->push_clip(ops->user, overlay.x, overlay.y,
                           overlay.w, overlay.h);
            gbar89_draw_nineslice_sprite(ops, m->style.sprite_overlay,
                                         &m->style.src_overlay, &inner,
                                         &m->style, 0xFFFFFFFFUL);
            ops->pop_clip(ops->user);
        } else {
            gbar89_draw_masked_color(m, ops, &inner, m->overlay_direction,
                                     overlay_ratio, m->style.color_overlay, 1);
        }
    }

    if ((m->flags & GBAR89_FLAG_DRAW_OVERLAY) != 0 &&
        m->style.sprite_overlay != GBAR89_INVALID_SPRITE) {
        gbar89_draw_nineslice_sprite(ops, m->style.sprite_overlay,
                                     &m->style.src_overlay, &inner,
                                     &m->style, 0xFFFFFFFFUL);
    }

    gbar89_draw_marker_lines(m, ops, &inner);
    gbar89_draw_layer_pips(m, ops);

    if (m->flags & GBAR89_FLAG_DRAW_BORDER) {
        gbar89_draw_border(m, ops);
    }
}

static void gbar89_draw_segment_fill(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     const GBar89_Rect *seg_rect,
                                     int direction,
                                     GBar89_Fix seg_ratio,
                                     unsigned long color)
{
    GBar89_Rect full_src;

    if (m == 0 || ops == 0 || seg_rect == 0) {
        return;
    }

    if (m->style.sprite_full != GBAR89_INVALID_SPRITE &&
        gbar89_rect_valid(&m->style.src_full)) {
        full_src = m->style.src_full;
        gbar89_draw_sprite_fill_dir(m, ops, m->style.sprite_full,
                                    &full_src, seg_rect,
                                    direction,
                                    seg_ratio, 0xFFFFFFFFUL);
    } else {
        gbar89_draw_masked_color(m, ops, seg_rect,
                                 direction,
                                 seg_ratio, color, 1);
    }
}

static void gbar89_draw_segmented(const GBar89_Meter *m,
                                  const GBar89_RenderOps *ops)
{
    GBar89_Rect inner;
    GBar89_Rect seg;
    GBar89_Fix total_units;
    GBar89_Fix seg_start;
    GBar89_Fix seg_end;
    GBar89_Fix seg_ratio;
    GBar89_Fix main_ratio;
    GBar89_Fix mid_ratio;
    GBar89_Rect fx_rect;
    int i;
    int count;
    int gap;
    int axis_total;
    int each;
    int extra;
    int pos;
    int vertical;
    unsigned long fill_color;
    long fill_value;

    if (m == 0 || ops == 0) {
        return;
    }

    count = m->segments;
    if (count < 1) {
        count = 1;
    }

    gap = m->style.segment_gap;
    if (gap < 0) {
        gap = 0;
    }

    inner = gbar89_inner_rect(m);
    vertical = 0;
    if (m->direction == GBAR89_DIR_TOP_TO_BOTTOM ||
        m->direction == GBAR89_DIR_BOTTOM_TO_TOP ||
        m->direction == GBAR89_DIR_CENTER_VERTICAL) {
        vertical = 1;
    }

    axis_total = vertical ? inner.h : inner.w;
    axis_total -= gap * (count - 1);
    if (axis_total < count) {
        axis_total = count;
    }

    each = axis_total / count;
    extra = axis_total % count;

    main_ratio = gbar89_current_ratio(m);
    main_ratio = gbar89_quantize_ratio(m, &inner, m->direction, main_ratio);
    total_units = main_ratio * count;
    if ((m->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        fill_value = m->visual_value;
    } else {
        fill_value = m->value;
    }
    fill_color = gbar89_fill_color_for_value(m, fill_value);

    pos = vertical ? inner.y : inner.x;

    for (i = 0; i < count; ++i) {
        int add;

        add = each;
        if (i < extra) {
            add += 1;
        }

        if (vertical) {
            if (m->direction == GBAR89_DIR_BOTTOM_TO_TOP) {
                seg = gbar89_rect_make(inner.x,
                                       inner.y + inner.h - (pos - inner.y) - add,
                                       inner.w,
                                       add);
            } else {
                seg = gbar89_rect_make(inner.x, pos, inner.w, add);
            }
        } else {
            if (m->direction == GBAR89_DIR_RIGHT_TO_LEFT) {
                seg = gbar89_rect_make(inner.x + inner.w - (pos - inner.x) - add,
                                       inner.y,
                                       add,
                                       inner.h);
            } else {
                seg = gbar89_rect_make(pos, inner.y, add, inner.h);
            }
        }

        if (m->style.sprite_empty != GBAR89_INVALID_SPRITE &&
            gbar89_rect_valid(&m->style.src_empty)) {
            gbar89_call_sprite(ops, m->style.sprite_empty,
                               &m->style.src_empty, &seg,
                               0xFFFFFFFFUL);
        } else {
            gbar89_call_rect(ops, &seg, m->style.color_empty);
        }
        gbar89_draw_background_detail(m, ops, &seg);

        seg_start = ((GBar89_Fix)i) << GBAR89_FIX_SHIFT;
        seg_end = ((GBar89_Fix)(i + 1)) << GBAR89_FIX_SHIFT;

        if (total_units >= seg_end) {
            seg_ratio = GBAR89_FIX_ONE;
        } else if (total_units <= seg_start) {
            seg_ratio = 0;
        } else {
            seg_ratio = total_units - seg_start;
        }

        if (seg_ratio > 0) {
            gbar89_draw_segment_fill(m, ops, &seg, m->direction,
                                     seg_ratio, fill_color);
            fx_rect = gbar89_fill_rect_for_ratio(&seg,
                                                 m->direction,
                                                 seg_ratio);
            gbar89_draw_fill_fx(m, ops, &fx_rect);
        }

        pos += add + gap;
    }

    if ((m->flags & GBAR89_FLAG_DRAW_MID_VALUE) != 0) {
        mid_ratio = gbar89_mid_ratio(m);
        mid_ratio = gbar89_quantize_ratio(m, &inner, m->mid_direction,
                                          mid_ratio);
        gbar89_draw_ratio_band(m, ops, &inner, m->mid_direction,
                               main_ratio, mid_ratio, m->style.color_mid);
    }

    if (m->flags & GBAR89_FLAG_DRAW_VALUE_OVERLAY) {
        GBar89_Fix overlay_ratio;
        GBar89_Rect overlay;

        overlay_ratio = gbar89_overlay_ratio(m);
        overlay = gbar89_fill_rect_for_ratio(&inner, m->overlay_direction,
                                             overlay_ratio);
        gbar89_call_rect(ops, &overlay, m->style.color_overlay);
        gbar89_draw_pattern_rect(m, ops, &overlay);
    }

    gbar89_draw_vector_units(m, ops, &inner);
    gbar89_draw_marker_lines(m, ops, &inner);
    gbar89_draw_layer_pips(m, ops);

    if (m->flags & GBAR89_FLAG_DRAW_BORDER) {
        gbar89_draw_border(m, ops);
    }
}

static void gbar89_angle_point_offset(const GBar89_Rect *rect,
                                      int angle_deg,
                                      int radius_percent,
                                      int offset_x,
                                      int offset_y,
                                      int *out_x,
                                      int *out_y)
{
    int cx;
    int cy;
    int rx;
    int ry;
    int c;
    int s;

    cx = rect->x + rect->w / 2 + offset_x;
    cy = rect->y + rect->h / 2 + offset_y;
    rx = (rect->w / 2) * radius_percent / 100;
    ry = (rect->h / 2) * radius_percent / 100;

    c = gbar89_cos_deg(angle_deg);
    s = gbar89_sin_deg(angle_deg);

    *out_x = cx + (rx * c) / GBAR89_TRIG_ONE;
    *out_y = cy + (ry * s) / GBAR89_TRIG_ONE;
}

static void gbar89_angle_point(const GBar89_Rect *rect,
                               int angle_deg,
                               int radius_percent,
                               int *out_x,
                               int *out_y)
{
    gbar89_angle_point_offset(rect, angle_deg, radius_percent,
                              0, 0, out_x, out_y);
}

static int gbar89_radial_base_inner(const GBar89_Meter *m)
{
    if (m->kind == GBAR89_KIND_RADIAL_RING) {
        return m->radial_inner_percent;
    }
    return 0;
}

static int gbar89_radial_px_percent(const GBar89_Meter *m, int px)
{
    GBar89_Rect inner;
    int radius;
    int p;

    inner = gbar89_inner_rect(m);
    radius = inner.w / 2;
    if (inner.h / 2 < radius) {
        radius = inner.h / 2;
    }
    if (radius < 1) {
        return 1;
    }
    p = (px * 100 + radius - 1) / radius;
    if (p < 1) {
        p = 1;
    }
    if (p > 40) {
        p = 40;
    }
    return p;
}

static GBar89_Fix gbar89_radial_quantize_ratio(const GBar89_Meter *m,
                                                GBar89_Fix ratio)
{
    int sweep;
    int filled;
    int cell;
    int q;

    if ((m->flags & GBAR89_FLAG_PIXEL_QUANTIZE) == 0) {
        return gbar89_fix_clamp01(ratio);
    }
    sweep = gbar89_abs_int(m->radial_sweep_deg);
    if (sweep < 1) {
        return 0;
    }
    cell = m->style.pixel_size;
    if (cell < 1) {
        cell = 1;
    }
    filled = (int)(((long)sweep * gbar89_fix_clamp01(ratio)) >>
                   GBAR89_FIX_SHIFT);
    q = (filled / cell) * cell;
    if (filled > 0 && q == 0) {
        q = cell;
    }
    if (q > sweep) {
        q = sweep;
    }
    return (GBar89_Fix)(((long)q << GBAR89_FIX_SHIFT) / sweep);
}

static void gbar89_draw_radial_arc_raw(const GBar89_Meter *m,
                                       const GBar89_RenderOps *ops,
                                       int start_deg,
                                       int sweep_deg,
                                       int outer_percent,
                                       int inner_percent,
                                       unsigned long color,
                                       int offset_x,
                                       int offset_y)
{
    GBar89_Vertex vertices[GBAR89_MAX_RADIAL_STEPS * 2 + 2];
    int indices[GBAR89_MAX_RADIAL_STEPS * 6];
    GBar89_Rect inner;
    int steps;
    int i;
    int angle;
    int vertex_count;
    int index_count;
    int cx;
    int cy;
    int base_sweep;

    if (m == 0 || ops == 0 || ops->draw_triangles == 0) {
        return;
    }
    if (sweep_deg == 0 || outer_percent <= 0) {
        return;
    }
    if (inner_percent < 0) {
        inner_percent = 0;
    }
    if (inner_percent >= outer_percent) {
        inner_percent = outer_percent - 1;
    }

    inner = gbar89_inner_rect(m);
    if (inner.w <= 0 || inner.h <= 0) {
        return;
    }

    base_sweep = gbar89_abs_int(m->radial_sweep_deg);
    if (base_sweep < 1) {
        base_sweep = 360;
    }
    steps = (gbar89_abs_int(sweep_deg) * m->radial_steps) / base_sweep;
    if (steps < 1) {
        steps = 1;
    }
    if (steps > GBAR89_MAX_RADIAL_STEPS) {
        steps = GBAR89_MAX_RADIAL_STEPS;
    }

    if (inner_percent > 0) {
        vertex_count = (steps + 1) * 2;
        index_count = steps * 6;

        for (i = 0; i <= steps; ++i) {
            angle = start_deg + (sweep_deg * i) / steps;
            gbar89_angle_point_offset(&inner, angle, outer_percent,
                                      offset_x, offset_y,
                                      &vertices[i * 2].x,
                                      &vertices[i * 2].y);
            vertices[i * 2].u = 0;
            vertices[i * 2].v = 0;
            vertices[i * 2].color = color;

            gbar89_angle_point_offset(&inner, angle, inner_percent,
                                      offset_x, offset_y,
                                      &vertices[i * 2 + 1].x,
                                      &vertices[i * 2 + 1].y);
            vertices[i * 2 + 1].u = 0;
            vertices[i * 2 + 1].v = 0;
            vertices[i * 2 + 1].color = color;
        }

        for (i = 0; i < steps; ++i) {
            indices[i * 6 + 0] = i * 2;
            indices[i * 6 + 1] = i * 2 + 1;
            indices[i * 6 + 2] = i * 2 + 2;
            indices[i * 6 + 3] = i * 2 + 2;
            indices[i * 6 + 4] = i * 2 + 1;
            indices[i * 6 + 5] = i * 2 + 3;
        }
    } else {
        cx = inner.x + inner.w / 2 + offset_x;
        cy = inner.y + inner.h / 2 + offset_y;
        vertices[0].x = cx;
        vertices[0].y = cy;
        vertices[0].u = 0;
        vertices[0].v = 0;
        vertices[0].color = color;

        for (i = 0; i <= steps; ++i) {
            angle = start_deg + (sweep_deg * i) / steps;
            gbar89_angle_point_offset(&inner, angle, outer_percent,
                                      offset_x, offset_y,
                                      &vertices[i + 1].x,
                                      &vertices[i + 1].y);
            vertices[i + 1].u = 0;
            vertices[i + 1].v = 0;
            vertices[i + 1].color = color;
        }

        vertex_count = steps + 2;
        index_count = steps * 3;
        for (i = 0; i < steps; ++i) {
            indices[i * 3 + 0] = 0;
            indices[i * 3 + 1] = i + 1;
            indices[i * 3 + 2] = i + 2;
        }
    }

    ops->draw_triangles(ops->user, vertices, vertex_count, indices,
                        index_count, GBAR89_INVALID_SPRITE);
}

static void gbar89_draw_radial_disc(const GBar89_RenderOps *ops,
                                    int cx, int cy, int radius,
                                    unsigned long color)
{
    GBar89_Vertex vertices[18];
    int indices[48];
    int i;
    int angle;

    if (ops == 0 || ops->draw_triangles == 0 || radius < 1) {
        return;
    }
    vertices[0].x = cx;
    vertices[0].y = cy;
    vertices[0].u = 0;
    vertices[0].v = 0;
    vertices[0].color = color;
    for (i = 0; i <= 16; ++i) {
        angle = (360 * i) / 16;
        vertices[i + 1].x = cx + (radius * gbar89_cos_deg(angle)) /
                                  GBAR89_TRIG_ONE;
        vertices[i + 1].y = cy + (radius * gbar89_sin_deg(angle)) /
                                  GBAR89_TRIG_ONE;
        vertices[i + 1].u = 0;
        vertices[i + 1].v = 0;
        vertices[i + 1].color = color;
    }
    for (i = 0; i < 16; ++i) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }
    ops->draw_triangles(ops->user, vertices, 18, indices, 48,
                        GBAR89_INVALID_SPRITE);
}

static void gbar89_draw_radial_square_cap(const GBar89_RenderOps *ops,
                                          int cx, int cy,
                                          int angle_deg,
                                          int half_size,
                                          unsigned long color)
{
    GBar89_Vertex v[4];
    int idx[6];
    int c;
    int s;
    int rx;
    int ry;
    int tx;
    int ty;
    int i;

    if (ops == 0 || ops->draw_triangles == 0 || half_size < 1) {
        return;
    }
    c = gbar89_cos_deg(angle_deg);
    s = gbar89_sin_deg(angle_deg);
    rx = (c * half_size) / GBAR89_TRIG_ONE;
    ry = (s * half_size) / GBAR89_TRIG_ONE;
    tx = (-s * half_size) / GBAR89_TRIG_ONE;
    ty = (c * half_size) / GBAR89_TRIG_ONE;

    v[0].x = cx - rx - tx;
    v[0].y = cy - ry - ty;
    v[1].x = cx + rx - tx;
    v[1].y = cy + ry - ty;
    v[2].x = cx + rx + tx;
    v[2].y = cy + ry + ty;
    v[3].x = cx - rx + tx;
    v[3].y = cy - ry + ty;
    for (i = 0; i < 4; ++i) {
        v[i].u = 0;
        v[i].v = 0;
        v[i].color = color;
    }
    idx[0] = 0;
    idx[1] = 1;
    idx[2] = 2;
    idx[3] = 0;
    idx[4] = 2;
    idx[5] = 3;
    ops->draw_triangles(ops->user, v, 4, idx, 6,
                        GBAR89_INVALID_SPRITE);
}

static void gbar89_draw_radial_caps(const GBar89_Meter *m,
                                    const GBar89_RenderOps *ops,
                                    int start_deg,
                                    int sweep_deg,
                                    int outer_percent,
                                    int inner_percent,
                                    unsigned long color,
                                    int offset_x,
                                    int offset_y)
{
    GBar89_Rect inner;
    int radius;
    int thickness;
    int mid_percent;
    int x;
    int y;
    int angle;
    int i;

    if (m->style.radial_cap_kind == GBAR89_RADIAL_CAP_BUTT ||
        inner_percent <= 0 ||
        gbar89_abs_int(sweep_deg) >= 360) {
        return;
    }
    inner = gbar89_inner_rect(m);
    radius = inner.w / 2;
    if (inner.h / 2 < radius) {
        radius = inner.h / 2;
    }
    thickness = radius * (outer_percent - inner_percent) / 100;
    if (thickness < 2) {
        thickness = 2;
    }
    mid_percent = (outer_percent + inner_percent) / 2;

    for (i = 0; i < 2; ++i) {
        angle = (i == 0) ? start_deg : start_deg + sweep_deg;
        gbar89_angle_point_offset(&inner, angle, mid_percent,
                                  offset_x, offset_y, &x, &y);
        if (m->style.radial_cap_kind == GBAR89_RADIAL_CAP_ROUND) {
            gbar89_draw_radial_disc(ops, x, y, thickness / 2, color);
        } else {
            gbar89_draw_radial_square_cap(ops, x, y, angle,
                                          thickness / 2, color);
        }
    }
}

static void gbar89_draw_radial_range_shape(const GBar89_Meter *m,
                                            const GBar89_RenderOps *ops,
                                            GBar89_Fix ratio0,
                                            GBar89_Fix ratio1,
                                            int outer_percent,
                                            int inner_percent,
                                            unsigned long color,
                                            int offset_x,
                                            int offset_y,
                                            int draw_caps)
{
    int total;
    int sign;
    int begin_abs;
    int end_abs;
    int segments;
    int cell;
    int gap;
    int i;
    int seg0;
    int seg1;
    int lo;
    int hi;
    int start;
    int sweep;

    ratio0 = gbar89_fix_clamp01(ratio0);
    ratio1 = gbar89_fix_clamp01(ratio1);
    if (ratio1 < ratio0) {
        GBar89_Fix t;
        t = ratio0;
        ratio0 = ratio1;
        ratio1 = t;
    }
    if (ratio1 <= ratio0) {
        return;
    }

    total = gbar89_abs_int(m->radial_sweep_deg);
    if (total < 1) {
        return;
    }
    sign = (m->radial_sweep_deg < 0) ? -1 : 1;
    begin_abs = (int)(((long)total * ratio0) >> GBAR89_FIX_SHIFT);
    end_abs = (int)(((long)total * ratio1) >> GBAR89_FIX_SHIFT);
    if (end_abs <= begin_abs) {
        end_abs = begin_abs + 1;
    }
    if (end_abs > total) {
        end_abs = total;
    }

    segments = m->style.radial_segments;
    if (segments < 2) {
        start = m->radial_start_deg + sign * begin_abs;
        sweep = sign * (end_abs - begin_abs);
        gbar89_draw_radial_arc_raw(m, ops, start, sweep,
                                   outer_percent, inner_percent,
                                   color, offset_x, offset_y);
        if (draw_caps) {
            gbar89_draw_radial_caps(m, ops, start, sweep,
                                    outer_percent, inner_percent,
                                    color, offset_x, offset_y);
        }
        return;
    }

    if (segments > total) {
        segments = total;
    }
    if (segments < 1) {
        segments = 1;
    }
    cell = total / segments;
    if (cell < 1) {
        cell = 1;
    }
    gap = m->style.radial_gap_deg;
    if (gap >= cell) {
        gap = cell - 1;
    }
    if (gap < 0) {
        gap = 0;
    }

    for (i = 0; i < segments; ++i) {
        seg0 = i * cell + gap / 2;
        seg1 = (i == segments - 1) ? total : (i + 1) * cell -
                                               (gap - gap / 2);
        lo = begin_abs > seg0 ? begin_abs : seg0;
        hi = end_abs < seg1 ? end_abs : seg1;
        if (hi > lo) {
            start = m->radial_start_deg + sign * lo;
            sweep = sign * (hi - lo);
            gbar89_draw_radial_arc_raw(m, ops, start, sweep,
                                       outer_percent, inner_percent,
                                       color, offset_x, offset_y);
            if (draw_caps) {
                gbar89_draw_radial_caps(m, ops, start, sweep,
                                        outer_percent, inner_percent,
                                        color, offset_x, offset_y);
            }
        }
    }
}

static void gbar89_draw_radial_outlined_range(const GBar89_Meter *m,
                                               const GBar89_RenderOps *ops,
                                               GBar89_Fix ratio0,
                                               GBar89_Fix ratio1,
                                               int outer_percent,
                                               int inner_percent,
                                               unsigned long color,
                                               int draw_caps)
{
    int p;
    int outline_inner;

    if ((m->style.fx_flags & GBAR89_FX_OUTER_OUTLINE) != 0 &&
        m->style.outline_size > 0) {
        p = gbar89_radial_px_percent(m, m->style.outline_size);
        outline_inner = inner_percent - p;
        if (outline_inner < 0) {
            outline_inner = 0;
        }
        gbar89_draw_radial_range_shape(m, ops, ratio0, ratio1,
                                       outer_percent + p, outline_inner,
                                       m->style.color_outline,
                                       0, 0, draw_caps);
    }
    gbar89_draw_radial_range_shape(m, ops, ratio0, ratio1,
                                   outer_percent, inner_percent,
                                   color, 0, 0, draw_caps);
}

static void gbar89_draw_radial_spoke(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops,
                                     int angle,
                                     int inner_percent,
                                     int outer_percent,
                                     unsigned long color)
{
    GBar89_Rect inner;
    int x0;
    int y0;
    int x1;
    int y1;

    if (ops->draw_line == 0) {
        return;
    }
    inner = gbar89_inner_rect(m);
    gbar89_angle_point(&inner, angle, inner_percent, &x0, &y0);
    gbar89_angle_point(&inner, angle, outer_percent, &x1, &y1);
    gbar89_call_line(ops, x0, y0, x1, y1, color);
}

static void gbar89_draw_radial_background_detail(const GBar89_Meter *m,
                                                  const GBar89_RenderOps *ops)
{
    int inner_percent;
    int outer_percent;
    int step;
    int thick;
    int total;
    int sign;
    int i;
    int j;
    int angle;
    int rings;
    int rp;
    int band;
    int half;
    int start;
    int sweep;

    if (m->style.bg_kind == GBAR89_BG_SOLID) {
        return;
    }
    inner_percent = gbar89_radial_base_inner(m);
    outer_percent = 100;
    step = m->style.bg_step;
    if (step < 3) {
        step = 3;
    }
    thick = gbar89_radial_px_percent(m, m->style.bg_size);
    total = gbar89_abs_int(m->radial_sweep_deg);
    sign = (m->radial_sweep_deg < 0) ? -1 : 1;
    rings = m->style.radial_detail_rings;
    if (rings < 1) {
        rings = 1;
    }
    start = m->radial_start_deg;
    sweep = m->radial_sweep_deg;

    if (m->style.bg_kind == GBAR89_BG_GRID) {
        for (i = 1; i <= rings; ++i) {
            rp = inner_percent + ((outer_percent - inner_percent) * i) /
                                 (rings + 1);
            gbar89_draw_radial_arc_raw(m, ops, start, sweep,
                                       rp + thick, rp,
                                       m->style.color_bg_detail, 0, 0);
        }
        for (i = 0; i <= total; i += step) {
            angle = start + sign * i + m->style.radial_phase_deg;
            gbar89_draw_radial_spoke(m, ops, angle,
                                     inner_percent, outer_percent,
                                     m->style.color_bg_detail);
        }
    } else if (m->style.bg_kind == GBAR89_BG_CHECKER) {
        half = inner_percent + (outer_percent - inner_percent) / 2;
        for (i = 0, j = 0; i < total; i += step, ++j) {
            angle = start + sign * i + m->style.radial_phase_deg;
            band = step;
            if (i + band > total) {
                band = total - i;
            }
            if ((j & 1) == 0) {
                gbar89_draw_radial_arc_raw(m, ops, angle, sign * band,
                                           half, inner_percent,
                                           m->style.color_bg_detail, 0, 0);
            } else {
                gbar89_draw_radial_arc_raw(m, ops, angle, sign * band,
                                           outer_percent, half,
                                           m->style.color_bg_detail, 0, 0);
            }
        }
    } else if (m->style.bg_kind == GBAR89_BG_SCANLINES) {
        for (i = 1; i <= rings * 2; i += 2) {
            rp = inner_percent + ((outer_percent - inner_percent) * i) /
                                 (rings * 2 + 1);
            gbar89_draw_radial_arc_raw(m, ops, start, sweep,
                                       rp + thick, rp,
                                       m->style.color_bg_detail, 0, 0);
        }
    } else if (m->style.bg_kind == GBAR89_BG_DIAGONAL) {
        if (ops->draw_line != 0) {
            GBar89_Rect inner;
            int x0;
            int y0;
            int x1;
            int y1;
            inner = gbar89_inner_rect(m);
            for (i = 0; i <= total; i += step) {
                angle = start + sign * i + m->style.radial_phase_deg;
                gbar89_angle_point(&inner, angle, inner_percent, &x0, &y0);
                gbar89_angle_point(&inner, angle + sign * (step / 2),
                                    outer_percent, &x1, &y1);
                gbar89_call_line(ops, x0, y0, x1, y1,
                                 m->style.color_bg_detail);
            }
        }
    } else if (m->style.bg_kind == GBAR89_BG_DITHER) {
        rp = inner_percent + (outer_percent - inner_percent) / 2;
        for (i = 0, j = 0; i < total; i += step, ++j) {
            if ((j & 1) == 0) {
                angle = start + sign * i + m->style.radial_phase_deg;
                band = step / 3;
                if (band < 1) {
                    band = 1;
                }
                gbar89_draw_radial_arc_raw(m, ops, angle, sign * band,
                                           rp + thick * 2, rp - thick,
                                           m->style.color_bg_detail, 0, 0);
            }
        }
    }
}

static void gbar89_draw_radial_fill_fx(const GBar89_Meter *m,
                                       const GBar89_RenderOps *ops,
                                       GBar89_Fix ratio)
{
    int inner_percent;
    int thickness;
    int gloss_inner;
    int rings;
    int i;
    int rp;
    int total;
    int filled;
    int sign;
    int step;
    int angle;
    int p;
    GBar89_Fix r0;
    GBar89_Fix r1;

    ratio = gbar89_fix_clamp01(ratio);
    if (ratio <= 0) {
        return;
    }
    inner_percent = gbar89_radial_base_inner(m);
    thickness = 100 - inner_percent;
    total = gbar89_abs_int(m->radial_sweep_deg);
    filled = (int)(((long)total * ratio) >> GBAR89_FIX_SHIFT);
    sign = (m->radial_sweep_deg < 0) ? -1 : 1;
    step = m->style.bg_step;
    if (step < 4) {
        step = 4;
    }

    if ((m->style.fx_flags & GBAR89_FX_GLOSS) != 0) {
        gloss_inner = 100 - (thickness * m->style.gloss_percent) / 100;
        if (gloss_inner < inner_percent) {
            gloss_inner = inner_percent;
        }
        gbar89_draw_radial_range_shape(m, ops, 0, ratio,
                                       100, gloss_inner,
                                       m->style.color_gloss, 0, 0, 0);
    }

    rings = m->style.radial_detail_rings;
    if (rings < 1) {
        rings = 1;
    }
    p = gbar89_radial_px_percent(m, 1);

    if ((m->style.fx_flags & GBAR89_FX_FILL_GRID) != 0 ||
        (m->style.fx_flags & GBAR89_FX_RADIAL_RINGS) != 0 ||
        ((m->flags & GBAR89_FLAG_DRAW_PATTERN) != 0 &&
         (m->style.pattern_kind == GBAR89_PATTERN_HORIZONTAL_STRIPES ||
          m->style.pattern_kind == GBAR89_PATTERN_CROSSHATCH))) {
        for (i = 1; i <= rings; ++i) {
            rp = inner_percent + (thickness * i) / (rings + 1);
            gbar89_draw_radial_range_shape(m, ops, 0, ratio,
                                           rp + p, rp,
                                           m->style.color_pattern,
                                           0, 0, 0);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_FILL_GRID) != 0 ||
        (m->style.fx_flags & GBAR89_FX_RADIAL_SPOKES) != 0 ||
        ((m->flags & GBAR89_FLAG_DRAW_PATTERN) != 0 &&
         (m->style.pattern_kind == GBAR89_PATTERN_VERTICAL_STRIPES ||
          m->style.pattern_kind == GBAR89_PATTERN_CROSSHATCH ||
          m->style.pattern_kind == GBAR89_PATTERN_TICKS))) {
        for (i = 0; i <= filled; i += step) {
            angle = m->radial_start_deg + sign * i +
                    m->style.radial_phase_deg;
            gbar89_draw_radial_spoke(m, ops, angle,
                                     inner_percent, 100,
                                     m->style.color_pattern);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_FILL_SCANLINES) != 0) {
        for (i = 1; i <= rings * 2; i += 2) {
            rp = inner_percent + (thickness * i) / (rings * 2 + 1);
            gbar89_draw_radial_range_shape(m, ops, 0, ratio,
                                           rp + p, rp,
                                           m->style.color_pattern,
                                           0, 0, 0);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_PIXEL_CELLS) != 0) {
        step = m->style.pixel_size;
        if (step < 2) {
            step = 2;
        }
        for (i = step; i < filled; i += step) {
            angle = m->radial_start_deg + sign * i;
            gbar89_draw_radial_spoke(m, ops, angle,
                                     inner_percent, 100,
                                     m->style.color_shadow);
        }
    }

    if ((m->flags & GBAR89_FLAG_DRAW_PATTERN) != 0 &&
        m->style.pattern_kind == GBAR89_PATTERN_DOTS) {
        step = m->style.pattern_step;
        if (step < 4) {
            step = 4;
        }
        rp = inner_percent + thickness / 2;
        for (i = 0; i <= filled; i += step) {
            GBar89_Rect inner;
            int x;
            int y;
            inner = gbar89_inner_rect(m);
            angle = m->radial_start_deg + sign * i +
                    m->style.radial_phase_deg;
            gbar89_angle_point(&inner, angle, rp, &x, &y);
            gbar89_draw_radial_disc(ops, x, y,
                                    m->style.pattern_size,
                                    m->style.color_pattern);
        }
    }

    if ((m->style.fx_flags & GBAR89_FX_INNER_SHADOW) != 0) {
        gbar89_draw_radial_range_shape(m, ops, 0, ratio,
                                       inner_percent + p * 2,
                                       inner_percent,
                                       m->style.color_shadow, 0, 0, 0);
    }

    if ((m->style.fx_flags & GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT) != 0) {
        r1 = ratio;
        r0 = ratio - (GBAR89_FIX_ONE / 40);
        if (r0 < 0) {
            r0 = 0;
        }
        gbar89_draw_radial_range_shape(m, ops, r0, r1,
                                       100, inner_percent,
                                       m->style.color_highlight,
                                       0, 0, 0);
    }
}

static void gbar89_draw_radial_markers(const GBar89_Meter *m,
                                       const GBar89_RenderOps *ops)
{
    int i;
    int angle;
    int inner_percent;
    int length;
    GBar89_Fix ratio;

    if ((m->flags & GBAR89_FLAG_DRAW_MARKERS) == 0) {
        return;
    }
    inner_percent = gbar89_radial_base_inner(m);
    length = m->style.radial_marker_length_percent;
    if (length < 1) {
        length = 1;
    }
    for (i = 0; i < m->marker_count; ++i) {
        ratio = gbar89_ratio_from_value(m, m->markers[i]);
        angle = m->radial_start_deg +
                (int)(((long)m->radial_sweep_deg * ratio) >>
                      GBAR89_FIX_SHIFT);
        gbar89_draw_radial_spoke(m, ops, angle,
                                 inner_percent - length / 3,
                                 100 + length,
                                 m->style.color_marker);
    }
}

static void gbar89_draw_radial_vector_units(const GBar89_Meter *m,
                                             const GBar89_RenderOps *ops)
{
    GBar89_Rect inner;
    GBar89_Rect slot;
    GBar89_Fix ratio;
    GBar89_Fix center;
    int count;
    int i;
    int angle;
    int x;
    int y;
    int size;
    int radius_percent;
    unsigned long color;

    if ((m->flags & GBAR89_FLAG_DRAW_VECTOR_UNITS) == 0 ||
        (m->unit_renderer == 0 &&
         (m->unit_points == 0 || m->unit_point_count < 2))) {
        return;
    }
    count = m->unit_count;
    if (count < 1) {
        return;
    }
    inner = gbar89_inner_rect(m);
    size = inner.w < inner.h ? inner.w : inner.h;
    size /= (count > 10) ? 9 : 7;
    if (size < 5) {
        size = 5;
    }
    radius_percent = m->style.radial_unit_radius_percent;
    ratio = gbar89_current_ratio(m);

    for (i = 0; i < count; ++i) {
        center = (GBar89_Fix)(((long)(i * 2 + 1) * GBAR89_FIX_ONE) /
                              (count * 2));
        angle = m->radial_start_deg +
                (int)(((long)m->radial_sweep_deg * center) >>
                      GBAR89_FIX_SHIFT);
        gbar89_angle_point(&inner, angle, radius_percent, &x, &y);
        slot = gbar89_rect_make(x - size / 2, y - size / 2, size, size);
        color = (ratio >= center) ? m->style.color_unit_fill
                                  : m->style.color_unit_empty;
        gbar89_draw_vector_glyph(m, ops, &slot, color);
    }
}

static unsigned long gbar89_radial_border_color(const GBar89_Meter *m)
{
    unsigned long color;

    color = m->style.color_border;
    if (m->state >= 0 && m->state < GBAR89_STATE_COUNT &&
        m->style.color_state_border[m->state] != 0UL) {
        color = m->style.color_state_border[m->state];
    }
    return color;
}

static void gbar89_draw_radial_frame(const GBar89_Meter *m,
                                     const GBar89_RenderOps *ops)
{
    int d;
    int g;
    int inner_percent;
    int i;
    int total;
    int sign;
    int pieces;
    int piece_sweep;
    int angle;
    unsigned long color;
    unsigned long outer_color;
    unsigned long inner_color;

    total = gbar89_abs_int(m->radial_sweep_deg);
    sign = (m->radial_sweep_deg < 0) ? -1 : 1;

    if ((m->style.fx_flags & GBAR89_FX_RADIAL_TICKS) != 0) {
        piece_sweep = m->style.bg_step;
        if (piece_sweep < 6) {
            piece_sweep = 6;
        }
        for (i = 0; i <= total; i += piece_sweep) {
            angle = m->radial_start_deg + sign * i +
                    m->style.radial_phase_deg;
            gbar89_draw_radial_spoke(m, ops, angle, 102, 111,
                                     m->style.color_border);
        }
    }

    if ((m->flags & GBAR89_FLAG_DRAW_BORDER) == 0 ||
        m->style.frame_kind == GBAR89_FRAME_NONE) {
        return;
    }
    d = gbar89_radial_px_percent(m, m->style.frame_depth);
    g = gbar89_radial_px_percent(m, m->style.frame_gap);
    inner_percent = gbar89_radial_base_inner(m);
    color = gbar89_radial_border_color(m);

    if (m->style.frame_kind == GBAR89_FRAME_SIMPLE) {
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + d, 100, color, 0, 0);
        if (inner_percent > 0) {
            gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                       m->radial_sweep_deg,
                                       inner_percent,
                                       inner_percent - d,
                                       color, 0, 0);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_DOUBLE) {
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + d, 100, color, 0, 0);
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 - g, 100 - g - d,
                                   m->style.color_highlight, 0, 0);
        if (inner_percent > 0) {
            gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                       m->radial_sweep_deg,
                                       inner_percent + g + d,
                                       inner_percent + g,
                                       m->style.color_shadow, 0, 0);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_BEVEL_OUT ||
               m->style.frame_kind == GBAR89_FRAME_BEVEL_IN) {
        outer_color = m->style.color_highlight;
        inner_color = m->style.color_shadow;
        if (m->style.frame_kind == GBAR89_FRAME_BEVEL_IN) {
            outer_color = m->style.color_shadow;
            inner_color = m->style.color_highlight;
        }
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + d, 100, outer_color, 0, 0);
        if (inner_percent > 0) {
            gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                       m->radial_sweep_deg,
                                       inner_percent,
                                       inner_percent - d,
                                       inner_color, 0, 0);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_BRACKETS) {
        pieces = 4;
        piece_sweep = m->style.frame_corner;
        if (piece_sweep < 8) {
            piece_sweep = 8;
        }
        if (piece_sweep > total / pieces) {
            piece_sweep = total / pieces;
        }
        for (i = 0; i < pieces; ++i) {
            angle = m->radial_start_deg + sign *
                    ((total * i) / pieces - piece_sweep / 2);
            gbar89_draw_radial_arc_raw(m, ops, angle,
                                       sign * piece_sweep,
                                       100 + d * 2, 100, color, 0, 0);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_RAIL) {
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + d, 100, color, 0, 0);
        if (inner_percent > 0) {
            gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                       m->radial_sweep_deg,
                                       inner_percent,
                                       inner_percent - d,
                                       color, 0, 0);
        }
        piece_sweep = m->style.frame_corner;
        if (piece_sweep < 8) {
            piece_sweep = 8;
        }
        for (i = 0; i <= total; i += piece_sweep) {
            angle = m->radial_start_deg + sign * i;
            gbar89_draw_radial_spoke(m, ops, angle,
                                     inner_percent - d,
                                     100 + d * 2,
                                     m->style.color_highlight);
        }
    } else if (m->style.frame_kind == GBAR89_FRAME_PIXEL) {
        pieces = m->style.frame_corner;
        if (pieces < 8) {
            pieces = 8;
        }
        piece_sweep = total / pieces;
        if (piece_sweep < 1) {
            piece_sweep = 1;
        }
        for (i = 0; i < pieces; ++i) {
            if ((i & 1) == 0) {
                angle = m->radial_start_deg + sign * i * piece_sweep;
                gbar89_draw_radial_arc_raw(m, ops, angle,
                                           sign * (piece_sweep - 1),
                                           100 + d * 2, 100,
                                           color, 0, 0);
            }
        }
    }
}

static void gbar89_draw_radial_pre_fx(const GBar89_Meter *m,
                                      const GBar89_RenderOps *ops)
{
    int inner_percent;
    int p;
    int i;
    int d;
    int ox;
    int oy;

    inner_percent = gbar89_radial_base_inner(m);
    p = gbar89_radial_px_percent(m, m->style.outline_size);
    ox = m->style.shadow_offset_x;
    oy = m->style.shadow_offset_y;

    if ((m->style.fx_flags & GBAR89_FX_DROP_SHADOW) != 0) {
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + p,
                                   inner_percent > p ? inner_percent - p : 0,
                                   m->style.color_shadow, ox, oy);
    }
    if ((m->style.fx_flags & GBAR89_FX_EXTRUDE) != 0) {
        d = m->style.extrude_depth;
        if (d < 1) {
            d = 1;
        }
        for (i = d; i >= 1; --i) {
            gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                       m->radial_sweep_deg,
                                       100 + p,
                                       inner_percent > p ? inner_percent - p : 0,
                                       m->style.color_extrude,
                                       (ox < 0) ? -i : i,
                                       (oy < 0) ? -i : i);
        }
    }
    if ((m->style.fx_flags & GBAR89_FX_OUTER_OUTLINE) != 0 &&
        m->style.outline_size > 0) {
        gbar89_draw_radial_arc_raw(m, ops, m->radial_start_deg,
                                   m->radial_sweep_deg,
                                   100 + p,
                                   inner_percent > p ? inner_percent - p : 0,
                                   m->style.color_outline, 0, 0);
    }
}

static void gbar89_draw_radial_layer_pips(const GBar89_Meter *m,
                                          const GBar89_RenderOps *ops)
{
    int filled;
    int i;
    int angle;
    int sweep;
    unsigned long color;

    if ((m->flags & GBAR89_FLAG_DRAW_LAYER_PIPS) == 0 ||
        (m->flags & GBAR89_FLAG_LAYERED) == 0) {
        return;
    }
    filled = gbar89_layer_count_filled(m, m->value);
    sweep = 7;
    for (i = 0; i < m->layer_count; ++i) {
        angle = m->radial_start_deg - 14 - i * 11;
        color = (i < filled) ? m->style.color_layer_fill[i]
                             : m->style.color_empty;
        if (color == 0UL) {
            color = m->style.color_fill;
        }
        gbar89_draw_radial_arc_raw(m, ops, angle, sweep,
                                   116, 106, color, 0, 0);
    }
}

static void gbar89_draw_radial(const GBar89_Meter *m,
                               const GBar89_RenderOps *ops)
{
    GBar89_Fix ratio;
    GBar89_Fix lag_ratio;
    GBar89_Fix overlay_ratio;
    GBar89_Fix mid_ratio;
    long fill_value;
    unsigned long fill_color;
    unsigned long lag_color;
    int inner_percent;

    if (m == 0 || ops == 0) {
        return;
    }
    if (ops->draw_triangles == 0) {
        gbar89_draw_linear_rects(m, ops);
        return;
    }

    inner_percent = gbar89_radial_base_inner(m);
    gbar89_draw_radial_pre_fx(m, ops);

    if ((m->flags & GBAR89_FLAG_DRAW_BG) != 0) {
        gbar89_draw_radial_range_shape(m, ops, 0, GBAR89_FIX_ONE,
                                       100, inner_percent,
                                       m->style.color_bg, 0, 0, 0);
        gbar89_draw_radial_background_detail(m, ops);
    }

    if ((m->flags & GBAR89_FLAG_DAMAGE_LAG) != 0) {
        lag_ratio = gbar89_display_ratio_from_value(m, m->lag_value);
        lag_ratio = gbar89_radial_quantize_ratio(m, lag_ratio);
        lag_color = gbar89_lag_color_for_value(m, m->lag_value);
        gbar89_draw_radial_range_shape(m, ops, 0, lag_ratio,
                                       100, inner_percent,
                                       lag_color, 0, 0, 1);
    }

    if ((m->flags & GBAR89_FLAG_USE_VISUAL) != 0) {
        fill_value = m->visual_value;
    } else {
        fill_value = m->value;
    }
    fill_color = gbar89_fill_color_for_value(m, fill_value);
    ratio = gbar89_radial_quantize_ratio(m, gbar89_current_ratio(m));
    gbar89_draw_radial_outlined_range(m, ops, 0, ratio,
                                      100, inner_percent,
                                      fill_color, 1);

    if ((m->flags & GBAR89_FLAG_DRAW_MID_VALUE) != 0) {
        mid_ratio = gbar89_radial_quantize_ratio(m, gbar89_mid_ratio(m));
        if (mid_ratio > ratio) {
            gbar89_draw_radial_outlined_range(m, ops, ratio, mid_ratio,
                                              100, inner_percent,
                                              m->style.color_mid, 0);
        } else if (mid_ratio < ratio) {
            gbar89_draw_radial_outlined_range(m, ops, mid_ratio, ratio,
                                              100, inner_percent,
                                              m->style.color_mid, 0);
        }
    }

    if ((m->flags & GBAR89_FLAG_DRAW_VALUE_OVERLAY) != 0) {
        overlay_ratio = gbar89_radial_quantize_ratio(
            m, gbar89_overlay_ratio(m));
        gbar89_draw_radial_range_shape(m, ops, 0, overlay_ratio,
                                       100, inner_percent,
                                       m->style.color_overlay, 0, 0, 1);
    }

    gbar89_draw_radial_fill_fx(m, ops, ratio);
    gbar89_draw_radial_vector_units(m, ops);
    gbar89_draw_radial_markers(m, ops);
    gbar89_draw_radial_layer_pips(m, ops);
    gbar89_draw_radial_frame(m, ops);
}

void gbar89_draw(const GBar89_Meter *meter, const GBar89_RenderOps *ops)
{
    if (meter == 0 || ops == 0) {
        return;
    }

    if (meter->kind != GBAR89_KIND_RADIAL_PIE &&
        meter->kind != GBAR89_KIND_RADIAL_RING) {
        gbar89_draw_pre_fx(meter, ops);
    }

    if (meter->kind == GBAR89_KIND_SEGMENTED) {
        gbar89_draw_segmented(meter, ops);
    } else if (meter->kind == GBAR89_KIND_SPRITE_CLIP) {
        gbar89_draw_sprite_clip(meter, ops);
    } else if (meter->kind == GBAR89_KIND_NINESLICE) {
        gbar89_draw_nineslice(meter, ops);
    } else if (meter->kind == GBAR89_KIND_RADIAL_PIE ||
               meter->kind == GBAR89_KIND_RADIAL_RING) {
        gbar89_draw_radial(meter, ops);
    } else {
        gbar89_draw_linear_rects(meter, ops);
    }
}
