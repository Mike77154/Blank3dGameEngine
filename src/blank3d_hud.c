#include "blank3d_hud.h"
#include "blank3d_numbar.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <string.h>

#include "ecg_font5x7.h"

static void b3d_gl_color(unsigned long rgba)
{
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    r = (GLubyte)((rgba >> 24) & 255UL);
    g = (GLubyte)((rgba >> 16) & 255UL);
    b = (GLubyte)((rgba >> 8) & 255UL);
    a = (GLubyte)(rgba & 255UL);
    glColor4ub(r, g, b, a);
}

static void b3d_draw_rect(void *user, int x, int y, int w, int h,
                          unsigned long rgba)
{
    Blank3DHud *hud;
    int yy;
    hud = (Blank3DHud *)user;
    if (!hud || w <= 0 || h <= 0) return;
    yy = hud->height - y - h;
    b3d_gl_color(rgba);
    glBegin(GL_QUADS);
    glVertex2i(x, yy);
    glVertex2i(x + w, yy);
    glVertex2i(x + w, yy + h);
    glVertex2i(x, yy + h);
    glEnd();
}

static void b3d_draw_line(void *user, int x1, int y1, int x2, int y2,
                          unsigned long rgba)
{
    Blank3DHud *hud;
    hud = (Blank3DHud *)user;
    if (!hud) return;
    b3d_gl_color(rgba);
    glBegin(GL_LINES);
    glVertex2i(x1, hud->height - y1);
    glVertex2i(x2, hud->height - y2);
    glEnd();
}

static void b3d_draw_sprite(void *user, int sprite_id,
                            int sx, int sy, int sw, int sh,
                            int dx, int dy, int dw, int dh,
                            unsigned long tint_rgba)
{
    Blank3DHud *hud;
    hud = (Blank3DHud *)user;
    if (!hud || dw <= 0 || dh <= 0) return;
    if (hud->sprite_provider.draw) {
        hud->sprite_provider.draw(hud->sprite_provider.user, sprite_id,
                                  sx, sy, sw, sh,
                                  dx, dy, dw, dh, tint_rgba);
        return;
    }
    /* Provider-less fallback keeps sprite/nine-slice meters inspectable.
     * The source rectangle is intentionally ignored until a real atlas
     * provider is installed. */
    b3d_draw_rect(user, dx, dy, dw, dh, tint_rgba);
}

static void b3d_draw_triangles(void *user,
                               const GBar89_Vertex *vertices,
                               int vertex_count,
                               const int *indices,
                               int index_count,
                               int sprite_id)
{
    Blank3DHud *hud;
    int i;
    int index;
    const GBar89_Vertex *vertex;
    hud = (Blank3DHud *)user;
    if (!hud || !vertices || !indices || vertex_count <= 0 || index_count <= 0)
        return;
    glBegin(GL_TRIANGLES);
    for (i = 0; i < index_count; ++i) {
        index = indices[i];
        if (index < 0 || index >= vertex_count) continue;
        vertex = &vertices[index];
        b3d_gl_color(vertex->color);
        glVertex2i(vertex->x, hud->height - vertex->y);
    }
    glEnd();
    (void)sprite_id;
}

static void b3d_push_clip(void *user, int x, int y, int w, int h)
{
    Blank3DHud *hud;
    int depth;
    int px;
    int py;
    int pr;
    int pb;
    int r;
    int b;
    hud = (Blank3DHud *)user;
    if (!hud || w <= 0 || h <= 0) return;
    depth = hud->clip_depth;
    if (depth >= B3D_HUD_CLIP_STACK_MAX) depth = B3D_HUD_CLIP_STACK_MAX - 1;
    if (hud->clip_depth > 0) {
        px = hud->clip_x[hud->clip_depth - 1];
        py = hud->clip_y[hud->clip_depth - 1];
        pr = px + hud->clip_w[hud->clip_depth - 1];
        pb = py + hud->clip_h[hud->clip_depth - 1];
        r = x + w;
        b = y + h;
        if (x < px) x = px;
        if (y < py) y = py;
        if (r > pr) r = pr;
        if (b > pb) b = pb;
        w = r - x;
        h = b - y;
        if (w < 0) w = 0;
        if (h < 0) h = 0;
    }
    hud->clip_x[depth] = x;
    hud->clip_y[depth] = y;
    hud->clip_w[depth] = w;
    hud->clip_h[depth] = h;
    if (hud->clip_depth < B3D_HUD_CLIP_STACK_MAX) hud->clip_depth++;
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, hud->height - y - h, w, h);
}

