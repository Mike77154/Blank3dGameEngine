#include <stdio.h>
#include <string.h>
#include "gcrosshair_core89.h"

typedef struct TestSink {
    int lines;
    int dots;
    int images;
    int last_x0;
    int last_y0;
} TestSink;

static void test_line(void *user, int x0, int y0, int x1, int y1,
                      int thickness_px, unsigned long color_rgba)
{
    TestSink *sink;
    sink = (TestSink *)user;
    sink->lines++;
    sink->last_x0 = x0;
    sink->last_y0 = y0;
    (void)x1;
    (void)y1;
    (void)thickness_px;
    (void)color_rgba;
}

static void test_dot(void *user, int x, int y, int diameter,
                     unsigned long color_rgba)
{
    TestSink *sink;
    sink = (TestSink *)user;
    sink->dots++;
    (void)x;
    (void)y;
    (void)diameter;
    (void)color_rgba;
}

static void test_image(void *user, int image_id, int x, int y,
                       int width, int height, unsigned long tint)
{
    TestSink *sink;
    sink = (TestSink *)user;
    sink->images++;
    (void)image_id;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)tint;
}

int main(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89_DrawCallbacks callbacks;
    GC89_DrawResult result;
    TestSink sink;
    int emitted;

    gc89_core_init(&core);
    memset(&spec, 0, sizeof(spec));
    memset(&sink, 0, sizeof(sink));
    spec.visible = 1;
    spec.draw_mode = GC89_DRAW_HYBRID;
    spec.arm_mask = GC89_ARM_ALL;
    spec.dot_enabled = 1;
    spec.gap_fx = GC89_FX_FROM_INT(9);
    spec.arm_length_fx = GC89_FX_FROM_INT(13);
    spec.thickness_fx = GC89_FX_FROM_INT(1);
    spec.dot_size_fx = GC89_FX_FROM_INT(3);
    spec.image_width_fx = GC89_FX_FROM_INT(32);
    spec.image_height_fx = GC89_FX_FROM_INT(32);
    spec.color_rgba = GC89_RGBA(209, 235, 255, 255);
    spec.image_tint_rgba = GC89_RGBA(255, 255, 255, 255);

    callbacks.draw_line = test_line;
    callbacks.draw_dot = test_dot;
    callbacks.draw_image = test_image;

    emitted = gc89_core_draw(&core, 640, 480, &spec, &callbacks, &sink, &result);
    if (emitted != 6 || sink.lines != 4 || sink.dots != 1 || sink.images != 1) return 1;
    if (result.center_x != 320 || result.center_y != 240) return 2;
    if (sink.last_y0 != 249) return 3;

    gc89_core_trigger_maxi(&core, GC89_ANIM_RETURN);
    gc89_core_update(&core, 16UL);
    if (core.scale_fx <= GC89_FX_ONE) return 4;

    printf("gcrosshair_core89: OK\n");
    return 0;
}
