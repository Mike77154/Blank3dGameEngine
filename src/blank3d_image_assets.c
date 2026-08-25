#include "blank3d_image_assets.h"

#include <stdio.h>
#include <string.h>

#define B3D_IMAGE_OUTPUT_BYTES (6U * 1024U * 1024U)
#define B3D_IMAGE_TEMP_BYTES   (6U * 1024U * 1024U)
#define B3D_IMAGE_FILE_BYTES   (4U * 1024U * 1024U)
#define B3D_IMAGE_MAX_DIM      1024U

IMGCC0_DECLARE_BUFFER(b3d_image_output, B3D_IMAGE_OUTPUT_BYTES);
IMGCC0_DECLARE_BUFFER(b3d_image_temp, B3D_IMAGE_TEMP_BYTES);
IMGCC0_DECLARE_BUFFER(b3d_image_file, B3D_IMAGE_FILE_BYTES);

static void b3d_image_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_image_error(Blank3DImageAssets *assets, const char *text)
{
    if (!assets) return;
    b3d_image_copy(assets->last_error,
                   (unsigned int)sizeof(assets->last_error), text);
}

void blank3d_image_assets_init(Blank3DImageAssets *assets)
{
    if (!assets) return;
    memset(assets, 0, sizeof(*assets));
    assets->next_dynamic_id = B3D_IMAGE_DYNAMIC_ID_FIRST;
}

void blank3d_image_assets_set_backend(Blank3DImageAssets *assets,
                                      const Blank3DImageBackend *backend)
{
    if (!assets) return;
    if (backend) assets->backend = *backend;
    else memset(&assets->backend, 0, sizeof(assets->backend));
}

void blank3d_image_assets_set_routes(Blank3DImageAssets *assets,
                                     AssetRoute89 *routes)
{
    if (!assets) return;
    assets->routes = routes;
}

Blank3DImageAsset *blank3d_image_assets_get_mut(Blank3DImageAssets *assets,
                                                int id)
{
    int i;
    if (!assets) return 0;
    for (i = 0; i < B3D_IMAGE_ASSET_MAX; ++i)
        if (assets->assets[i].used && assets->assets[i].id == id)
            return &assets->assets[i];
    return 0;
}

const Blank3DImageAsset *blank3d_image_assets_get(const Blank3DImageAssets *assets,
                                                  int id)
{
    int i;
    if (!assets) return 0;
    for (i = 0; i < B3D_IMAGE_ASSET_MAX; ++i)
        if (assets->assets[i].used && assets->assets[i].id == id)
            return &assets->assets[i];
    return 0;
}

int blank3d_image_assets_register(Blank3DImageAssets *assets,
                                  int id, const char *path)
{
    Blank3DImageAsset *slot;
    int i;
    if (!assets || !path || !path[0] || id <= 0 || id > 65535) return 0;
    slot = blank3d_image_assets_get_mut(assets, id);
    if (slot) {
        if (slot->loaded && strcmp(slot->path, path) != 0 &&
            assets->backend.destroy_texture && slot->texture_token)
            assets->backend.destroy_texture(assets->backend.user,
                                             slot->texture_token);
        slot->loaded = 0;
        slot->texture_token = 0U;
        slot->width = slot->height = 0U;
        b3d_image_copy(slot->path, B3D_IMAGE_PATH_MAX, path);
        return 1;
    }
    for (i = 0; i < B3D_IMAGE_ASSET_MAX; ++i) {
        if (assets->assets[i].used) continue;
        slot = &assets->assets[i];
        memset(slot, 0, sizeof(*slot));
        slot->used = 1;
        slot->id = id;
        b3d_image_copy(slot->path, B3D_IMAGE_PATH_MAX, path);
        ++assets->count;
        return 1;
    }
    b3d_image_error(assets, "image asset registry full");
    return 0;
}

