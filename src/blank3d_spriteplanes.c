#include "blank3d_spriteplanes.h"

#include <string.h>

static sprpl89_color b3d_sp_color(unsigned long rgba)
{
    sprpl89_color c;
    c.r = (sprpl89_u8)((rgba >> 24) & 255UL);
    c.g = (sprpl89_u8)((rgba >> 16) & 255UL);
    c.b = (sprpl89_u8)((rgba >> 8) & 255UL);
    c.a = (sprpl89_u8)(rgba & 255UL);
    return c;
}

void blank3d_spriteplanes_init(Blank3DSpritePlaneWorld *world,
                               Blank3DImageAssets *images)
{
    if (!world) return;
    memset(world, 0, sizeof(*world));
    sprpl89_init(&world->ctx);
    world->images = images;
}

void blank3d_spriteplanes_clear(Blank3DSpritePlaneWorld *world)
{
    if (!world) return;
    sprpl89_clear_sprites(&world->ctx);
    memset(world->life_ms, 0, sizeof(world->life_ms));
}

static void b3d_sp_axes(sprpl89_sprite *spr, int axis)
{
    if (!spr) return;
    if (axis == 1) {
        spr->axis_right = sprpl89_v3(SP89_FX_ONE, 0, 0);
        spr->axis_up = sprpl89_v3(0, 0, SP89_FX_ONE);
    } else if (axis == 2) {
        spr->axis_right = sprpl89_v3(0, 0, SP89_FX_ONE);
        spr->axis_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    } else {
        spr->axis_right = sprpl89_v3(SP89_FX_ONE, 0, 0);
        spr->axis_up = sprpl89_v3(0, SP89_FX_ONE, 0);
    }
    spr->surface_right = spr->axis_right;
    spr->surface_up = spr->axis_up;
}

static int b3d_sp_frame_for_image(Blank3DSpritePlaneWorld *world,
                                  int image_id,
                                  long logical_w_q16,
                                  long logical_h_q16)
{
    sprpl89_atlas_frame frame;
    int i;
    if (!world || image_id <= 0) return -1;
    for (i = 0; i < world->ctx.frame_count; ++i) {
        if ((int)world->ctx.frames[i].page_id == image_id &&
            (int)world->ctx.frames[i].user_id == image_id)
            return i;
    }
    memset(&frame, 0, sizeof(frame));
    frame.u0 = 0; frame.v0 = 0;
    frame.u1 = SP89_FX_ONE; frame.v1 = SP89_FX_ONE;
    frame.pivot_x = SP89_FX_HALF; frame.pivot_y = SP89_FX_HALF;
    frame.logical_w = (sprpl89_fx)logical_w_q16;
    frame.logical_h = (sprpl89_fx)logical_h_q16;
    frame.page_id = (sprpl89_u16)image_id;
    frame.user_id = (sprpl89_u16)image_id;
    return sprpl89_add_frame(&world->ctx, &frame);
}

int blank3d_spriteplanes_spawn(Blank3DSpritePlaneWorld *world,
                               const Blank3DSpritePlaneSpec *spec)
{
    sprpl89_sprite spr;
    int frame_id;
    int sprite_id;
    if (!world || !spec || spec->image_id <= 0 ||
        spec->width_q16 <= 0 || spec->height_q16 <= 0) return -1;
    frame_id = b3d_sp_frame_for_image(world, spec->image_id,
                                      spec->width_q16, spec->height_q16);
    if (frame_id < 0) return -1;
    memset(&spr, 0, sizeof(spr));
    spr.active = 1;
    spr.flags = (sprpl89_u16)(SP89_FLAG_VISIBLE | SP89_FLAG_RASTER_LAYER |
                 SP89_FLAG_ALPHA_BLEND |
                 (spec->depth_test ? SP89_FLAG_DEPTH_TEST : 0));
    if (spec->blend_mode == SP89_BLEND_ADDITIVE)
        spr.flags = (sprpl89_u16)(spr.flags | SP89_FLAG_ADDITIVE);
    spr.billboard_mode = (sprpl89_u16)spec->billboard_mode;
    spr.frame_mode = SP89_FRAME_STATIC;
    spr.anchor_mode = SP89_ANCHOR_CENTER;
    spr.render_queue = (sprpl89_u16)(spec->blend_mode == SP89_BLEND_ADDITIVE
                       ? SP89_QUEUE_ADDITIVE : SP89_QUEUE_TRANSPARENT);
    spr.blend_mode = (sprpl89_u16)spec->blend_mode;
    spr.pos = sprpl89_v3((sprpl89_fx)spec->x_q16, (sprpl89_fx)spec->y_q16,
                      (sprpl89_fx)spec->z_q16);
    b3d_sp_axes(&spr, spec->plane_axis);
    spr.axis_lock = sprpl89_v3(0, SP89_FX_ONE, 0);
    spr.width = (sprpl89_fx)spec->width_q16;
    spr.height = (sprpl89_fx)spec->height_q16;
    spr.scale_x = SP89_FX_ONE;
    spr.scale_y = SP89_FX_ONE;
    spr.base_frame = (sprpl89_u16)frame_id;
    spr.frame_count = 1;
    spr.view_count = 1;
    spr.texture_page = (sprpl89_u16)spec->image_id;
    spr.color = b3d_sp_color(spec->tint_rgba ? spec->tint_rgba
                                             : 0xFFFFFFFFUL);
    sprite_id = sprpl89_spawn_sprite(&world->ctx, &spr);
    if (sprite_id >= 0 && sprite_id < SP89_MAX_SPRITES)
        world->life_ms[sprite_id] = spec->life_ms;
    return sprite_id;
}

