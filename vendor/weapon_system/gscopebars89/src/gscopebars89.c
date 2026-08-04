/*
 * gscopebars89.c
 * C89 bar/channel bridge. No dynamic allocation.
 */
#include "../include/gscopebars89.h"

#define GSB89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

gsb89_bar gsb89_bar_make(short bar_id,
                          short channel_id,
                          short kind,
                          short direction,
                          gsp89_fx x0,
                          gsp89_fx y0,
                          gsp89_fx x1,
                          gsp89_fx y1)
{
    gsb89_bar b;
    b.bar_id = bar_id;
    b.channel_id = channel_id;
    b.kind = kind;
    b.direction = direction;
    b.flags = GSB89_VISIBLE | GSB89_SHOW_BACKGROUND | GSB89_SHOW_OUTLINE;
    b.layer = 20;
    b.part_id = channel_id;
    b.segment_count = 10;
    b.glyph_id = -1;
    b.x0 = x0;
    b.y0 = y0;
    b.x1 = x1;
    b.y1 = y1;
    b.thickness = GSP89_NORM(300);
    b.radius_x = GSP89_NORM(5000);
    b.radius_y = GSP89_NORM(5000);
    b.start_deg_x100 = -9000;
    b.end_deg_x100 = 9000;
    b.foreground = gsp89_rgba(64, 255, 96, 255);
    b.background = gsp89_rgba(0, 0, 0, 96);
    b.outline = gsp89_rgba(0, 0, 0, 255);
    b.thickness_px = 1;
    b.outline_px = 1;
    b.blend_mode = GSP89_BLEND_ALPHA;
    return b;
}

gsp89_fx gsb89_channel_ratio(const gsb89_channel *channel)
{
    long value;
    long span;
    long q;
    if (!channel) return 0;
    span = channel->maximum - channel->minimum;
    if (span <= 0) return 0;
    value = channel->value - channel->minimum;
    if (value < 0) value = 0;
    if (value > span) value = span;
    if (span < 32768L) {
        q = (value * 65536L) / span;
    } else {
        q = ((value / 256L) * 65536L) / (span / 256L);
    }
    return (gsp89_fx)GSB89_CLAMP(q, 0L, 65536L);
}

int gsb89_find_channel(const gsb89_channel *channels,
                       short channel_count,
                       short channel_id,
                       gsb89_channel *out_channel)
{
    short i;
    if (!channels || channel_count < 1 || !out_channel) return 0;
    for (i = 0; i < channel_count; ++i) {
        if (channels[i].channel_id == channel_id) {
            *out_channel = channels[i];
            return 1;
        }
    }
    return 0;
}

static gsp89_color gsb89_alpha(gsp89_color c, short global_alpha)
{
    long a;
    global_alpha = (short)GSB89_CLAMP(global_alpha, 0, 255);
    a = ((long)c.a * (long)global_alpha) / 255L;
    c.a = (unsigned char)GSB89_CLAMP(a, 0L, 255L);
    return c;
}

static gsp89_style gsb89_style(const gsb89_bar *bar,
                               gsp89_color color,
                               short filled,
                               short global_alpha)
{
    short flags;
    flags = filled ? GSP89_FLAG_FILLED : 0;
    if (bar->flags & GSB89_SHOW_OUTLINE) flags = (short)(flags | GSP89_FLAG_OUTLINE);
    return gsp89_style_make(gsb89_alpha(color, global_alpha),
                            gsb89_alpha(bar->outline, global_alpha),
                            bar->thickness_px,
                            (bar->flags & GSB89_SHOW_OUTLINE) ? bar->outline_px : 0,
                            bar->layer,
                            bar->part_id,
                            bar->blend_mode,
                            flags);
}

static void gsb89_emit_background(gsp89_painter *painter,
                                  const gsb89_bar *bar,
                                  short global_alpha)
{
    gsp89_style style;
    if (!(bar->flags & GSB89_SHOW_BACKGROUND)) return;
    style = gsb89_style(bar, bar->background, 1, global_alpha);
    if (bar->kind == GSB89_BAR_ARC) {
        gsp89_paint_arc(painter, bar->x0, bar->y0,
                        bar->radius_x, bar->radius_y,
                        bar->start_deg_x100, bar->end_deg_x100, &style);
    } else {
        gsp89_paint_rect(painter, bar->x0, bar->y0, bar->x1, bar->y1, &style);
    }
}

