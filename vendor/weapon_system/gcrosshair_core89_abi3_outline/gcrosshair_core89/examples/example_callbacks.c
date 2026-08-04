#include <stdio.h>
#include <string.h>
#include "gcrosshair_core89.h"

static void line_cb(void *user, int x0, int y0, int x1, int y1,
                    int thickness, unsigned long color)
{
    (void)user;
    printf("line %d,%d -> %d,%d thickness=%d rgba=%lu\n",
           x0, y0, x1, y1, thickness, color);
}

static void dot_cb(void *user, int x, int y, int diameter,
                   unsigned long color)
{
    (void)user;
    printf("dot %d,%d diameter=%d rgba=%lu\n", x, y, diameter, color);
}

static void image_cb(void *user, int image_id, int x, int y,
                     int width, int height, unsigned long tint)
{
    (void)user;
    printf("image id=%d rect=%d,%d %dx%d tint=%lu\n",
           image_id, x, y, width, height, tint);
}

int main(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89_DrawCallbacks cb;

    gc89_core_init(&core);
    memset(&spec, 0, sizeof(spec));
    spec.visible = 1;
    spec.draw_mode = GC89_DRAW_HYBRID;
    spec.arm_mask = GC89_ARM_ALL;
    spec.dot_enabled = 1;
    spec.gap_fx = GC89_FX_FROM_INT(9);
    spec.arm_length_fx = GC89_FX_FROM_INT(13);
    spec.thickness_fx = GC89_FX_FROM_INT(1);
    spec.dot_size_fx = GC89_FX_FROM_INT(3);
    spec.image_id = 7;
    spec.image_width_fx = GC89_FX_FROM_INT(48);
    spec.image_height_fx = GC89_FX_FROM_INT(48);
    spec.color_rgba = GC89_RGBA(209, 235, 255, 255);
    spec.image_tint_rgba = GC89_RGBA(255, 255, 255, 255);

    cb.draw_line = line_cb;
    cb.draw_dot = dot_cb;
    cb.draw_image = image_cb;
    gc89_core_draw(&core, 1280, 720, &spec, &cb, 0, 0);
    return 0;
}