int blank3d_image_assets_resolve_request(Blank3DImageAssets *assets,
                                         const char *request)
{
    int i;
    int id;
    char routed[AR89_PATH_CAP];
    const char *path;
    if (!assets || !request || !request[0]) return 0;
    path = request;
    if (assets->routes &&
        ar89_resolve_request(assets->routes, AR89_KIND_IMAGE, request,
                             routed, (ar89_u32)sizeof(routed)))
        path = routed;
    for (i = 0; i < B3D_IMAGE_ASSET_MAX; ++i) {
        if (assets->assets[i].used && !strcmp(assets->assets[i].path, path))
            return assets->assets[i].id;
    }
    id = assets->next_dynamic_id;
    while (id <= B3D_IMAGE_DYNAMIC_ID_LAST &&
           blank3d_image_assets_get(assets, id)) ++id;
    if (id > B3D_IMAGE_DYNAMIC_ID_LAST) {
        b3d_image_error(assets, "dynamic image id range exhausted");
        return 0;
    }
    if (!blank3d_image_assets_register(assets, id, path)) return 0;
    assets->next_dynamic_id = id + 1;
    return id;
}

int blank3d_image_assets_resolve_path(Blank3DImageAssets *assets,
                                      const char *path)
{
    return blank3d_image_assets_resolve_request(assets, path);
}

int blank3d_image_assets_load(Blank3DImageAssets *assets, int id)
{
    Blank3DImageAsset *slot;
    imgcc0_open_options opt;
    imgcc0_image image;
    int rc;
    if (!assets) return 0;
    slot = blank3d_image_assets_get_mut(assets, id);
    if (!slot) {
        b3d_image_error(assets, "unknown image asset id");
        return 0;
    }
    if (slot->loaded) return 1;
    if (!assets->backend.upload_rgba) {
        b3d_image_error(assets, "image backend has no RGBA uploader");
        return 0;
    }
    imgcc0_open_options_init(&opt);
    opt.output_buffer = IMGCC0_BUFFER_DATA(b3d_image_output);
    opt.output_buffer_size = IMGCC0_BUFFER_SIZE(b3d_image_output);
    opt.temp_buffer = IMGCC0_BUFFER_DATA(b3d_image_temp);
    opt.temp_buffer_size = IMGCC0_BUFFER_SIZE(b3d_image_temp);
    opt.file_buffer = IMGCC0_BUFFER_DATA(b3d_image_file);
    opt.file_buffer_size = IMGCC0_BUFFER_SIZE(b3d_image_file);
    opt.max_width = B3D_IMAGE_MAX_DIM;
    opt.max_height = B3D_IMAGE_MAX_DIM;
    opt.strict = 0;
    imgcc0_image_init(&image);
    rc = imgcc0_open_file(slot->path, &opt, &image);
    if (rc != IMGCC0_OK || !image.ok || !image.frames || image.frame_count < 1U ||
        !image.frames[0].pixels) {
        b3d_image_error(assets, image.error_message[0]
                         ? image.error_message : imgcc0_error_string(rc));
        imgcc0_image_reset(&image);
        return 0;
    }
    slot->texture_token = assets->backend.upload_rgba(
        assets->backend.user, image.frames[0].pixels,
        image.frames[0].width, image.frames[0].height);
    if (!slot->texture_token) {
        b3d_image_error(assets, "renderer rejected decoded RGBA image");
        imgcc0_image_reset(&image);
        return 0;
    }
    slot->width = image.frames[0].width;
    slot->height = image.frames[0].height;
    slot->format = (int)image.format;
    slot->loaded = 1;
    imgcc0_image_reset(&image);
    assets->last_error[0] = '\0';
    return 1;
}

void blank3d_image_assets_shutdown(Blank3DImageAssets *assets)
{
    int i;
    if (!assets) return;
    if (assets->backend.destroy_texture) {
        for (i = 0; i < B3D_IMAGE_ASSET_MAX; ++i) {
            Blank3DImageAsset *slot;
            slot = &assets->assets[i];
            if (slot->used && slot->loaded && slot->texture_token)
                assets->backend.destroy_texture(assets->backend.user,
                                                 slot->texture_token);
        }
    }
    memset(assets, 0, sizeof(*assets));
}

const char *blank3d_image_assets_error(const Blank3DImageAssets *assets)
{
    return assets ? assets->last_error : "image assets unavailable";
}
