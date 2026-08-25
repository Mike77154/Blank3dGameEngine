#include "blank3d_projectilevisual2d89_bridge.h"

#include <string.h>

typedef struct B3DPV2D89FrameContextTag {
    Blank3DProjectileSpriteRuntime89 *sprites;
    const Blank3DWeaponModules *modules;
} B3DPV2D89FrameContext;

static int b3d_pv2d89_sample_frame(void *user,
                                   int weapon_id,
                                   unsigned int age_ms,
                                   int projectile_slot,
                                   PV2D89Frame *out_frame)
{
    B3DPV2D89FrameContext *ctx;
    Blank3DProjectileSpriteSample89 sample;
    if (!user || !out_frame) return 0;
    ctx = (B3DPV2D89FrameContext *)user;
    if (!ctx->sprites || !ctx->modules ||
        ctx->modules->weapon_id != weapon_id) return 0;
    memset(&sample, 0, sizeof(sample));
    if (!blank3d_projectile_sprite89_sample(ctx->sprites, ctx->modules,
                                            age_ms, projectile_slot,
                                            &sample)) return 0;
    memset(out_frame, 0, sizeof(*out_frame));
    out_frame->valid = sample.valid ? 1 : 0;
    out_frame->image_handle = sample.image_id;
    out_frame->source_x = sample.source_x;
    out_frame->source_y = sample.source_y;
    out_frame->source_w = sample.source_w;
    out_frame->source_h = sample.source_h;
    out_frame->scale_x_q16 = sample.scale_x_q16;
    out_frame->scale_y_q16 = sample.scale_y_q16;
    out_frame->frame_index = sample.frame_index;
    if (sample.source_enabled) out_frame->flags |= PV2D89_FRAME_SOURCE_RECT;
    if (sample.flags & B3D_PSPR89_FLIP_X) out_frame->flags |= PV2D89_FRAME_FLIP_X;
    if (sample.flags & B3D_PSPR89_FLIP_Y) out_frame->flags |= PV2D89_FRAME_FLIP_Y;
    return out_frame->valid;
}

static PV2D89Color b3d_pv2d89_color(unsigned int r, unsigned int g,
                                    unsigned int b, unsigned int a)
{
    PV2D89Color c;
    c.r = (unsigned char)(r > 255U ? 255U : r);
    c.g = (unsigned char)(g > 255U ? 255U : g);
    c.b = (unsigned char)(b > 255U ? 255U : b);
    c.a = (unsigned char)(a > 255U ? 255U : a);
    return c;
}

