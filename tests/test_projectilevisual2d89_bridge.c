#include <stdio.h>
#include <string.h>

#include "blank3d_projectilevisual2d89_bridge.h"

static unsigned int test_upload(void *user, const unsigned char *rgba,
                                unsigned int width, unsigned int height)
{
    static unsigned int token = 300U;
    (void)user;
    if (!rgba || width == 0U || height == 0U) return 0U;
    ++token;
    return token;
}

static void test_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

int main(void)
{
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    AssetRoute89 routes;
    Blank3DProjectileSpriteRuntime89 sprites;
    Blank3DWeaponModules m;
    PV2D89Sample visual;
    Vec3 pos;
    Vec3 right;
    Vec3 up;
    int rc;

    memset(&backend, 0, sizeof(backend));
    backend.upload_rgba = test_upload;
    backend.destroy_texture = test_destroy;
    blank3d_image_assets_init(&images);
    blank3d_image_assets_set_backend(&images, &backend);
    ar89_init(&routes);
    if (!ar89_register_explicit(&routes, AR89_KIND_IMAGE, "fire_atlas",
            "config/weapons/assets/flamethrower_fireloop_128_50_atlas.png")) return 1;
    blank3d_image_assets_set_routes(&images, &routes);
    blank3d_projectile_sprite89_init(&sprites, &images, &routes);

    memset(&m, 0, sizeof(m));
    m.weapon_id = 14;
    m.projectile_visual = GWM89_PROJECTILE_VISUAL_EXPANDIBLE_FIRE;
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_TILECELL;
    m.projectile_visual_billboard = 1;
    m.projectile_sprite_loop = GWM89_PROJECTILE_SPRITE_LOOP_FORWARD;
    strcpy(m.projectile_visual_image, "fire_atlas");
    m.projectile_visual_frame_width = 128U;
    m.projectile_visual_frame_height = 128U;
    m.projectile_visual_frame_count = 50U;
    m.projectile_visual_frame_ms = 33U;
    m.projectile_visual_step_x = 1;
    m.projectile_visual_width_scale_q16 = 101581L;
    m.projectile_visual_height_scale_q16 = 137626L;
    m.projectile_visual_glow = 1;
    m.projectile_visual_glow_scale_q16 = 85197L;
    m.projectile_visual_glow_alpha = 105U;
    m.projectile_visual_core = 1;
    m.projectile_visual_core_scale_q16 = 47186L;
    m.projectile_visual_core_alpha = 150U;
    m.projectile_visual_additive = 1;
    m.projectile_visual_light = 1;
    m.projectile_visual_light_intensity_q16 = 78643L;
    m.projectile_visual_light_radius_q16 = 262144L;
    m.projectile_visual_light_r = 255U;
    m.projectile_visual_light_g = 112U;
    m.projectile_visual_light_b = 32U;

    pos.x = 4096; pos.y = 8192; pos.z = -12288;
    right.x = 4096; right.y = 0; right.z = 0;
    up.x = 0; up.y = 4096; up.z = 0;

    rc = blank3d_projectilevisual2d89_build(&sprites, &m,
        &pos, &right, &up, 4096, 33U, 1000U, 1, &visual);
    if (rc != PV2D89_OK || !visual.valid || !visual.replace_mesh ||
        visual.pass_count != 3 || visual.passes[1].source_x != 128 ||
        visual.passes[1].source_w != 128 ||
        visual.passes[1].corners_q16[0].x >= visual.passes[1].corners_q16[1].x ||
        !visual.light.enabled) {
        puts("Blank3D ProjectileVisual2D89 bridge failed");
        return 2;
    }

    blank3d_image_assets_shutdown(&images);
    puts("Blank3D ProjectileVisual2D89 bridge: PASS");
    return 0;
}
