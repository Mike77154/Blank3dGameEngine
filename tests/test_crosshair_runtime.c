#include <stdio.h>
#include <string.h>

#include "blank3d_crosshair.h"

typedef struct TestDrawTag {
    int lines;
    int dots;
    int images;
    int x0[8];
    int y0[8];
    int x1[8];
    int y1[8];
    int thickness[8];
    unsigned long rgba[8];
} TestDraw;

static void test_line(void *user,
                      int x0, int y0, int x1, int y1,
                      int thickness_px,
                      unsigned long rgba)
{
    TestDraw *draw;
    draw = (TestDraw *)user;
    if (draw) {
        if (draw->lines < 8) {
            draw->x0[draw->lines] = x0;
            draw->y0[draw->lines] = y0;
            draw->x1[draw->lines] = x1;
            draw->y1[draw->lines] = y1;
            draw->thickness[draw->lines] = thickness_px;
            draw->rgba[draw->lines] = rgba;
        }
        draw->lines++;
    }
}

static void test_dot(void *user,
                     int center_x, int center_y,
                     int diameter_px,
                     unsigned long rgba)
{
    TestDraw *draw;
    draw = (TestDraw *)user;
    if (draw) draw->dots++;
    (void)center_x; (void)center_y;
    (void)diameter_px; (void)rgba;
}

static void test_image(void *user,
                       int image_id,
                       int x, int y,
                       int width, int height,
                       unsigned long tint_rgba)
{
    TestDraw *draw;
    draw = (TestDraw *)user;
    if (draw) draw->images++;
    (void)image_id; (void)x; (void)y;
    (void)width; (void)height; (void)tint_rgba;
}