static void b3d_pop_clip(void *user)
{
    Blank3DHud *hud;
    int at;
    hud = (Blank3DHud *)user;
    if (!hud) return;
    if (hud->clip_depth > 0) hud->clip_depth--;
    if (hud->clip_depth <= 0) {
        hud->clip_depth = 0;
        glDisable(GL_SCISSOR_TEST);
        return;
    }
    at = hud->clip_depth - 1;
    glEnable(GL_SCISSOR_TEST);
    glScissor(hud->clip_x[at],
              hud->height - hud->clip_y[at] - hud->clip_h[at],
              hud->clip_w[at], hud->clip_h[at]);
}

static void b3d_begin_2d(int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, 0.0, (GLdouble)height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void b3d_end_2d(void)
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static int b3d_ecg_color_equal(ECG_Color a, ECG_Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static void b3d_draw_ecg_surface(Blank3DHud *hud,
                                 const Blank3DBigHudNode *node,
                                 int x, int y, int width, int height)
{
    const ECG_Surface *surface;
    ECG_Color clear_color;
    ECG_Color background_color;
    ECG_Color pixel;
    unsigned int pixel_count;
    unsigned int i;
    unsigned int alpha;
    unsigned int out_at;
    GLfloat zoom_x;
    GLfloat zoom_y;
    if (!hud || !node) return;
    surface = blank3d_ecg_vitals_surface(&hud->ecg);
    if (!surface || !surface->pixels || width <= 0 || height <= 0) return;

    clear_color = hud->ecg.dynamics.clear_color;
    background_color = hud->ecg.config.background_box.color;
    pixel_count = surface->width * surface->height;
    for (i = 0u; i < pixel_count; ++i) {
        pixel = surface->pixels[i];
        alpha = node->ecg_content_alpha;
        if (b3d_ecg_color_equal(pixel, clear_color)) {
            alpha = node->ecg_clear_alpha;
        } else if (hud->ecg.config.background_box.filled &&
                   b3d_ecg_color_equal(pixel, background_color)) {
            alpha = node->ecg_background_alpha;
        }
        out_at = i * 4u;
        hud->ecg_rgba[out_at] = pixel.r;
        hud->ecg_rgba[out_at + 1u] = pixel.g;
        hud->ecg_rgba[out_at + 2u] = pixel.b;
        hud->ecg_rgba[out_at + 3u] = (unsigned char)alpha;
    }

    zoom_x = (GLfloat)width / (GLfloat)surface->width;
    zoom_y = (GLfloat)height / (GLfloat)surface->height;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glRasterPos2i(x, hud->height - y);
    glPixelZoom(zoom_x, -zoom_y);
    glDrawPixels((GLsizei)surface->width,
                 (GLsizei)surface->height,
                 GL_RGBA, GL_UNSIGNED_BYTE, hud->ecg_rgba);
    glPixelZoom(1.0f, 1.0f);
}

static void b3d_draw_text5x7(Blank3DHud *hud,
                             int x, int y,
                             const char *text,
                             int scale,
                             int gap,
                             unsigned long rgba)
{
    unsigned int i;
    unsigned int row;
    unsigned int col;
    ECG_U8 bits;
    int cursor_x;
    if (!hud || !text || scale < 1) return;
    if (gap < 0) gap = 0;
    cursor_x = x;
    b3d_gl_color(rgba);
    glBegin(GL_QUADS);
    for (i = 0u; text[i] != '\0'; ++i) {
        for (row = 0u; row < 7u; ++row) {
            bits = ecg_font5x7_row(text[i], row);
            for (col = 0u; col < 5u; ++col) {
                int px;
                int py;
                if (((bits >> (4u - col)) & 1u) == 0u) continue;
                px = cursor_x + (int)col * scale;
                py = y + (int)row * scale;
                glVertex2i(px, hud->height - py);
                glVertex2i(px + scale, hud->height - py);
                glVertex2i(px + scale, hud->height - py - scale);
                glVertex2i(px, hud->height - py - scale);
            }
        }
        cursor_x += 5 * scale + gap;
    }
    glEnd();
}

static void b3d_text_append(char *dst, unsigned int cap, const char *src)
{
    unsigned int at;
    unsigned int i;
    if (!dst || cap == 0u || !src) return;
    at = (unsigned int)strlen(dst);
    i = 0u;
    while (src[i] && at + 1u < cap) dst[at++] = src[i++];
    dst[at] = '\0';
}

static void b3d_text_append_number(char *dst, unsigned int cap,
                                   long value, int pad)
{
    char number[40];
    if (pad < 0) pad = 0;
    if (pad > 18) pad = 18;
    if (pad > 0) sprintf(number, "%0*ld", pad, value);
    else sprintf(number, "%ld", value);
    b3d_text_append(dst, cap, number);
}

static void b3d_counter_text(const Blank3DBigHudNode *node,
                             const Blank3DBigHudTelemetry *telemetry,
                             char *out,
                             unsigned int cap)
{
    long value;
    long value2;
    if (!out || cap == 0u) return;
    out[0] = '\0';
    if (!node) return;
    value = blank3d_bighud_resolve_binding(telemetry, node->bind, 0L);
    value2 = blank3d_bighud_resolve_binding(telemetry, node->bind2, 0L);
    b3d_text_append(out, cap, node->counter_label);
    b3d_text_append(out, cap, node->counter_prefix);
    b3d_text_append_number(out, cap, value, node->counter_pad);
    if (node->counter_mode == B3D_BIGHUD_COUNTER_PAIR || node->bind2[0]) {
        b3d_text_append(out, cap, node->counter_separator);
        b3d_text_append_number(out, cap, value2, node->counter_pad2);
    }
    b3d_text_append(out, cap, node->counter_suffix);
}

static void b3d_draw_counter(Blank3DHud *hud,
                             const Blank3DBigHudNode *node,
                             const Blank3DBigHudTelemetry *telemetry)
{
    char text[160];
    int x;
    int y;
    int w;
    int h;
    int scale;
    int gap;
    int text_w;
    int text_h;
    int count;
    if (!hud || !node || !telemetry) return;
    b3d_counter_text(node, telemetry, text, sizeof(text));
    scale = node->counter_scale;
    if (scale < 1) scale = 1;
    gap = node->counter_gap;
    if (gap < 0) gap = 0;
    count = (int)strlen(text);
    text_w = count > 0 ? count * (5 * scale + gap) - gap : 1;
    text_h = 7 * scale;
    blank3d_bighud_resolve_rect(node, hud->width, hud->height,
                                text_w, text_h, &x, &y, &w, &h);
    if (node->counter_background)
        b3d_draw_rect(hud, x - 3, y - 3, text_w + 6, text_h + 6,
                      node->counter_background_color);
    if (node->counter_border) {
        b3d_draw_line(hud, x - 3, y - 3, x + text_w + 3, y - 3,
                      node->counter_border_color);
        b3d_draw_line(hud, x + text_w + 3, y - 3,
                      x + text_w + 3, y + text_h + 3,
                      node->counter_border_color);
        b3d_draw_line(hud, x + text_w + 3, y + text_h + 3,
                      x - 3, y + text_h + 3,
                      node->counter_border_color);
        b3d_draw_line(hud, x - 3, y + text_h + 3, x - 3, y - 3,
                      node->counter_border_color);
    }
    if (node->counter_shadow)
        b3d_draw_text5x7(hud, x + scale, y + scale, text,
                         scale, gap, node->counter_shadow_color);
    b3d_draw_text5x7(hud, x, y, text, scale, gap, node->counter_color);
    (void)w;
    (void)h;
}

static void b3d_draw_bighud(Blank3DHud *hud,
                            const Blank3DBigHudTelemetry *telemetry,
                            unsigned int frame_ms)
{
    int i;
    int x;
    int y;
    int w;
    int h;
    int source_w;
    int source_h;
    Blank3DBigHudNode *node;
    if (!hud || !telemetry || !hud->layout.enabled) return;
    for (i = 0; i < hud->layout.node_count; ++i) {
        node = &hud->layout.nodes[i];
        if (!node->enabled) continue;
        if (node->type == B3D_BIGHUD_NODE_BAR) {
            blank3d_bighud_update_bar(node, telemetry, frame_ms);
            blank3d_bighud_resolve_rect(node, hud->width, hud->height,
                                        node->meter.rect.w,
                                        node->meter.rect.h,
                                        &x, &y, &w, &h);
            node->meter.rect.x = x;
            node->meter.rect.y = y;
            node->meter.rect.w = w;
            node->meter.rect.h = h;
            gbar89_draw(&node->meter, &hud->render_ops);
        } else if (node->type == B3D_BIGHUD_NODE_ECG) {
            source_w = (B3D_ECG_WIDTH * node->ecg_scale_x_q8) / 256;
            source_h = (B3D_ECG_HEIGHT * node->ecg_scale_y_q8) / 256;
            if (source_w < 1) source_w = 1;
            if (source_h < 1) source_h = 1;
            blank3d_bighud_resolve_rect(node, hud->width, hud->height,
                                        source_w, source_h,
                                        &x, &y, &w, &h);
            b3d_draw_ecg_surface(hud, node, x, y, w, h);
        } else if (node->type == B3D_BIGHUD_NODE_COUNTER) {
            b3d_draw_counter(hud, node, telemetry);
        }
    }
}



static int b3d_scale_px(int value, int scale_q8)
{
    int result;
    if (value == 0) return 0;
    if (scale_q8 < 1) scale_q8 = 1;
    if (value > 0) {
        result = (value * scale_q8 + 128) / 256;
        return result < 1 ? 1 : result;
    }
    result = ((-value) * scale_q8 + 128) / 256;
    return result < 1 ? -1 : -result;
}

static void b3d_scale_meter_pixels(GBar89_Meter *meter,
                                   int scale_x_q8,
                                   int scale_y_q8)
{
    int average_q8;
    if (!meter) return;
    average_q8 = (scale_x_q8 + scale_y_q8) / 2;
    meter->style.margin_left = b3d_scale_px(meter->style.margin_left, scale_x_q8);
    meter->style.margin_right = b3d_scale_px(meter->style.margin_right, scale_x_q8);
    meter->style.margin_top = b3d_scale_px(meter->style.margin_top, scale_y_q8);
    meter->style.margin_bottom = b3d_scale_px(meter->style.margin_bottom, scale_y_q8);
    meter->style.padding_left = b3d_scale_px(meter->style.padding_left, scale_x_q8);
    meter->style.padding_right = b3d_scale_px(meter->style.padding_right, scale_x_q8);
    meter->style.padding_top = b3d_scale_px(meter->style.padding_top, scale_y_q8);
    meter->style.padding_bottom = b3d_scale_px(meter->style.padding_bottom, scale_y_q8);
    meter->style.border_size = b3d_scale_px(meter->style.border_size, average_q8);
    meter->style.segment_gap = b3d_scale_px(meter->style.segment_gap, average_q8);
    meter->style.marker_size = b3d_scale_px(meter->style.marker_size, average_q8);
    meter->style.pattern_step = b3d_scale_px(meter->style.pattern_step, average_q8);
    meter->style.pattern_size = b3d_scale_px(meter->style.pattern_size, average_q8);
    meter->style.frame_depth = b3d_scale_px(meter->style.frame_depth, average_q8);
    meter->style.frame_gap = b3d_scale_px(meter->style.frame_gap, average_q8);
    meter->style.frame_corner = b3d_scale_px(meter->style.frame_corner, average_q8);
    meter->style.outline_size = b3d_scale_px(meter->style.outline_size, average_q8);
    meter->style.shadow_offset_x = b3d_scale_px(meter->style.shadow_offset_x, scale_x_q8);
    meter->style.shadow_offset_y = b3d_scale_px(meter->style.shadow_offset_y, scale_y_q8);
    meter->style.extrude_depth = b3d_scale_px(meter->style.extrude_depth, average_q8);
    meter->style.bg_step = b3d_scale_px(meter->style.bg_step, average_q8);
    meter->style.bg_size = b3d_scale_px(meter->style.bg_size, average_q8);
    meter->style.pixel_size = b3d_scale_px(meter->style.pixel_size, average_q8);
    meter->unit_padding = b3d_scale_px(meter->unit_padding, average_q8);
    meter->mask_amount = b3d_scale_px(meter->mask_amount, average_q8);
}

void blank3d_hud_draw_layout_scaled(Blank3DHud *hud,
                                      Blank3DBigHud *layout,
                                      const Blank3DBigHudTelemetry *telemetry,
                                      unsigned int frame_ms,
                                      int anchor,
                                      int offset_x,
                                      int offset_y,
                                      int scale_x_q8,
                                      int scale_y_q8,
                                      int canvas_width,
                                      int canvas_height)
{
    int i;
    int base_x;
    int base_y;
    int scaled_canvas_w;
    int scaled_canvas_h;
    int local_x;
    int local_y;
    int local_w;
    int local_h;
    int default_w;
    int default_h;
    int average_q8;
    Blank3DBigHudNode *node;
    Blank3DBigHudNode temp;
    if (!hud || !layout || !telemetry || !layout->enabled) return;
    if (scale_x_q8 < 1) scale_x_q8 = 1;
    if (scale_y_q8 < 1) scale_y_q8 = 1;
    if (canvas_width < 1) canvas_width = 1;
    if (canvas_height < 1) canvas_height = 1;
    average_q8 = (scale_x_q8 + scale_y_q8) / 2;
    scaled_canvas_w = b3d_scale_px(canvas_width, scale_x_q8);
    scaled_canvas_h = b3d_scale_px(canvas_height, scale_y_q8);
    base_x = offset_x;
    base_y = offset_y;
    switch (anchor) {
    case B3D_BIGHUD_ANCHOR_TOP_CENTER:
        base_x = (hud->width - scaled_canvas_w) / 2 + offset_x;
        break;
    case B3D_BIGHUD_ANCHOR_TOP_RIGHT:
        base_x = hud->width - scaled_canvas_w - offset_x;
        break;
    case B3D_BIGHUD_ANCHOR_CENTER:
        base_x = (hud->width - scaled_canvas_w) / 2 + offset_x;
        base_y = (hud->height - scaled_canvas_h) / 2 + offset_y;
        break;
    case B3D_BIGHUD_ANCHOR_BOTTOM_LEFT:
        base_y = hud->height - scaled_canvas_h - offset_y;
        break;
    case B3D_BIGHUD_ANCHOR_BOTTOM_CENTER:
        base_x = (hud->width - scaled_canvas_w) / 2 + offset_x;
        base_y = hud->height - scaled_canvas_h - offset_y;
        break;
    case B3D_BIGHUD_ANCHOR_BOTTOM_RIGHT:
        base_x = hud->width - scaled_canvas_w - offset_x;
        base_y = hud->height - scaled_canvas_h - offset_y;
        break;
    case B3D_BIGHUD_ANCHOR_TOP_LEFT:
    default:
        break;
    }
    for (i = 0; i < layout->node_count; ++i) {
        node = &layout->nodes[i];
        if (!node->enabled || node->type == B3D_BIGHUD_NODE_ECG) continue;
        default_w = node->type == B3D_BIGHUD_NODE_BAR ? node->meter.rect.w : 1;
        default_h = node->type == B3D_BIGHUD_NODE_BAR ? node->meter.rect.h : 1;
        blank3d_bighud_resolve_rect(node, canvas_width, canvas_height,
                                    default_w, default_h,
                                    &local_x, &local_y, &local_w, &local_h);
        temp = *node;
        temp.anchor = B3D_BIGHUD_ANCHOR_TOP_LEFT;
        temp.has_pos = 1;
        temp.has_size = 1;
        temp.offset_x = base_x + b3d_scale_px(local_x, scale_x_q8);
        temp.offset_y = base_y + b3d_scale_px(local_y, scale_y_q8);
        temp.width = b3d_scale_px(local_w, scale_x_q8);
        temp.height = b3d_scale_px(local_h, scale_y_q8);
        if (node->type == B3D_BIGHUD_NODE_BAR) {
            blank3d_bighud_update_bar(node, telemetry, frame_ms);
            temp.meter = node->meter;
            temp.meter.rect.x = temp.offset_x;
            temp.meter.rect.y = temp.offset_y;
            temp.meter.rect.w = temp.width;
            temp.meter.rect.h = temp.height;
            b3d_scale_meter_pixels(&temp.meter, scale_x_q8, scale_y_q8);
            gbar89_draw(&temp.meter, &hud->render_ops);
        } else if (node->type == B3D_BIGHUD_NODE_COUNTER) {
            temp.counter_scale = b3d_scale_px(node->counter_scale, average_q8);
            temp.counter_gap = b3d_scale_px(node->counter_gap, average_q8);
            b3d_draw_counter(hud, &temp, telemetry);
        }
    }
}

static void b3d_draw_circle_outline(Blank3DHud *hud,
                                    int cx, int cy,
                                    int rx, int ry,
                                    unsigned long rgba)
{
    static const short unit_x[32] = {
        1024,1004,946,851,724,569,392,200,0,-200,-392,-569,-724,-851,-946,-1004,
        -1024,-1004,-946,-851,-724,-569,-392,-200,0,200,392,569,724,851,946,1004
    };
    static const short unit_y[32] = {
        0,200,392,569,724,851,946,1004,1024,1004,946,851,724,569,392,200,
        0,-200,-392,-569,-724,-851,-946,-1004,-1024,-1004,-946,-851,-724,-569,-392,-200
    };
    int i;
    int j;
    if (!hud || rx <= 0 || ry <= 0) return;
    b3d_gl_color(rgba);
    glBegin(GL_LINES);
    for (i = 0; i < 32; ++i) {
        j = (i + 1) & 31;
        glVertex2i(cx + (unit_x[i] * rx) / 1024,
                   hud->height - (cy + (unit_y[i] * ry) / 1024));
        glVertex2i(cx + (unit_x[j] * rx) / 1024,
                   hud->height - (cy + (unit_y[j] * ry) / 1024));
    }
    glEnd();
}

static int b3d_scope_sqrt(long value, int upper_bound)
{
    int low;
    int high;
    int mid;
    long square;
    if (value <= 0L || upper_bound <= 0) return 0;
    low = 0;
    high = upper_bound;
    while (low <= high) {
        mid = low + (high - low) / 2;
        square = (long)mid * (long)mid;
        if (square == value) return mid;
        if (square < value) low = mid + 1;
        else high = mid - 1;
    }
    return high < 0 ? 0 : high;
}

/*
 * Paint the whole area outside the circular lens.  The old implementation
 * covered only the four sides of the lens' bounding square, leaving the
 * square's corner wedges transparent.  Two curved quad strips close those
 * wedges without stencil buffers, floating point or dynamic memory.
 */
static void b3d_draw_scope_mask(Blank3DHud *hud,
                                int cx, int cy, int radius,
                                unsigned long rgba)
{
    enum { B3D_SCOPE_MASK_SEGMENTS = 128 };
    int i;
    int x;
    int dx;
    int arc_y;
    int top_y;
    int bottom_y;
    long remain;
    if (!hud || radius <= 0) return;

    b3d_draw_rect(hud, 0, 0, cx - radius, hud->height, rgba);
    b3d_draw_rect(hud, cx + radius, 0, hud->width - cx - radius,
                  hud->height, rgba);

    b3d_gl_color(rgba);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= B3D_SCOPE_MASK_SEGMENTS; ++i) {
        dx = -radius + (2 * radius * i) / B3D_SCOPE_MASK_SEGMENTS;
        remain = (long)radius * (long)radius - (long)dx * (long)dx;
        arc_y = b3d_scope_sqrt(remain, radius);
        x = cx + dx;
        top_y = cy - arc_y;
        if (top_y < 0) top_y = 0;
        glVertex2i(x, hud->height);
        glVertex2i(x, hud->height - top_y);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= B3D_SCOPE_MASK_SEGMENTS; ++i) {
        dx = -radius + (2 * radius * i) / B3D_SCOPE_MASK_SEGMENTS;
        remain = (long)radius * (long)radius - (long)dx * (long)dx;
        arc_y = b3d_scope_sqrt(remain, radius);
        x = cx + dx;
        bottom_y = cy + arc_y;
        if (bottom_y > hud->height) bottom_y = hud->height;
        glVertex2i(x, hud->height - bottom_y);
        glVertex2i(x, 0);
    }
    glEnd();
}

static unsigned long b3d_scope_rgba(gsp89_color color)
{
    return ((unsigned long)color.r << 24) |
           ((unsigned long)color.g << 16) |
           ((unsigned long)color.b << 8) |
           (unsigned long)color.a;
}

static void b3d_sniper_hud_emit(void *user, const gsh89_cmd *cmd)
{
    Blank3DHud *hud;
    int cx;
    int cy;
    int radius;
    unsigned long white;
    hud = (Blank3DHud *)user;
    if (!hud || !cmd) return;
    white = 0xE8F0E0FFUL;
    switch (cmd->kind) {
    case GSH89_CMD_SCOPE_MASK:
        cx = cmd->x0;
        cy = cmd->y0;
        radius = cmd->x1;
        b3d_draw_scope_mask(hud, cx, cy, radius, 0x000000F0UL);
        b3d_draw_circle_outline(hud, cx, cy, radius, radius, 0x101510FFUL);
        break;
    case GSH89_CMD_RETICLE_LINE:
    case GSH89_CMD_RANGE_TICK:
        b3d_draw_line(hud, cmd->x0, cmd->y0, cmd->x1, cmd->y1, white);
        break;
    case GSH89_CMD_RETICLE_DOT:
        b3d_draw_rect(hud, cmd->x0 - cmd->x1, cmd->y0 - cmd->y1,
                      cmd->x1 * 2, cmd->y1 * 2, white);
        break;
    case GSH89_CMD_BREATH_TELEMETRY:
        b3d_draw_rect(hud, hud->width / 2 - 75, hud->height - 42,
                      150, 7, 0x182018C0UL);
        b3d_draw_rect(hud, hud->width / 2 - 74, hud->height - 41,
                      (148 * cmd->x0) / 100, 5,
                      cmd->x1 ? 0xD05040FFUL : 0x90D080FFUL);
        break;
    case GSH89_CMD_TARGET_TELEMETRY:
        cx = hud->width / 2;
        cy = hud->height / 2;
        b3d_draw_line(hud, cx - 18, cy - 18, cx - 10, cy - 18,
                      0x80FF80FFUL);
        b3d_draw_line(hud, cx - 18, cy - 18, cx - 18, cy - 10,
                      0x80FF80FFUL);
        b3d_draw_line(hud, cx + 18, cy - 18, cx + 10, cy - 18,
                      0x80FF80FFUL);
        b3d_draw_line(hud, cx + 18, cy - 18, cx + 18, cy - 10,
                      0x80FF80FFUL);
        break;
    default:
        break;
    }
}

static void b3d_scope_preset_emit(void *user, const gsp89_draw_cmd *cmd)
{
    Blank3DHud *hud;
    unsigned long rgba;
    hud = (Blank3DHud *)user;
    if (!hud || !cmd) return;
    rgba = b3d_scope_rgba(cmd->color);
    switch (cmd->kind) {
    case GSP89_CMD_LINE:
        b3d_draw_line(hud, cmd->x0, cmd->y0, cmd->x1, cmd->y1, rgba);
        break;
    case GSP89_CMD_RECT:
        if (cmd->flags & GSP89_FLAG_FILLED)
            b3d_draw_rect(hud, cmd->x0, cmd->y0,
                          cmd->x1 - cmd->x0, cmd->y1 - cmd->y0, rgba);
        else {
            b3d_draw_line(hud, cmd->x0, cmd->y0, cmd->x1, cmd->y0, rgba);
            b3d_draw_line(hud, cmd->x1, cmd->y0, cmd->x1, cmd->y1, rgba);
            b3d_draw_line(hud, cmd->x1, cmd->y1, cmd->x0, cmd->y1, rgba);
            b3d_draw_line(hud, cmd->x0, cmd->y1, cmd->x0, cmd->y0, rgba);
        }
        break;
    case GSP89_CMD_TRIANGLE:
        b3d_gl_color(rgba);
        glBegin((cmd->flags & GSP89_FLAG_FILLED) ? GL_TRIANGLES : GL_LINES);
        glVertex2i(cmd->x0, hud->height - cmd->y0);
        glVertex2i(cmd->x1, hud->height - cmd->y1);
        glVertex2i(cmd->x2, hud->height - cmd->y2);
        if (!(cmd->flags & GSP89_FLAG_FILLED)) {
            glVertex2i(cmd->x1, hud->height - cmd->y1);
            glVertex2i(cmd->x2, hud->height - cmd->y2);
            glVertex2i(cmd->x0, hud->height - cmd->y0);
        }
        glEnd();
        break;
    case GSP89_CMD_CIRCLE:
    case GSP89_CMD_ELLIPSE:
    case GSP89_CMD_ARC:
        b3d_draw_circle_outline(hud, cmd->x0, cmd->y0,
                                cmd->radius_x, cmd->radius_y, rgba);
        break;
    case GSP89_CMD_SCOPE_MASK:
        break;
    default:
        break;
    }
}

/* BigVaderHudder now owns normal gameplay HUD composition.  The sniper
 * overlay remains a specialized combat layer and is drawn afterwards. */
void blank3d_hud_set_sprite_provider(Blank3DHud *hud,
                                     const Blank3DHudSpriteProvider *provider)
{
    if (!hud) return;
    if (provider) hud->sprite_provider = *provider;
    else memset(&hud->sprite_provider, 0, sizeof(hud->sprite_provider));
}

void blank3d_hud_init(Blank3DHud *hud)
{
    if (!hud) return;
    memset(hud, 0, sizeof(*hud));

    memset(&hud->render_ops, 0, sizeof(hud->render_ops));
    hud->render_ops.user = hud;
    hud->render_ops.draw_rect = b3d_draw_rect;
    hud->render_ops.draw_line = b3d_draw_line;
    hud->render_ops.draw_sprite = b3d_draw_sprite;
    hud->render_ops.draw_triangles = b3d_draw_triangles;
    hud->render_ops.push_clip = b3d_push_clip;
    hud->render_ops.pop_clip = b3d_pop_clip;

    blank3d_ecg_vitals_init(&hud->ecg);
    if (!blank3d_bighud_load(&hud->layout, &hud->ecg,
                             "config/hud/gameplay.bighud")) {
        fprintf(stderr, "[BigVaderHudder] %s\n",
                blank3d_bighud_error(&hud->layout));
    }
}

void blank3d_hud_draw(Blank3DHud *hud,
                      Blank3DNumbarSystem *numbars,
                      int width,
                      int height,
                      int player_health,
                      int clip,
                      int clip_capacity,
                      int reserve,
                      int first_person,
                      int muzzle_flash,
                      int threat_level,
                      unsigned int damage_flash_ms,
                      unsigned int frame_ms,
                      Blank3DSniper *sniper)
{
    int cx;
    int cy;
    int gap;
    Blank3DBigHudTelemetry telemetry;
    if (!hud || width <= 0 || height <= 0) return;
    hud->width = width;
    hud->height = height;
    if (clip_capacity < 1) clip_capacity = 1;
    if (reserve < 0) reserve = 0;

    blank3d_ecg_vitals_update(&hud->ecg, player_health, threat_level,
                               damage_flash_ms, frame_ms);
    memset(&telemetry, 0, sizeof(telemetry));
    telemetry.player_health = player_health;
    telemetry.player_health_max = 100L;
    telemetry.weapon_loaded = clip;
    telemetry.weapon_capacity = clip_capacity;
    telemetry.weapon_reserve = reserve;
    telemetry.gameplay_threat = threat_level;
    telemetry.ecg_bpm = (long)blank3d_ecg_vitals_bpm(&hud->ecg);
    telemetry.damage_flash_ms = (long)damage_flash_ms;
    telemetry.first_person = first_person;
    telemetry.muzzle_flash = muzzle_flash;

    b3d_begin_2d(width, height);
    b3d_draw_bighud(hud, &telemetry, frame_ms);
    if (numbars) blank3d_numbar_draw(numbars, hud, frame_ms);

    cx = width / 2;
    cy = height / 2;
    gap = first_person ? 5 : 8;
    if (!sniper || !blank3d_sniper_is_scoped(sniper)) {
        b3d_draw_line(hud, cx - 13, cy, cx - gap, cy, 0xFFFFFFFFUL);
        b3d_draw_line(hud, cx + gap, cy, cx + 13, cy, 0xFFFFFFFFUL);
        b3d_draw_line(hud, cx, cy - 13, cx, cy - gap, 0xFFFFFFFFUL);
        b3d_draw_line(hud, cx, cy + gap, cx, cy + 13, 0xFFFFFFFFUL);
    }
    if (muzzle_flash) {
        b3d_draw_rect(hud, cx - 3, cy - 3, 6, 6, 0xFFD040D0UL);
    }
    if (sniper)
        blank3d_sniper_emit(sniper, b3d_sniper_hud_emit, hud,
                            b3d_scope_preset_emit, hud);
    b3d_end_2d();
}
