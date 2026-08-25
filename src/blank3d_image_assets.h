#ifndef BLANK3D_IMAGE_ASSETS_H
#define BLANK3D_IMAGE_ASSETS_H

#include "imgcc0.h"
#include "assetroute89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_IMAGE_ASSET_MAX 128
#define B3D_IMAGE_PATH_MAX AR89_PATH_CAP
#define B3D_IMAGE_DYNAMIC_ID_FIRST 20000
#define B3D_IMAGE_DYNAMIC_ID_LAST 32000

typedef unsigned int (*Blank3DImageUploadFn)(void *user,
                                              const unsigned char *rgba,
                                              unsigned int width,
                                              unsigned int height);
typedef void (*Blank3DImageDestroyFn)(void *user, unsigned int texture_token);

typedef struct Blank3DImageBackendTag {
    void *user;
    Blank3DImageUploadFn upload_rgba;
    Blank3DImageDestroyFn destroy_texture;
} Blank3DImageBackend;

typedef struct Blank3DImageAssetTag {
    int used;
    int id;
    int loaded;
    unsigned int width;
    unsigned int height;
    int format;
    unsigned int texture_token;
    char path[B3D_IMAGE_PATH_MAX];
} Blank3DImageAsset;

typedef struct Blank3DImageAssetsTag {
    Blank3DImageAsset assets[B3D_IMAGE_ASSET_MAX];
    int count;
    int next_dynamic_id;
    Blank3DImageBackend backend;
    AssetRoute89 *routes;
    char last_error[160];
} Blank3DImageAssets;

void blank3d_image_assets_init(Blank3DImageAssets *assets);
void blank3d_image_assets_set_backend(Blank3DImageAssets *assets,
                                      const Blank3DImageBackend *backend);
void blank3d_image_assets_set_routes(Blank3DImageAssets *assets,
                                     AssetRoute89 *routes);
void blank3d_image_assets_shutdown(Blank3DImageAssets *assets);
int blank3d_image_assets_register(Blank3DImageAssets *assets,
                                  int id, const char *path);
int blank3d_image_assets_resolve_path(Blank3DImageAssets *assets,
                                      const char *path);
int blank3d_image_assets_resolve_request(Blank3DImageAssets *assets,
                                         const char *request);
int blank3d_image_assets_load(Blank3DImageAssets *assets, int id);
const Blank3DImageAsset *blank3d_image_assets_get(const Blank3DImageAssets *assets,
                                                  int id);
Blank3DImageAsset *blank3d_image_assets_get_mut(Blank3DImageAssets *assets,
                                                int id);
const char *blank3d_image_assets_error(const Blank3DImageAssets *assets);

#ifdef __cplusplus
}
#endif

#endif
