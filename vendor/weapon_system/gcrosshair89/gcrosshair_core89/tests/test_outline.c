#include <stdio.h>
#include <string.h>
#include "gcrosshair_core89.h"

typedef struct OutlineSink {
    int lines;
    int dots;
    int line_thickness[8];
    unsigned long line_color[8];
    int dot_diameter[2];
    unsigned long dot_color[2];
} OutlineSink;

static void outline_line(void *user,
                         int x0, int y0, int x1, int y1,
                         int thickness_px,
                         unsigned long color_rgba)
{
    OutlineSink *sink;
    sink = (OutlineSink *)user;
    if (sink->lines < 8) {
        sink->line_thickness[sink->lines] = thickness_px;
        sink->line_color[sink->lines] = color_rgba;
    }
    sink->lines++;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
}

static void outline_dot(void *user,
                        int center_x, int center_y,
                        int diameter_px,
                        unsigned long color_rgba)
{
    OutlineSink *sink;
    sink = (OutlineSink *)user;
    if (sink->dots < 2) {
        sink->dot_diameter[sink->dots] = diameter_px;
        sink->dot_color[sink->dots] = color_rgba;
    }
    sink->dots++;
    (void)center_x;
    (void)center_y;
}

int main(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89_DrawCallbacks callbacks;
    OutlineSink sink;
    int emitted;
    int i;

    gc89_core_init(&core);
    memset(&spec, 0, sizeof(spec));
    memset(&sink, 0, sizeof(sink));

    spec.visible = 1;
    spec.draw_mode = GC89_DRAW_VECTOR;
    spec.arm_mask = GC89_ARM_ALL;
    spec.dot_enabled = 1;
    spec.gap_fx = GC89_FX_FROM_INT(5);
    spec.arm_length_fx = GC89_FX_FROM_INT(8);
    spec.thickness_fx = GC89_FX_FROM_INT(1);
    spec.dot_size_fx = GC89_FX_FROM_INT(3);
    spec.color_rgba = GC89_RGBA(255, 255, 255, 255);
    spec.shape_type = GC89_SHAPE_CROSS;
    spec.outline_enabled = 1;
    spec.outline_width_fx = GC89_FX_FROM_INT(1);
    spec.outline_color_rgba = GC89_RGBA(0, 0, 0, 255);

    callbacks.draw_line = outline_line;
    callbacks.draw_dot = outline_dot;
    callbacks.draw_image = 0;

    emitted = gc89_core_draw(&core, 640, 480, &spec,
                             &callbacks, &sink, 0);
    if (emitted != 10) return 1;
    if (sink.lines != 8 || sink.dots != 2) return 2;

    for (i = 0; i < 4; ++i) {
        if (sink.line_thickness[i] != 3) return 3;
        if (sink.line_color[i] != GC89_RGBA(0, 0, 0, 255)) return 4;
    }
    for (i = 4; i < 8; ++i) {
        if (sink.line_thickness[i] != 1) return 5;
        if (sink.line_color[i] != GC89_RGBA(255, 255, 255, 255)) return 6;
    }
    if (sink.dot_diameter[0] != 5 || sink.dot_diameter[1] != 3) return 7;
    if (sink.dot_color[0] != GC89_RGBA(0, 0, 0, 255)) return 8;
    if (sink.dot_color[1] != GC89_RGBA(255, 255, 255, 255)) return 9;

    puts("gcrosshair_core89 ABI3 outline: OK");
    return 0;
}
