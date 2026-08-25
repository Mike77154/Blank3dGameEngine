#include <stdio.h>
#include <string.h>
#include "gcrosshair_provider89.h"

static int line_count;
static int dot_count;
static int vector_calls;
static int asset_calls;
static int hud_begin_calls;
static int hud_end_calls;
static int last_semantic_id;
static int first_line_x0;
static int first_line_y0;

static void count_line(void *user, int x0, int y0, int x1, int y1,
                       int thickness, unsigned long color)
{
    (void)user; (void)x0; (void)y0; (void)x1; (void)y1;
    (void)thickness; (void)color;
    if (line_count == 0) {
        first_line_x0 = x0;
        first_line_y0 = y0;
    }
    line_count++;
}

static void count_dot(void *user, int x, int y, int diameter,
                      unsigned long color)
{
    (void)user; (void)x; (void)y; (void)diameter; (void)color;
    dot_count++;
}

static int handle_vector(void *user,
                         const GC89P_VectorCommand *command,
                         int *out_emitted)
{
    int *mode;
    mode = (int *)user;
    vector_calls++;
    if (!command || !command->spec) return GC89P_UNHANDLED;
    last_semantic_id = command->meta ? command->meta->semantic_id : -1;
    if (*mode == 0) return GC89P_UNHANDLED;
    *out_emitted = 77;
    return GC89P_HANDLED;
}

static int handle_asset(void *user, int image_id,
                        int x, int y, int w, int h,
                        unsigned long tint)
{
    int *enabled;
    enabled = (int *)user;
    (void)image_id; (void)x; (void)y; (void)w; (void)h; (void)tint;
    asset_calls++;
    return *enabled ? GC89P_HANDLED : GC89P_UNHANDLED;
}

static int hud_viewport(void *user, int *w, int *h)
{
    (void)user;
    *w = 800;
    *h = 600;
    return 1;
}

static int hud_anchor(void *user, int w, int h, int *x, int *y)
{
    (void)user; (void)w; (void)h;
    *x = 123;
    *y = 234;
    return 1;
}

static void hud_begin(void *user, int x, int y, const GC89_DrawSpec *spec)
{
    (void)user; (void)x; (void)y; (void)spec;
    hud_begin_calls++;
}

static void hud_end(void *user, int emitted)
{
    (void)user; (void)emitted;
    hud_end_calls++;
}

static void make_spec(GC89_DrawSpec *spec)
{
    memset(spec, 0, sizeof(*spec));
    spec->visible = 1;
    spec->draw_mode = GC89_DRAW_VECTOR;
    spec->arm_mask = GC89_ARM_ALL;
    spec->dot_enabled = 1;
    spec->gap_fx = GC89_FX_FROM_INT(4);
    spec->arm_length_fx = GC89_FX_FROM_INT(8);
    spec->thickness_fx = GC89_FX_FROM_INT(1);
    spec->dot_size_fx = GC89_FX_FROM_INT(3);
    spec->color_rgba = GC89_RGBA(255, 255, 255, 255);
    spec->shape_type = GC89_SHAPE_CROSS;
    spec->shape_segment_mask = GC89_SEGMENT_ALL;
    spec->shape_direction_mask = GC89_DIRECTION_ALL;
}

static int test_vector_provider_and_fallback(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_Runtime runtime;
    GC89P_VectorProvider vector_provider;
    GC89_DrawCallbacks primitives;
    GC89P_DrawReport report;
    GC89P_DrawMeta meta;
    int mode;

    gc89_core_init(&core);
    make_spec(&spec);
    gc89p_runtime_init(&runtime);
    memset(&vector_provider, 0, sizeof(vector_provider));
    memset(&primitives, 0, sizeof(primitives));
    vector_provider.draw_vector = handle_vector;
    primitives.draw_line = count_line;
    primitives.draw_dot = count_dot;
    mode = 1;
    meta.semantic_id = 39;
    meta.semantic_name = "ring_small";
    meta.semantic_category = "circular";
    gc89p_runtime_set_vector_provider(&runtime, &vector_provider, &mode);
    gc89p_runtime_set_primitive_provider(&runtime, &primitives, 0);

    line_count = 0; dot_count = 0; vector_calls = 0;
    last_semantic_id = -1;
    if (gc89p_draw_ex(&runtime, &core, 640, 480, &spec, &meta, &report) != 77)
        return 0;
    if (last_semantic_id != 39) return 0;
    if (!report.vector_provider_used || report.internal_vector_used)
        return 0;
    if (line_count != 0 || dot_count != 0 || vector_calls != 1)
        return 0;

    mode = 0;
    line_count = 0; dot_count = 0; vector_calls = 0;
    if (gc89p_draw(&runtime, &core, 640, 480, &spec, &report) <= 0)
        return 0;
    if (report.vector_provider_used || !report.internal_vector_used)
        return 0;
    if (line_count != 4 || dot_count != 1 || vector_calls != 1)
        return 0;
    return 1;
}

