#include <stdio.h>
#include <string.h>
#include "gscopeanim89.h"
#include "gscopepresets89.h"

#define TEST_MAX_SHAPES 512

static gsv89_shape transformed[TEST_MAX_SHAPES];

static int same_shape_fields(const gsv89_shape *a, const gsv89_shape *b)
{
    if (a->kind != b->kind || a->part_id != b->part_id || a->flags != b->flags ||
        a->layer != b->layer || a->thickness_px != b->thickness_px || a->outline_px != b->outline_px) return 0;
    if (a->x0 != b->x0 || a->y0 != b->y0 || a->x1 != b->x1 || a->y1 != b->y1 ||
        a->x2 != b->x2 || a->y2 != b->y2 || a->x3 != b->x3 || a->y3 != b->y3) return 0;
    if (a->a != b->a || a->b != b->b || a->c != b->c || a->d != b->d) return 0;
    if (a->i0 != b->i0 || a->i1 != b->i1 || a->i2 != b->i2 || a->i3 != b->i3) return 0;
    if (a->points != b->points || a->point_count != b->point_count) return 0;
    return 1;
}

int main(void)
{
    gsa89_ctx anim;
    gsa89_ctx master_anim;
    gri89_doc master_doc;
    gsa89_pose pose;
    const gsvp89_preset *preset;
    short i;
    short changed;
    long peak_scale;
    int failures;
    failures = 0;
    gsvp89_set_catalog_path("config/reticles/catalog.ini");
    if (!gsvp89_reload()) return 2;
    preset = gsvp89_find("fine_cross");
    if (!preset) return 3;
    if (!gsa89_load(&anim, "config/animations/fire_scale.ini")) return 4;

    /* The default HUD animation set must be identity until an event fires. */
    gri89_init(&master_doc);
    if (!gri89_load(&master_doc, "config/hud/sniper_default.ini")) return 5;
    if (!gsa89_load_doc(&master_anim, &master_doc)) return 6;
    gsa89_sample(&master_anim, &pose);
    if (pose.root.scale_x1000 != 1000L || pose.root.offset_x_norm != 0L ||
        pose.root.offset_y_norm != 0L || pose.root.alpha_x1000 != 1000L) ++failures;

    gsa89_sample(&anim, &pose);
    if (pose.root.scale_x1000 != 1000L) ++failures;
    if (gsa89_apply_shapes(&pose, preset->shapes, preset->shape_count, transformed, TEST_MAX_SHAPES) != preset->shape_count) ++failures;
    for (i = 0; i < preset->shape_count; ++i) if (!same_shape_fields(&preset->shapes[i], &transformed[i])) ++failures;

    if (gsa89_trigger(&anim, "fire") != 1) ++failures;
    gsa89_tick(&anim, 20);
    gsa89_sample(&anim, &pose);
    if (pose.root.scale_x1000 <= 1000L) ++failures;
    peak_scale = pose.root.scale_x1000;
    gsa89_apply_shapes(&pose, preset->shapes, preset->shape_count, transformed, TEST_MAX_SHAPES);
    changed = 0;
    for (i = 0; i < preset->shape_count; ++i) if (!same_shape_fields(&preset->shapes[i], &transformed[i])) changed = 1;
    if (!changed) ++failures;

    gsa89_tick(&anim, 100);
    gsa89_sample(&anim, &pose);
    if (pose.root.scale_x1000 != 1000L) ++failures;

    printf("clips=%d channels=%d fire_scale_peak=%ld settled=%ld failures=%d\n",
           (int)anim.clip_count, (int)anim.channel_count,
           peak_scale, pose.root.scale_x1000, failures);
    return failures ? 1 : 0;
}
