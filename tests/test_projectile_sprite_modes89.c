#include <stdio.h>
#include <string.h>

#include "blank3d_projectile_sprite89.h"

static unsigned int test_upload(void *user, const unsigned char *rgba,
                                unsigned int width, unsigned int height)
{
    static unsigned int token = 100U;
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

static void base_recipe(Blank3DWeaponModules *m, int weapon_id)
{
    memset(m, 0, sizeof(*m));
    m->weapon_id = weapon_id;
    m->projectile_visual_billboard = 1;
    m->projectile_sprite_loop = GWM89_PROJECTILE_SPRITE_LOOP_FORWARD;
    m->projectile_visual_frame_ms = 33U;
    m->projectile_visual_phase_step = 1U;
    m->projectile_visual_width_scale_q16 = 65536L;
    m->projectile_visual_height_scale_q16 = 65536L;
    strcpy(m->projectile_visual_clip, "default");
}

int main(void)
{
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    AssetRoute89 routes;
    Blank3DProjectileSpriteRuntime89 runtime;
    Blank3DProjectileSpriteSample89 a;
    Blank3DProjectileSpriteSample89 b;
    Blank3DWeaponModules m;
    int fire_id;
    int muzzle_id;

    memset(&backend, 0, sizeof(backend));
    backend.upload_rgba = test_upload;
    backend.destroy_texture = test_destroy;
    blank3d_image_assets_init(&images);
    blank3d_image_assets_set_backend(&images, &backend);
    ar89_init(&routes);
    if (!ar89_register_explicit(&routes, AR89_KIND_IMAGE, "fire_atlas",
            "config/weapons/assets/flamethrower_fireloop_128_50_atlas.png") ||
        !ar89_register_explicit(&routes, AR89_KIND_IMAGE, "muzzle_flash",
            "config/weapons/assets/pistol_muzzle_flash.jpg") ||
        !ar89_register_explicit(&routes, AR89_KIND_IMAGE, "test_fire_strip8",
            "tests/fixtures/test_fire_strip8.png") ||
        !ar89_register_explicit(&routes, AR89_KIND_DATA, "test_sequence",
            "tests/data/projectile_visual_sequence.iseq89") ||
        !ar89_register_explicit(&routes, AR89_KIND_DATA, "test_renlist",
            "tests/data/projectile_visual_renlist.rnlist")) return 1;
    blank3d_image_assets_set_routes(&images, &routes);
    blank3d_projectile_sprite89_init(&runtime, &images, &routes);
    fire_id = blank3d_image_assets_resolve_request(&images, "fire_atlas");
    muzzle_id = blank3d_image_assets_resolve_request(&images, "muzzle_flash");
    if (fire_id <= 0 || muzzle_id <= 0 || fire_id == muzzle_id) return 2;

    /* 1. StaticSprite89 */
    base_recipe(&m, 101);
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_STATIC;
    strcpy(m.projectile_visual_image, "muzzle_flash");
    if (!blank3d_projectile_sprite89_sample(&runtime, &m, 0U, 0, &a) ||
        a.image_id != muzzle_id || a.source_enabled) {
        puts("StaticSprite89 projectile mode failed");
        return 3;
    }

    /* 2. ImageSequencer89 loose images */
    base_recipe(&m, 102);
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_SEQUENCE;
    strcpy(m.projectile_visual_recipe, "test_sequence");
    if (!blank3d_projectile_sprite89_sample(&runtime, &m, 10U, 0, &a) ||
        !blank3d_projectile_sprite89_sample(&runtime, &m, 45U, 0, &b) ||
        a.image_id != fire_id || b.image_id != muzzle_id) {
        puts("ImageSequencer89 projectile mode failed");
        return 4;
    }

    /* 3. RenList89 -> ImageSequencer89 */
    base_recipe(&m, 103);
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_RENLIST;
    strcpy(m.projectile_visual_recipe, "test_renlist");
    strcpy(m.projectile_visual_animation, "projectile");
    strcpy(m.projectile_visual_clip, "burn");
    if (!blank3d_projectile_sprite89_sample(&runtime, &m, 5U, 0, &a) ||
        !blank3d_projectile_sprite89_sample(&runtime, &m, 45U, 0, &b) ||
        a.image_id != fire_id || b.image_id != muzzle_id) {
        puts("RenList89 projectile mode failed");
        return 5;
    }

    /* 4. TileCell89 atlas/grid */
    base_recipe(&m, 104);
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_TILECELL;
    strcpy(m.projectile_visual_image, "fire_atlas");
    m.projectile_visual_frame_width = 128U;
    m.projectile_visual_frame_height = 128U;
    m.projectile_visual_frame_count = 50U;
    m.projectile_visual_frame_ms = 33U;
    m.projectile_visual_step_x = 1;
    m.projectile_visual_step_y = 0;
    if (!blank3d_projectile_sprite89_sample(&runtime, &m, 0U, 0, &a) ||
        !blank3d_projectile_sprite89_sample(&runtime, &m, 33U, 0, &b) ||
        a.image_id != fire_id || !a.source_enabled ||
        a.source_x != 0 || a.source_y != 0 || a.source_w != 128 ||
        b.source_x != 128 || b.source_y != 0) {
        puts("TileCell89 projectile mode failed");
        return 6;
    }

    /* 5. GMSpritestrip89. _strip8 is inferred from the routed filename. */
    base_recipe(&m, 105);
    m.projectile_sprite_mode = GWM89_PROJECTILE_SPRITE_GMSTRIP;
    strcpy(m.projectile_visual_image, "test_fire_strip8");
    m.projectile_visual_frame_count = 0U;
    m.projectile_visual_frame_ms = 25U;
    if (!blank3d_projectile_sprite89_sample(&runtime, &m, 0U, 0, &a) ||
        !blank3d_projectile_sprite89_sample(&runtime, &m, 25U, 0, &b) ||
        !a.source_enabled || a.source_w != 128 ||
        a.source_x != 0 || b.source_x != 128) {
        puts("GMSpritestrip89 projectile mode failed");
        return 7;
    }

    blank3d_image_assets_shutdown(&images);
    puts("Blank3D five projectile sprite modes: PASS");
    return 0;
}