static int test_hud_and_asset(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_Runtime runtime;
    GC89P_AssetProvider asset;
    GC89P_HudProvider hud;
    GC89P_DrawReport report;
    int enabled;

    gc89_core_init(&core);
    make_spec(&spec);
    spec.draw_mode = GC89_DRAW_IMAGE;
    spec.image_id = 1000;
    spec.image_width_fx = GC89_FX_FROM_INT(32);
    spec.image_height_fx = GC89_FX_FROM_INT(32);
    spec.image_tint_rgba = GC89_RGBA(255, 255, 255, 255);
    gc89p_runtime_init(&runtime);
    memset(&asset, 0, sizeof(asset));
    memset(&hud, 0, sizeof(hud));
    asset.draw_asset = handle_asset;
    hud.get_viewport = hud_viewport;
    hud.get_anchor = hud_anchor;
    hud.begin = hud_begin;
    hud.end = hud_end;
    enabled = 1;
    gc89p_runtime_set_asset_provider(&runtime, &asset, &enabled);
    gc89p_runtime_set_hud_provider(&runtime, &hud, 0);

    asset_calls = 0; hud_begin_calls = 0; hud_end_calls = 0;
    if (gc89p_draw(&runtime, &core, 1, 1, &spec, &report) != 1)
        return 0;
    if (!report.asset_provider_used || !report.hud_provider_used)
        return 0;
    if (report.center_x != 123 || report.center_y != 234)
        return 0;
    if (asset_calls != 1 || hud_begin_calls != 1 || hud_end_calls != 1)
        return 0;
    return 1;
}


static int test_hud_anchor_with_internal_vector(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_Runtime runtime;
    GC89P_HudProvider hud;
    GC89_DrawCallbacks primitives;
    GC89P_DrawReport report;

    gc89_core_init(&core);
    make_spec(&spec);
    spec.dot_enabled = 0;
    gc89p_runtime_init(&runtime);
    memset(&hud, 0, sizeof(hud));
    memset(&primitives, 0, sizeof(primitives));
    hud.get_viewport = hud_viewport;
    hud.get_anchor = hud_anchor;
    primitives.draw_line = count_line;
    gc89p_runtime_set_hud_provider(&runtime, &hud, 0);
    gc89p_runtime_set_primitive_provider(&runtime, &primitives, 0);

    line_count = 0;
    first_line_x0 = 0;
    first_line_y0 = 0;
    if (gc89p_draw(&runtime, &core, 1, 1, &spec, &report) <= 0)
        return 0;
    if (!report.internal_vector_used || report.center_x != 123 ||
        report.center_y != 234)
        return 0;
    /* Left arm: center 123, gap 4, arm 8 => x0=111, y=234. */
    if (first_line_x0 != 111 || first_line_y0 != 234)
        return 0;
    return 1;
}

static int test_software_surface(void)
{
    GC89_Core core;
    GC89_DrawSpec spec;
    GC89P_SoftwareSurface surface;
    GC89P_DrawReport report;
    unsigned char rgba[64 * 64 * 4];
    int i;
    int nonzero;

    memset(rgba, 0, sizeof(rgba));
    gc89p_surface_init(&surface, rgba, 64, 64, 64 * 4);
    gc89p_surface_clear(&surface, GC89_RGBA(0, 0, 0, 0));
    gc89_core_init(&core);
    make_spec(&spec);
    if (gc89p_draw_rgba(&core, &spec, &surface, &report) <= 0)
        return 0;
    nonzero = 0;
    for (i = 0; i < (int)sizeof(rgba); ++i) {
        if (rgba[i] != 0) {
            nonzero = 1;
            break;
        }
    }
    if (!nonzero || !report.internal_vector_used) return 0;
    return 1;
}

int main(void)
{
    if (!test_vector_provider_and_fallback()) {
        puts("FAIL vector provider/fallback");
        return 1;
    }
    if (!test_hud_and_asset()) {
        puts("FAIL hud/asset provider");
        return 1;
    }
    if (!test_hud_anchor_with_internal_vector()) {
        puts("FAIL hud anchor/internal vector");
        return 1;
    }
    if (!test_software_surface()) {
        puts("FAIL software surface");
        return 1;
    }
    puts("provider89 tests: OK");
    return 0;
}