void blank3d_projectilevisual2d89_recipe(
    const Blank3DWeaponModules *modules,
    PV2D89Recipe *out_recipe)
{
    int is_fire;
    if (!out_recipe) return;
    pv2d89_recipe_init(out_recipe);
    if (!modules) return;
    out_recipe->enabled = modules->projectile_visual_billboard &&
        modules->projectile_sprite_mode != GWM89_PROJECTILE_SPRITE_NONE;
    out_recipe->weapon_id = modules->weapon_id;
    out_recipe->billboard_mode = out_recipe->enabled ?
        PV2D89_BILLBOARD_VIEW : PV2D89_BILLBOARD_NONE;
    out_recipe->replace_mesh = 1;
    out_recipe->blend_mode = modules->projectile_visual_additive ?
        PV2D89_BLEND_ADDITIVE : PV2D89_BLEND_ALPHA;
    out_recipe->width_scale_q16 = modules->projectile_visual_width_scale_q16 > 0L ?
        modules->projectile_visual_width_scale_q16 : PV2D89_Q16_ONE;
    out_recipe->height_scale_q16 = modules->projectile_visual_height_scale_q16 > 0L ?
        modules->projectile_visual_height_scale_q16 : PV2D89_Q16_ONE;
    out_recipe->phase_step = modules->projectile_visual_phase_step;

    is_fire = modules->projectile_visual == GWM89_PROJECTILE_VISUAL_EXPANDIBLE_FIRE;
    if (is_fire) {
        out_recipe->pulse_min_q16 = 58982L; /* .90 */
        out_recipe->pulse_max_q16 = 72090L; /* 1.10 */
        out_recipe->phase_flip_x = 1;
        out_recipe->age_fade = 1;
        out_recipe->main_start = b3d_pv2d89_color(255U, 238U, 196U, 215U);
        out_recipe->main_mid = b3d_pv2d89_color(255U, 205U, 132U, 215U);
        out_recipe->main_end = b3d_pv2d89_color(255U, 154U, 72U, 215U);
    } else {
        out_recipe->pulse_min_q16 = PV2D89_Q16_ONE;
        out_recipe->pulse_max_q16 = PV2D89_Q16_ONE;
        out_recipe->main_start = b3d_pv2d89_color(255U, 255U, 255U, 255U);
        out_recipe->main_mid = out_recipe->main_start;
        out_recipe->main_end = out_recipe->main_start;
    }

    out_recipe->glow_enabled = modules->projectile_visual_glow ? 1 : 0;
    out_recipe->glow_scale_q16 = modules->projectile_visual_glow_scale_q16 > 0L ?
        modules->projectile_visual_glow_scale_q16 : PV2D89_Q16_ONE;
    if (is_fire)
        out_recipe->glow_color = b3d_pv2d89_color(255U, 104U, 28U,
            modules->projectile_visual_glow_alpha);
    else
        out_recipe->glow_color = b3d_pv2d89_color(255U, 255U, 255U,
            modules->projectile_visual_glow_alpha);

    out_recipe->core_enabled = modules->projectile_visual_core ? 1 : 0;
    out_recipe->core_scale_q16 = modules->projectile_visual_core_scale_q16 > 0L ?
        modules->projectile_visual_core_scale_q16 : PV2D89_Q16_ONE;
    out_recipe->core_color = is_fire ?
        b3d_pv2d89_color(255U, 248U, 214U,
            modules->projectile_visual_core_alpha) :
        b3d_pv2d89_color(255U, 255U, 255U,
            modules->projectile_visual_core_alpha);

    out_recipe->light_enabled = modules->projectile_visual_light ? 1 : 0;
    out_recipe->light_intensity_q16 = modules->projectile_visual_light_intensity_q16;
    out_recipe->light_radius_q16 = modules->projectile_visual_light_radius_q16;
    out_recipe->light_color = b3d_pv2d89_color(
        modules->projectile_visual_light_r,
        modules->projectile_visual_light_g,
        modules->projectile_visual_light_b, 255U);
}

int blank3d_projectilevisual2d89_build(
    Blank3DProjectileSpriteRuntime89 *sprites,
    const Blank3DWeaponModules *modules,
    const Vec3 *position_q12,
    const Vec3 *camera_right_q12,
    const Vec3 *camera_up_q12,
    g3d_fix mesh_scale_q12,
    unsigned int age_ms,
    unsigned int life_ms,
    int projectile_slot,
    PV2D89Sample *out_sample)
{
    PV2D89Recipe recipe;
    PV2D89ProjectileInput input;
    PV2D89FrameProvider provider;
    B3DPV2D89FrameContext frame_ctx;
    if (!sprites || !modules || !position_q12 || !camera_right_q12 ||
        !camera_up_q12 || !out_sample) return PV2D89_ERR_ARGUMENT;
    blank3d_projectilevisual2d89_recipe(modules, &recipe);
    if (!pv2d89_wants_billboard(&recipe)) {
        pv2d89_sample_clear(out_sample);
        return PV2D89_NO_VISUAL;
    }
    memset(&input, 0, sizeof(input));
    input.position_q16.x = (long)position_q12->x * 16L;
    input.position_q16.y = (long)position_q12->y * 16L;
    input.position_q16.z = (long)position_q12->z * 16L;
    input.camera_right_q16.x = (long)camera_right_q12->x * 16L;
    input.camera_right_q16.y = (long)camera_right_q12->y * 16L;
    input.camera_right_q16.z = (long)camera_right_q12->z * 16L;
    input.camera_up_q16.x = (long)camera_up_q12->x * 16L;
    input.camera_up_q16.y = (long)camera_up_q12->y * 16L;
    input.camera_up_q16.z = (long)camera_up_q12->z * 16L;
    input.mesh_scale_q16 = (long)mesh_scale_q12 * 16L;
    input.age_ms = age_ms;
    input.life_ms = life_ms;
    input.projectile_slot = projectile_slot;

    frame_ctx.sprites = sprites;
    frame_ctx.modules = modules;
    provider.user = &frame_ctx;
    provider.sample_frame = b3d_pv2d89_sample_frame;
    return pv2d89_build(&recipe, &input, &provider, out_sample);
}
