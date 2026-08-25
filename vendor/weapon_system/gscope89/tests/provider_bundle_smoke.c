#include <stdio.h>
#include <string.h>
#include "gscopebundle89.h"

static short provider_primitive_count = 0;
static short sink_cmd_count = 0;
static short provider_paint_seen = 0;
static short provider_animation_seen = 0;

static int host_primitive(void *user,
                          gsp89_painter *painter,
                          const gsv89_shape *shape,
                          const gsv89_palette *palette,
                          short global_alpha)
{
    (void)user;
    (void)painter;
    (void)palette;
    (void)global_alpha;
    ++provider_primitive_count;
    /* Demonstrate partial ownership: provider handles DOT only. */
    if (shape->kind == GSV89_SHAPE_DOT) return GPR89_HANDLED;
    return GPR89_FALLBACK;
}


static int host_animation(void *user,
                          const gsa89_pose *pose,
                          const gsv89_shape *src,
                          short shape_count,
                          gsv89_shape *dst,
                          short dst_capacity)
{
    (void)user;
    (void)pose;
    (void)src;
    (void)shape_count;
    (void)dst;
    (void)dst_capacity;
    ++provider_animation_seen;
    /* Observe animation, then let the internal fixed-point transform run. */
    return GPR89_FALLBACK;
}

static int host_paint(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    (void)cmd;
    ++provider_paint_seen;
    /* Observe paint but let the normal renderer sink consume it. */
    return GPR89_FALLBACK;
}

static void final_sink(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    (void)cmd;
    ++sink_cmd_count;
}

static short host_read_fov(void *user, short *out_fov)
{
    g89_camera *camera;
    camera = (g89_camera *)user;
    *out_fov = camera->base_fov_deg_x100;
    return 1;
}

static void host_write_fov(void *user, short base_fov, short current_fov)
{
    g89_camera *camera;
    camera = (g89_camera *)user;
    camera->base_fov_deg_x100 = base_fov;
    camera->current_fov_deg_x100 = current_fov;
}

int main(void)
{
    gscb89_ctx bundle;
    gpr89_provider host;
    gri89_doc recipe;
    gsp89_painter painter;
    const gsvp89_preset *preset;
    g89_camera camera;
    gtz89_profile zoom_profile;
    gtz89_ctx zoom;
    gscb89_ctx strict_bundle;
    gri89_doc strict_recipe;
    gsp89_painter strict_painter;
    gsv89_shape strict_shape;
    gsv89_palette strict_palette;
    gsa89_ctx anim;
    gsa89_pose anim_pose;
    gsv89_shape anim_scratch[256];
    short sink_before_anim;
    short primitives_before_anim;

    memset(&camera, 0, sizeof(camera));
    camera.base_fov_deg_x100 = 6000;
    camera.current_fov_deg_x100 = 6000;

    gscb89_init(&bundle);
    if (!gscb89_load_recipe(&bundle, "config/hud/sniper_pso1.ini", &recipe)) {
        printf("recipe_fail err=%d\n", bundle.last_error);
        return 1;
    }

    gpr89_provider_init(&host, "host");
    host.capabilities = GPR89_CAP_PRIMITIVE | GPR89_CAP_PAINT | GPR89_CAP_ZOOM | GPR89_CAP_ANIMATION;
    host.emit_primitive = host_primitive;
    host.emit_draw_cmd = host_paint;
    host.animate_shapes = host_animation;
    host.user = &camera;
    host.zoom_read_base_fov = host_read_fov;
    host.zoom_write_fov = host_write_fov;
    if (!gscb89_register_provider(&bundle, &host)) return 2;

    gscb89_painter_init(&bundle, &painter, 640, 480, final_sink, 0);
    preset = gscb89_preset_from_recipe(&bundle, &recipe, "vector_reticle");
    if (!preset) return 3;
    if (!gscb89_emit_preset(&bundle, &painter, preset, 0, 255)) return 4;

    /* Animation set is selected by the same master HUD recipe. */
    if (!gsa89_load_doc(&anim, &recipe)) return 15;
    if (gsa89_trigger(&anim, "fire") < 1) return 16;
    gsa89_tick(&anim, 20);
    gsa89_sample(&anim, &anim_pose);
    if (anim_pose.root.scale_x1000 <= 1000L) return 17;
    sink_before_anim = sink_cmd_count;
    primitives_before_anim = provider_primitive_count;
    if (!gscb89_emit_animated_preset(&bundle, &painter, preset, 0,
                                     &anim_pose, anim_scratch, 256, 255)) return 18;
    if (sink_cmd_count <= sink_before_anim) return 19;
    if (provider_animation_seen != 1) return 21;
    if ((short)(provider_primitive_count - primitives_before_anim) != preset->shape_count) return 20;

    if (!gtz89_profile_from_recipe(&recipe, &zoom_profile)) return 5;
    gtz89_init(&zoom, &zoom_profile, camera.base_fov_deg_x100);
    if (!gscb89_bind_zoom_provider(&bundle, &zoom)) return 6;
    gtz89_begin(&zoom);
    gtz89_update(&zoom, 1);

    printf("preset=%s shapes=%d primitive_provider_seen=%d paint_provider_seen=%d animation_provider_seen=%d sink=%d zoom_mode=%u fov=%d\n",
           preset->name, preset->shape_count,
           provider_primitive_count, provider_paint_seen, provider_animation_seen, sink_cmd_count,
           gtz89_get_provider_mode(&zoom), camera.current_fov_deg_x100);

    if (provider_primitive_count != (short)(preset->shape_count * 2)) return 7;
    if (provider_paint_seen < 1) return 8;
    if (sink_cmd_count < 1) return 9;
    if (gtz89_get_provider_mode(&zoom) == GTZ89_PROVIDER_INTERNAL) return 10;

    /* external:<name> must fail closed when the named provider is absent. */
    gscb89_init(&strict_bundle);
    gri89_init(&strict_recipe);
    if (!gri89_load(&strict_recipe, "config/providers/external_required_example.ini")) return 11;
    if (!gscb89_apply_provider_recipe(&strict_bundle, &strict_recipe)) return 12;
    gscb89_painter_init(&strict_bundle, &strict_painter, 320, 240, final_sink, 0);
    gsv89_palette_init(&strict_palette);
    strict_shape = gsv89_line(GSV89_PART_PRIMARY, -GSP89_FX_HALF, 0, GSP89_FX_HALF, 0);
    if (gscb89_emit_vector(&strict_bundle, &strict_painter, &strict_shape, 1, &strict_palette, 255)) return 13;
    if (strict_bundle.last_error != GSCB89_ERR_PROVIDER_REQUIRED) return 14;

    printf("strict_external_missing=blocked err=%d\n", strict_bundle.last_error);
    return 0;
}