static int expect(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    static const char *presets[] = {
        "pistol_crisp",
        "smg_tracking",
        "shotgun_dynamic",
        "precision_dot",
        "sniper_hairline",
        "projectile_lead_circle",
        "projectile_lead_broken",
        "spray_control",
        "aoe_wide_ring"
    };
    Blank3DCrosshair crosshair;
    GC89_DrawCallbacks callbacks;
    TestDraw draw;
    const GC89A_Modifier *modifier;
    int i;
    int ok;

    ok = 1;
    memset(&draw, 0, sizeof(draw));
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.draw_line = test_line;
    callbacks.draw_dot = test_dot;
    callbacks.draw_image = test_image;

    ok &= expect(blank3d_crosshair_init(
                     &crosshair, "config/crosshair/gcrosshair.ini"),
                 gcb89_recipe_last_error());
    ok &= expect(GC89R_ABI_VERSION == 2,
                 "animation-capable runtime ABI 2 should be active");
    ok &= expect(gcb89_preset_count() == 194,
                 "Blank3D root should expose vendor 192 + 2 compatibility presets");
    ok &= expect(blank3d_crosshair_find_preset(
                     "dynamic_fire_bloom") == 187,
                 "new animation bundle presets should be reachable through the local root");
    ok &= expect(blank3d_crosshair_find_preset(
                     "blank3d_pistol_first_person") == 192,
                 "first-person legacy pistol recipe should be INI preset 192");
    ok &= expect(blank3d_crosshair_find_preset(
                     "blank3d_pistol_third_person") == 193,
                 "third-person legacy pistol recipe should be INI preset 193");

    /* Verify the OpenGL-equivalent primitive fallback route independently
       from the optional external provider slot. */
    blank3d_crosshair_set_primitive_fallback(&crosshair, &callbacks, &draw);
    ok &= expect(blank3d_crosshair_set_preset_name(
                     &crosshair, "blank3d_pistol_first_person"),
                 "first-person compatibility recipe should load");
    memset(&draw, 0, sizeof(draw));
    ok &= expect(blank3d_crosshair_draw(&crosshair,
                                         1280, 720,
                                         0, 0, 0, 0L, 1UL),
                 "primitive fallback should draw without provider");
    ok &= expect(draw.lines == 4 && draw.dots == 0 && draw.images == 0,
                 "legacy first-person recipe should emit four lines only");
    ok &= expect(draw.x0[0] == 627 && draw.y0[0] == 360 &&
                 draw.x1[0] == 635 && draw.y1[0] == 360 &&
                 draw.x0[1] == 645 && draw.y0[1] == 360 &&
                 draw.x1[1] == 653 && draw.y1[1] == 360 &&
                 draw.x0[2] == 640 && draw.y0[2] == 347 &&
                 draw.x1[2] == 640 && draw.y1[2] == 355 &&
                 draw.x0[3] == 640 && draw.y0[3] == 365 &&
                 draw.x1[3] == 640 && draw.y1[3] == 373,
                 "first-person recipe should reproduce old +/-13, gap 5 geometry");
    ok &= expect(draw.thickness[0] == 1 &&
                 draw.rgba[0] == 0xFFFFFFFFUL,
                 "first-person recipe should preserve 1px opaque white lines");

    /* The compatibility preset now has declarative ABI-2 event animation.
       Neutral geometry above remains exact; FIRE must transiently expand it. */
    ok &= expect(crosshair.runtime.animation_recipe.events
                     [GCB89_ANIM_EVENT_FIRE].enabled,
                 "Blank3D pistol FIRE animation should come from INI");
    memset(&draw, 0, sizeof(draw));
    ok &= expect(blank3d_crosshair_draw(&crosshair,
                                         1280, 720,
                                         0, 1, 0, 0L, 16UL),
                 "FIRE edge should draw through anim89 runtime");
    modifier = blank3d_crosshair_animation_modifier(&crosshair);
    ok &= expect(modifier && modifier->scale_fx > GC89_FX_ONE,
                 "FIRE recipe should expand the classic pistol crosshair");

    ok &= expect(blank3d_crosshair_trigger_event(
                     &crosshair, GCB89_ANIM_EVENT_CUSTOM1),
                 "manual custom1 recipe event should be exposed to Blank3D");
    ok &= expect(blank3d_crosshair_draw(&crosshair,
                                         1280, 720,
                                         0, 0, 0, 0L, 16UL),
                 "manual custom event should advance on frame clock");
    modifier = blank3d_crosshair_animation_modifier(&crosshair);
    ok &= expect(modifier && modifier->rotation_deg_fx != 0,
                 "custom1 should animate rotation from INI");

    memset(&draw, 0, sizeof(draw));
    ok &= expect(blank3d_crosshair_set_preset_name(
                     &crosshair, "blank3d_pistol_third_person"),
                 "third-person compatibility recipe should load");
    ok &= expect(blank3d_crosshair_draw(&crosshair,
                                         1280, 720,
                                         0, 0, 0, 0L, 1UL),
                 "third-person compatibility recipe should draw");
    ok &= expect(draw.lines == 4 &&
                 draw.x0[0] == 627 && draw.x1[0] == 632 &&
                 draw.x0[1] == 648 && draw.x1[1] == 653 &&
                 draw.y0[2] == 347 && draw.y1[2] == 352 &&
                 draw.y0[3] == 368 && draw.y1[3] == 373,
                 "third-person recipe should reproduce old +/-13, gap 8 geometry");

    ok &= expect(blank3d_crosshair_set_preset_name(
                     &crosshair, "dynamic_fire_bloom"),
                 "animation-rich preset should load");
    ok &= expect(blank3d_crosshair_trigger_event(
                     &crosshair, GCB89_ANIM_EVENT_FIRE),
                 "manual animation event bridge should work");

    /* An explicitly connected provider still overrides the fallback. */
    blank3d_crosshair_set_primitive_provider(&crosshair, &callbacks, &draw);

    for (i = 0; i < (int)(sizeof(presets) / sizeof(presets[0])); ++i) {
        int before;
        before = draw.lines + draw.dots + draw.images;
        ok &= expect(blank3d_crosshair_set_preset_name(&crosshair, presets[i]),
                     "weapon preset name should resolve through INI catalog");
        ok &= expect(blank3d_crosshair_draw(&crosshair,
                                             1280, 720,
                                             0, i == 2, 0,
                                             0L, 1UL),
                     "runtime draw should route through provider chain");
        ok &= expect(draw.lines + draw.dots + draw.images > before,
                     "selected preset should emit primitives");
    }

    ok &= expect(blank3d_crosshair_set_preset_name(&crosshair, "none"),
                 "none should be accepted as explicit disable");
    ok &= expect(!blank3d_crosshair_draw(&crosshair,
                                          1280, 720,
                                          0, 0, 0, 0L, 1UL),
                 "disabled crosshair should not draw");

    if (!ok) return 1;
    printf("Blank3D gcrosshair89 runtime integration: OK "
           "(%d lines, %d dots, %d images)\n",
           draw.lines, draw.dots, draw.images);
    return 0;
}
