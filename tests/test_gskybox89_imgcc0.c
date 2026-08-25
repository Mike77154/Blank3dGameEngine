#include "blank3d_skybox89.h"
#include "assetroute89.h"
#include "assetroute89_posix.h"
#include "imgcc0.h"

#include <stdio.h>
#include <string.h>

typedef struct TestUploadTag {
    unsigned int count;
    unsigned int width[6];
    unsigned int height[6];
    unsigned char rgba[6][4];
} TestUpload;

static unsigned int test_upload(void *user, const unsigned char *rgba,
                                unsigned int width, unsigned int height)
{
    TestUpload *u;
    unsigned int i;
    u = (TestUpload *)user;
    if (!u || !rgba || width == 0U || height == 0U || u->count >= 6U)
        return 0U;
    i = u->count;
    u->width[i] = width;
    u->height[i] = height;
    u->rgba[i][0] = rgba[0];
    u->rgba[i][1] = rgba[1];
    u->rgba[i][2] = rgba[2];
    u->rgba[i][3] = rgba[3];
    ++u->count;
    return i + 1U;
}

static void test_destroy(void *user, unsigned int token)
{
    (void)user;
    (void)token;
}

static int check_face(Blank3DImageAssets *images,
                      Blank3DSkybox89 *sky,
                      int face,
                      int expected_format)
{
    int id;
    const Blank3DImageAsset *asset;
    id = blank3d_skybox89_face_image_id(sky, face);
    if (id <= 0) return 0;
    if (!blank3d_image_assets_load(images, id)) {
        fprintf(stderr, "face %d load failed: %s\n", face,
                blank3d_image_assets_error(images));
        return 0;
    }
    asset = blank3d_image_assets_get(images, id);
    if (!asset || !asset->loaded) return 0;
    if (asset->width != 16U || asset->height != 16U) return 0;
    if (asset->format != expected_format) return 0;
    return 1;
}

int main(void)
{
    AssetRoute89 routes;
    AR89_FileProvider files;
    Blank3DImageAssets images;
    Blank3DImageBackend backend;
    Blank3DSkybox89 sky;
    TestUpload upload;
    int face;
    int conv;

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
    blank3d_skybox89_set_enabled(&sky, 1);
    blank3d_skybox89_set_layers(&sky, 1, 1, 0);
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_POS_X, "sky_px")) return 12;
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_NEG_X, "sky_nx")) return 13;
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_POS_Y, "sky_py")) return 14;
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_NEG_Y, "sky_ny")) return 15;
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_POS_Z, "sky_pz")) return 16;
    if (!blank3d_skybox89_set_face_request(&sky, GSKYBOX89_FACE_NEG_Z, "sky_nz")) return 17;
    if (blank3d_skybox89_resolve_faces(&sky) != GSKYBOX89_CUBE_ALL_FACES) return 18;

    if (!check_face(&images, &sky, GSKYBOX89_FACE_POS_X, IMGCC0_FMT_PNG)) return 20;
    if (!check_face(&images, &sky, GSKYBOX89_FACE_NEG_X, IMGCC0_FMT_JPEG)) return 21;
    if (!check_face(&images, &sky, GSKYBOX89_FACE_POS_Y, IMGCC0_FMT_BMP)) return 22;
    if (!check_face(&images, &sky, GSKYBOX89_FACE_NEG_Y, IMGCC0_FMT_TGA)) return 23;
    if (!check_face(&images, &sky, GSKYBOX89_FACE_POS_Z, IMGCC0_FMT_PCX)) return 24;
    if (!check_face(&images, &sky, GSKYBOX89_FACE_NEG_Z, IMGCC0_FMT_TIFF)) return 25;
    if (upload.count != 6U) return 26;

    for (face = 0; face < 6; ++face) {
        if (upload.width[face] != 16U || upload.height[face] != 16U) return 27;
    }

    conv = GSKYBOX89_ASSET_CONV_UNKNOWN;
    if (gskybox89_asset_face_from_name("Sky_Night01FT.png", &face, &conv) !=
        GSKYBOX89_ASSET_OK || face != GSKYBOX89_FACE_POS_Z ||
        conv != GSKYBOX89_ASSET_CONV_SOURCE6) return 28;
    if (!blank3d_skybox89_add_face_path(&sky,
            "tests/fixtures/skybox_imgcc0/sky_px.png")) return 29;

    blank3d_image_assets_shutdown(&images);
    puts("PASS: gskybox89 -> AssetRoute89 -> imgcc0 six-face decode");
    return 0;
}