static void gsb89_emit_linear(gsp89_painter *painter,
                              const gsb89_bar *bar,
                              gsp89_fx ratio,
                              short global_alpha)
{
    gsp89_fx x0;
    gsp89_fx y0;
    gsp89_fx x1;
    gsp89_fx y1;
    gsp89_style style;
    x0 = bar->x0;
    y0 = bar->y0;
    x1 = bar->x1;
    y1 = bar->y1;
    if (bar->direction == GSB89_DIR_RIGHT_TO_LEFT) {
        x0 = bar->x1 - gsp89_fx_mul(bar->x1 - bar->x0, ratio);
    } else if (bar->direction == GSB89_DIR_TOP_TO_BOTTOM) {
        y1 = bar->y0 + gsp89_fx_mul(bar->y1 - bar->y0, ratio);
    } else if (bar->direction == GSB89_DIR_BOTTOM_TO_TOP) {
        y0 = bar->y1 - gsp89_fx_mul(bar->y1 - bar->y0, ratio);
    } else {
        x1 = bar->x0 + gsp89_fx_mul(bar->x1 - bar->x0, ratio);
    }
    style = gsb89_style(bar, bar->foreground, 1, global_alpha);
    gsp89_paint_rect(painter, x0, y0, x1, y1, &style);
}

static void gsb89_emit_segmented(gsp89_painter *painter,
                                 const gsb89_bar *bar,
                                 gsp89_fx ratio,
                                 short global_alpha)
{
    short i;
    short segments;
    short filled_count;
    gsp89_fx step_x;
    gsp89_fx step_y;
    gsp89_fx gap_x;
    gsp89_fx gap_y;
    gsp89_fx x0;
    gsp89_fx y0;
    gsp89_fx x1;
    gsp89_fx y1;
    gsp89_style style;

    segments = bar->segment_count < 1 ? 1 : bar->segment_count;
    filled_count = (short)(((long)segments * ratio) / GSP89_FX_ONE);
    if (filled_count > segments) filled_count = segments;
    step_x = (bar->x1 - bar->x0) / segments;
    step_y = (bar->y1 - bar->y0) / segments;
    gap_x = step_x / 8;
    gap_y = step_y / 8;
    style = gsb89_style(bar, bar->foreground, 1, global_alpha);

    for (i = 0; i < filled_count; ++i) {
        x0 = bar->x0 + step_x * i;
        y0 = bar->y0 + step_y * i;
        x1 = x0 + step_x;
        y1 = y0 + step_y;
        if (step_x != 0) {
            x0 += gap_x;
            x1 -= gap_x;
        }
        if (step_y != 0) {
            y0 += gap_y;
            y1 -= gap_y;
        }
        gsp89_paint_rect(painter, x0, y0, x1, y1, &style);
    }
}

static void gsb89_emit_arc(gsp89_painter *painter,
                           const gsb89_bar *bar,
                           gsp89_fx ratio,
                           short global_alpha)
{
    long span;
    short end;
    gsp89_style style;
    span = (long)bar->end_deg_x100 - (long)bar->start_deg_x100;
    if (bar->direction == GSB89_DIR_COUNTERCLOCKWISE) span = -span;
    end = (short)((long)bar->start_deg_x100 + (span * ratio) / GSP89_FX_ONE);
    style = gsb89_style(bar, bar->foreground, 0, global_alpha);
    gsp89_paint_arc(painter, bar->x0, bar->y0,
                    bar->radius_x, bar->radius_y,
                    bar->start_deg_x100, end, &style);
}

