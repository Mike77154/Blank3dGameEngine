#include "blank3d_crosshair.h"

#include <stdio.h>
#include <string.h>

typedef struct DrawTag {
    int lines;
    int dots;
    int images;
    int last_image_id;
} Draw;

static void line_cb(void *user, int x0, int y0, int x1, int y1,
                    int thickness, unsigned long rgba)
{
    Draw *d = (Draw *)user;
    if (d) ++d->lines;
    (void)x0; (void)y0; (void)x1; (void)y1; (void)thickness; (void)rgba;
}

static void dot_cb(void *user, int x, int y, int diameter, unsigned long rgba)
{
    Draw *d = (Draw *)user;
    if (d) ++d->dots;
    (void)x; (void)y; (void)diameter; (void)rgba;
}

static void image_cb(void *user, int image_id, int x, int y,
                     int w, int h, unsigned long rgba)
{
    Draw *d = (Draw *)user;
    if (d) { ++d->images; d->last_image_id = image_id; }
    (void)x; (void)y; (void)w; (void)h; (void)rgba;
}

static int fail(const char *text)
{
    fprintf(stderr, "FAIL: %s\n", text);
    return 1;
}

int main(void)
{
    Blank3DCrosshair crosshair;
    GC89_DrawCallbacks cb;
    Draw draw;
    memset(&cb, 0, sizeof(cb));
    cb.draw_line = line_cb;
    cb.draw_dot = dot_cb;
    cb.draw_image = image_cb;
    memset(&draw, 0, sizeof(draw));

    if (!blank3d_crosshair_init(&crosshair,
            "tests/data/crosshair_images/gcrosshair.ini"))
        return fail(gcb89_recipe_last_error());
    blank3d_crosshair_set_primitive_fallback(&crosshair, &cb, &draw);

    if (!blank3d_crosshair_set_preset_name(&crosshair, "test_image"))
        return fail("image preset lookup");
    if (!blank3d_crosshair_draw(&crosshair, 1280, 720, 0, 0, 0, 0L, 1UL))
        return fail("image preset draw");
    if (draw.images != 1 || draw.lines != 0 || draw.last_image_id != 1000)
        return fail("image-only mode did not remain image-only");

    memset(&draw, 0, sizeof(draw));
    if (!blank3d_crosshair_set_preset_name(&crosshair, "test_hybrid"))
        return fail("hybrid preset lookup");
    if (!blank3d_crosshair_draw(&crosshair, 1280, 720, 0, 0, 0, 0L, 1UL))
        return fail("hybrid preset draw");
    if (draw.images != 1 || draw.lines <= 0 || draw.last_image_id != 1000)
        return fail("hybrid mode must emit raster and vector primitives");

    puts("GCrosshair89 image + hybrid vector preservation: PASS");
    return 0;
}
