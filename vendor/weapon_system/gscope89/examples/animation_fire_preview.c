#include <stdio.h>
#include "gscopeanim89.h"
#include "gscopepresets89.h"

#define PREVIEW_MAX_SHAPES 512
static gsv89_shape preview_shapes[PREVIEW_MAX_SHAPES];
int gpr89_render_preset(const gsvp89_preset *preset, const char *filename);

static int render_frame(const gsvp89_preset *base, const gsa89_ctx *anim,
                        const char *filename, long *out_scale)
{
    gsa89_pose pose;
    gsvp89_preset p;
    gsa89_sample(anim, &pose);
    if (out_scale) *out_scale = pose.root.scale_x1000;
    if (gsa89_apply_shapes(&pose, base->shapes, base->shape_count,
                           preview_shapes, PREVIEW_MAX_SHAPES) != base->shape_count) return 0;
    p = *base;
    p.shapes = preview_shapes;
    return gpr89_render_preset(&p, filename);
}

int main(void)
{
    gsa89_ctx anim;
    const gsvp89_preset *preset;
    long scale;
    gsvp89_set_catalog_path("config/reticles/catalog.ini");
    if (!gsvp89_reload()) return 2;
    preset = gsvp89_find("fine_cross");
    if (!preset) return 3;
    if (!gsa89_load(&anim, "config/animations/fire_scale.ini")) return 4;
    if (!render_frame(preset, &anim, "tests/animation_frames/000_idle.ppm", &scale)) return 5;
    printf("frame=idle scale_x1000=%ld\n", scale);
    gsa89_trigger(&anim, "fire");
    gsa89_tick(&anim, 20);
    if (!render_frame(preset, &anim, "tests/animation_frames/020_fire.ppm", &scale)) return 6;
    printf("frame=20ms scale_x1000=%ld\n", scale);
    gsa89_tick(&anim, 15);
    if (!render_frame(preset, &anim, "tests/animation_frames/035_peak.ppm", &scale)) return 7;
    printf("frame=35ms scale_x1000=%ld\n", scale);
    gsa89_tick(&anim, 35);
    if (!render_frame(preset, &anim, "tests/animation_frames/070_return.ppm", &scale)) return 8;
    printf("frame=70ms scale_x1000=%ld\n", scale);
    gsa89_tick(&anim, 50);
    if (!render_frame(preset, &anim, "tests/animation_frames/120_settled.ppm", &scale)) return 9;
    printf("frame=120ms scale_x1000=%ld\n", scale);
    return 0;
}
