#include "blank3d_skybox_recipe89.h"
#include "assetroute89.h"
#include "assetroute89_posix.h"
#include "imgcc0.h"

#include <stdio.h>
#include <string.h>

typedef struct TestUploadTag {
    unsigned int count;
} TestUpload;

/* Portable test stub: recipe logic owns configuration, not the GL backend. */
void blank3d_skybox89_gl_attach(Blank3DSkybox89 *sky)
{
    (void)sky;
}

static unsigned int test_upload(void *user, const unsigned char *rgba,
                                unsigned int width, unsigned int height)
{
    TestUpload *upload;
    (void)rgba;
    if (!user || width == 0U || height == 0U) return 0U;
    upload = (TestUpload *)user;
    ++upload->count;
    return upload->count;
}

static void test_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

static int expect_loaded_format(Blank3DImageAssets *images, int id, int format)
{
    const Blank3DImageAsset *asset;
    if (id <= 0 || !blank3d_image_assets_load(images, id)) return 0;
    asset = blank3d_image_assets_get(images, id);
    return asset && asset->loaded && asset->width == 16U &&
           asset->height == 16U && asset->format == format;
}

int main(void)
{
    AssetRoute89 routes;
    AR89_FileProvider files;
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    Blank3DSkybox89 sky;
    Blank3DSkyboxRecipe89 recipes;
    TestUpload upload;
    int i;
    int shared;

    memset(&upload, 0, sizeof(upload));
    ar89_init(&routes);
    ar89_posix_make_provider(&files);
    ar89_set_file_provider(&routes, &files);
    if (!ar89_add_root(&routes, AR89_KIND_IMAGE,
                       "tests/fixtures/skybox_imgcc0", 1)) return 10;
    if (!ar89_scan_roots(&routes)) return 11;

    blank3d_image_assets_init(&images);
    memset(&backend, 0, sizeof(backend));
    backend.user = &upload;
    backend.upload_rgba = test_upload;
    backend.destroy_texture = test_destroy;
    blank3d_image_assets_set_backend(&images, &backend);
    blank3d_image_assets_set_routes(&images, &routes);

    blank3d_skybox89_init(&sky, &images);
    blank3d_skybox_recipe89_init(&recipes, &sky);
    blank3d_skybox_recipe89_set_catalog(
        &recipes, "tests/fixtures/skybox_recipes/catalog.ini");

    if (!blank3d_skybox_recipe89_apply_named(&recipes, "six")) {
        fprintf(stderr, "%s\n", blank3d_skybox_recipe89_status(&recipes));
        return 20;
    }
    if (!sky.enabled || !sky.cube_enabled || sky.screen_enabled || sky.dome_enabled)
        return 21;
    if (sky.radius_q12 != 91L * G3D_FIX_ONE) return 22;
    if (blank3d_skybox89_resolved_mask(&sky) != GSKYBOX89_CUBE_ALL_FACES)
        return 23;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_POS_X], IMGCC0_FMT_PNG)) return 24;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_NEG_X], IMGCC0_FMT_JPEG)) return 25;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_POS_Y], IMGCC0_FMT_BMP)) return 26;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_NEG_Y], IMGCC0_FMT_TGA)) return 27;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_POS_Z], IMGCC0_FMT_PCX)) return 28;
    if (!expect_loaded_format(&images, sky.face_image_id[GSKYBOX89_FACE_NEG_Z], IMGCC0_FMT_TIFF)) return 29;

    if (!blank3d_skybox_recipe89_apply_named(&recipes, "cross")) return 30;
    shared = sky.face_image_id[0];
    if (shared <= 0) return 31;
    for (i = 1; i < B3D_SKYBOX89_FACE_COUNT; ++i)
        if (sky.face_image_id[i] != shared) return 32;
    if (sky.face_uv[GSKYBOX89_FACE_POS_X].u0_q16 != SBR89_FX_ONE / 2L)
        return 33;
    if (sky.face_uv[GSKYBOX89_FACE_POS_X].v0_q16 != SBR89_FX_ONE / 3L)
        return 34;
    if (sky.uv_inset_pixels != 2) return 35;

    if (!blank3d_skybox_recipe89_apply_named(&recipes, "panorama")) return 40;
    if (!sky.enabled || sky.screen_enabled || sky.cube_enabled || !sky.dome_enabled)
        return 41;
    if (sky.dome_image_id <= 0) return 42;
    if (!expect_loaded_format(&images, sky.dome_image_id, IMGCC0_FMT_PNG)) return 43;

    if (!blank3d_skybox_recipe89_apply_named(&recipes, "nested")) return 50;
    if (sky.radius_q12 != 97L * G3D_FIX_ONE) return 51;
    if (strcmp(blank3d_skybox_recipe89_active_name(&recipes), "nested") != 0)
        return 52;
    if (!blank3d_skybox_recipe89_reload(&recipes)) return 53;

    blank3d_skybox_recipe89_disable(&recipes);
    if (sky.enabled) return 54;

    blank3d_image_assets_shutdown(&images);
    puts("PASS: TOML/RPY-ready skybox recipes + matryoshka + faces/atlas/panorama");
    return 0;
}