int blank3d_spriteplanes_spawn_path(Blank3DSpritePlaneWorld *world,
                                    const char *path,
                                    const Blank3DSpritePlaneSpec *spec)
{
    Blank3DSpritePlaneSpec local;
    int id;
    if (!world || !world->images || !path || !spec) return -1;
    id = blank3d_image_assets_resolve_path(world->images, path);
    if (!id) return -1;
    local = *spec;
    local.image_id = id;
    return blank3d_spriteplanes_spawn(world, &local);
}

void blank3d_spriteplanes_update(Blank3DSpritePlaneWorld *world,
                                 unsigned int dt_ms)
{
    int i;
    if (!world) return;
    sprpl89_set_tick_ms(&world->ctx, world->ctx.tick_ms + (sprpl89_u32)dt_ms);
    for (i = 0; i < SP89_MAX_SPRITES; ++i) {
        sprpl89_sprite *spr;
        if (world->life_ms[i] == B3D_SPRITEPLANE_PERSISTENT) continue;
        spr = sprpl89_get_sprite(&world->ctx, i);
        if (!spr || !spr->active) { world->life_ms[i] = 0UL; continue; }
        if ((unsigned long)dt_ms >= world->life_ms[i]) {
            sprpl89_kill_sprite(&world->ctx, i);
            world->life_ms[i] = 0UL;
        } else world->life_ms[i] -= (unsigned long)dt_ms;
    }
}

int blank3d_spriteplanes_emit(Blank3DSpritePlaneWorld *world,
                              const sprpl89_camera *camera,
                              sprpl89_emit *out)
{
    int result;
    if (!world || !camera || !out) return SP89_ERR_BADARG;
    /* sprpl89_emit is a frame-local command buffer.  The upstream emitter
       intentionally does not clear it, because hosts may append several
       sprite families. Blank3D owns one complete world-sprite pass here, so
       carrying packets across frames creates immortal/ghost sprites even
       after sprpl89_kill_sprite() correctly retires their instances. */
    result = sprpl89_emit_begin(out);
    if (result != SP89_OK) return result;
    sprpl89_set_camera(&world->ctx, camera);
    return sprpl89_emit_all_sorted(&world->ctx, out);
}

int blank3d_spriteplanes_mode_from_text(const char *text)
{
    if (!text) return SP89_BILLBOARD_CAMERA_FACING;
    if (!strcmp(text, "fixed")) return SP89_BILLBOARD_FIXED;
    if (!strcmp(text, "view") || !strcmp(text, "view_aligned"))
        return SP89_BILLBOARD_VIEW_ALIGNED;
    if (!strcmp(text, "y") || !strcmp(text, "y_axis"))
        return SP89_BILLBOARD_Y_AXIS;
    if (!strcmp(text, "surface")) return SP89_BILLBOARD_SURFACE;
    if (!strcmp(text, "crossed_y")) return SP89_BILLBOARD_CROSSED_Y;
    return SP89_BILLBOARD_CAMERA_FACING;
}

int blank3d_spriteplanes_axis_from_text(const char *text)
{
    if (!text) return 0;
    if (!strcmp(text, "xz")) return 1;
    if (!strcmp(text, "yz")) return 2;
    return 0;
}