static void gsb89_emit_ticks(gsp89_painter *painter,
                             const gsb89_bar *bar,
                             gsp89_fx ratio,
                             short global_alpha)
{
    short i;
    short segments;
    short filled_count;
    gsp89_fx t;
    gsp89_fx x;
    gsp89_fx y;
    gsp89_style style;
    segments = bar->segment_count < 2 ? 2 : bar->segment_count;
    filled_count = (short)(((long)segments * ratio) / GSP89_FX_ONE);
    style = gsb89_style(bar, bar->foreground, 0, global_alpha);
    for (i = 0; i < filled_count; ++i) {
        t = (gsp89_fx)(((long)i * GSP89_FX_ONE) / (long)(segments - 1));
        x = bar->x0 + gsp89_fx_mul(bar->x1 - bar->x0, t);
        y = bar->y0 + gsp89_fx_mul(bar->y1 - bar->y0, t);
        if (bar->direction == GSB89_DIR_TOP_TO_BOTTOM ||
            bar->direction == GSB89_DIR_BOTTOM_TO_TOP) {
            gsp89_paint_line(painter, x - bar->thickness, y,
                             x + bar->thickness, y, &style);
        } else {
            gsp89_paint_line(painter, x, y - bar->thickness,
                             x, y + bar->thickness, &style);
        }
    }
}

static void gsb89_emit_marker(gsp89_painter *painter,
                              const gsb89_bar *bar,
                              gsp89_fx ratio,
                              short global_alpha)
{
    gsp89_fx x;
    gsp89_fx y;
    gsp89_style style;
    x = bar->x0 + gsp89_fx_mul(bar->x1 - bar->x0, ratio);
    y = bar->y0 + gsp89_fx_mul(bar->y1 - bar->y0, ratio);
    style = gsb89_style(bar, bar->foreground, 1, global_alpha);
    gsp89_paint_circle(painter, x, y, bar->thickness, &style);
}

void gsb89_emit_bar(gsp89_painter *painter,
                    const gsb89_bar *bar,
                    const gsb89_channel *channel,
                    short global_alpha)
{
    gsp89_fx ratio;
    if (!painter || !bar || !channel) return;
    if (!(bar->flags & GSB89_VISIBLE) || !channel->visible) return;
    ratio = gsb89_channel_ratio(channel);
    if (bar->flags & GSB89_INVERT_VALUE) ratio = GSP89_FX_ONE - ratio;
    if ((bar->flags & GSB89_HIDE_WHEN_FULL) && ratio >= GSP89_FX_ONE) return;
    if ((bar->flags & GSB89_HIDE_WHEN_EMPTY) && ratio <= 0) return;

    gsb89_emit_background(painter, bar, global_alpha);
    switch (bar->kind) {
        case GSB89_BAR_SEGMENTED:
            gsb89_emit_segmented(painter, bar, ratio, global_alpha);
            break;
        case GSB89_BAR_ARC:
            gsb89_emit_arc(painter, bar, ratio, global_alpha);
            break;
        case GSB89_BAR_TICKS:
            gsb89_emit_ticks(painter, bar, ratio, global_alpha);
            break;
        case GSB89_BAR_MARKER:
            gsb89_emit_marker(painter, bar, ratio, global_alpha);
            break;
        default:
            gsb89_emit_linear(painter, bar, ratio, global_alpha);
            break;
    }

    if ((bar->flags & GSB89_SHOW_GLYPH) && bar->glyph_id >= 0) {
        gsp89_style style;
        style = gsb89_style(bar, bar->foreground, 0, global_alpha);
        gsp89_paint_glyph(painter, bar->glyph_id,
                          bar->x0, bar->y0,
                          GSP89_NORM(500), GSP89_NORM(500), &style);
    }
}

void gsb89_emit_layout_from_array(gsp89_painter *painter,
                                  const gsb89_layout *layout,
                                  const gsb89_channel *channels,
                                  short channel_count,
                                  short global_alpha)
{
    short i;
    gsb89_channel channel;
    if (!painter || !layout || !layout->bars) return;
    for (i = 0; i < layout->bar_count; ++i) {
        if (gsb89_find_channel(channels, channel_count,
                               layout->bars[i].channel_id, &channel)) {
            gsb89_emit_bar(painter, &layout->bars[i], &channel, global_alpha);
        }
    }
}

void gsb89_emit_layout_from_callback(gsp89_painter *painter,
                                     const gsb89_layout *layout,
                                     gsb89_channel_cb channel_cb,
                                     void *user,
                                     short global_alpha)
{
    short i;
    gsb89_channel channel;
    if (!painter || !layout || !layout->bars || !channel_cb) return;
    for (i = 0; i < layout->bar_count; ++i) {
        if (channel_cb(user, layout->bars[i].channel_id, &channel)) {
            gsb89_emit_bar(painter, &layout->bars[i], &channel, global_alpha);
        }
    }
}
