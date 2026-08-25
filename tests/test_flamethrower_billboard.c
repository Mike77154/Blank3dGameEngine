#include <stdio.h>
#include <string.h>

#include "blank3d_flamethrower_billboard89.h"
#include "blank3d_image_assets.h"

static unsigned int seen_width;
static unsigned int seen_height;
static int seen_transparent_corner;
static int seen_hot_center;

static unsigned int test_upload(void *user, const unsigned char *rgba,
                                unsigned int width, unsigned int height)
{
    (void)user;
    if (!rgba || width == 0U || height == 0U) return 0U;
    seen_width = width;
    seen_height = height;
    if (width > 64U && height > 64U) {
        unsigned long center_index;
        seen_transparent_corner = rgba[3] == 0U;
        center_index = ((unsigned long)64U * (unsigned long)width + 64UL) * 4UL;
        seen_hot_center = rgba[center_index + 3UL] > 200U;
    }
    return 1U;
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
    Blank3DWeaponModules recipe;
    Blank3DFlameBillboardSample young;
    Blank3DFlameBillboardSample middle;
    Blank3DFlameBillboardSample old;
    int image_id;

    memset(&backend, 0, sizeof(backend));
    memset(&recipe, 0, sizeof(recipe));
    backend.upload_rgba = test_upload;
    backend.destroy_texture = test_destroy;
    blank3d_image_assets_init(&images);
    blank3d_image_assets_set_backend(&images, &backend);
    ar89_init(&routes);
    if (!ar89_register_explicit(&routes, AR89_KIND_IMAGE,
            "test_fire",
            "config/weapons/assets/flamethrower_fireloop_128_50_atlas.png"))
        return 1;
    blank3d_image_assets_set_routes(&images, &routes);
    image_id = blank3d_image_assets_resolve_request(&images, "test_fire");
    if (image_id <= 0 || !blank3d_image_assets_load(&images, image_id)) {
        printf("generic projectile atlas route/load failed: %s\n",
               blank3d_image_assets_error(&images));
        return 2;
    }
    if (seen_width != 1024U || seen_height != 896U ||
        !seen_transparent_corner || !seen_hot_center) {
        puts("generic projectile atlas dimensions/alpha failed");
        return 3;
    }

    strcpy(recipe.projectile_visual_image, "test_fire");
    recipe.projectile_visual_frame_width = 128U;
    recipe.projectile_visual_frame_height = 128U;
    recipe.projectile_visual_columns = 8U;
    recipe.projectile_visual_rows = 7U;
    recipe.projectile_visual_frame_count = 50U;
    recipe.projectile_visual_frame_ms = 33U;
    recipe.projectile_visual_phase_step = 7U;
    recipe.projectile_visual_width_scale_q16 = 101581L;
    recipe.projectile_visual_height_scale_q16 = 137626L;
    recipe.projectile_visual_glow = 1;
    recipe.projectile_visual_glow_scale_q16 = 85197L;
    recipe.projectile_visual_glow_alpha = 105U;
    recipe.projectile_visual_core = 1;
    recipe.projectile_visual_core_scale_q16 = 47186L;
    recipe.projectile_visual_core_alpha = 150U;

    blank3d_projectile_billboard_sample(&recipe, 0U, 1200U, 0,
                                        G3D_FIX_ONE / 4, &young);
    blank3d_projectile_billboard_sample(&recipe, 600U, 1200U, 7,
                                        G3D_FIX_ONE, &middle);
    blank3d_projectile_billboard_sample(&recipe, 1199U, 1200U, 13,
                                        G3D_FIX_ONE, &old);
    if (young.frame_index >= recipe.projectile_visual_frame_count ||
        middle.frame_index >= recipe.projectile_visual_frame_count ||
        old.frame_index >= recipe.projectile_visual_frame_count ||
        young.atlas_column >= recipe.projectile_visual_columns ||
        middle.atlas_row >= recipe.projectile_visual_rows ||
        old.atlas_row >= recipe.projectile_visual_rows) {
        puts("generic projectile billboard frame routing failed");
        return 4;
    }
    if (young.width_q12 <= 0 || young.height_q12 <= 0 ||
        middle.width_q12 <= young.width_q12 ||
        middle.height_q12 <= young.height_q12) {
        puts("generic projectile billboard growth failed");
        return 5;
    }
    if (!(young.glow_a > middle.glow_a && middle.glow_a > old.glow_a)) {
        puts("generic projectile billboard fade failed");
        return 6;
    }
    if (young.frame_index == middle.frame_index &&
        middle.frame_index == old.frame_index) {
        puts("generic projectile billboard phase failed");
        return 7;
    }
    blank3d_image_assets_shutdown(&images);
    puts("Blank3D generic AssetRoute projectile billboard recipe: PASS");
    return 0;
}
