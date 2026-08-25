#include "blank3d_skybox89.h"

#include <string.h>

static void b3d_skybox_copy(char *dst, unsigned int cap, const char *src)
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

static void b3d_skybox_full_uv(B3DSkyboxUVRect89 *rect)
{
    if (!rect) return;
    rect->u0_q16 = 0L;
    rect->v0_q16 = 0L;
    rect->u1_q16 = B3D_SKYBOX89_UV_ONE;
    rect->v1_q16 = B3D_SKYBOX89_UV_ONE;
}

static void b3d_skybox_sync_layers(Blank3DSkybox89 *sky)
{
    int mask;
    if (!sky) return;
    mask = 0;
    if (sky->screen_enabled) mask |= GSKYBOX89_LAYER_SCREEN;
    if (sky->cube_enabled && sky->resolved_face_mask)
        mask |= GSKYBOX89_LAYER_CUBE6;
    if (sky->dome_enabled) mask |= GSKYBOX89_LAYER_DOME;
    sky->core_config.layer_mask = mask;
    sky->core_config.cube_face_mask = sky->resolved_face_mask;
    sky->core.cfg = sky->core_config;
}

void blank3d_skybox89_init(Blank3DSkybox89 *sky,
                           Blank3DImageAssets *images)
{
    Gskybox89_Backend backend;
    int i;
    if (!sky) return;
    memset(sky, 0, sizeof(*sky));
    sky->images = images;
    sky->enabled = 0;
    sky->screen_enabled = 1;
    sky->cube_enabled = 1;
    sky->dome_enabled = 0;
    sky->radius_q12 = 80L * G3D_FIX_ONE;
    sky->current_layer = 0;
    sky->current_face = GSKYBOX89_FACE_NONE;
    sky->uv_inset_pixels = 0;
    for (i = 0; i < B3D_SKYBOX89_FACE_COUNT; ++i)
        b3d_skybox_full_uv(&sky->face_uv[i]);
    b3d_skybox_full_uv(&sky->current_uv);
    gskybox89_default_config(&sky->core_config);
    sky->core_config.layer_mask = GSKYBOX89_LAYER_SCREEN;
    sky->core_config.render_after_opaque = 1;
    sky->core_config.depth_write = 0;
    sky->core_config.depth_func = GSKYBOX89_DEPTH_LEQUAL;
    sky->core_config.screen_depth_func = GSKYBOX89_DEPTH_LEQUAL;
    sky->core_config.cull_mode = GSKYBOX89_CULL_FRONT;
    memset(&backend, 0, sizeof(backend));
    gskybox89_init(&sky->core, &sky->core_config, &backend);
}

void blank3d_skybox89_set_enabled(Blank3DSkybox89 *sky, int enabled)
{
    if (!sky) return;
    sky->enabled = enabled ? 1 : 0;
}

void blank3d_skybox89_set_layers(Blank3DSkybox89 *sky,
                                 int screen_enabled,
                                 int cube_enabled,
                                 int dome_enabled)
{
    if (!sky) return;
    sky->screen_enabled = screen_enabled ? 1 : 0;
    sky->cube_enabled = cube_enabled ? 1 : 0;
    sky->dome_enabled = dome_enabled ? 1 : 0;
    b3d_skybox_sync_layers(sky);
}

void blank3d_skybox89_set_radius(Blank3DSkybox89 *sky, g3d_fix radius_q12)
{
    if (!sky) return;
    if (radius_q12 < 2L * G3D_FIX_ONE) radius_q12 = 2L * G3D_FIX_ONE;
    if (radius_q12 > 120L * G3D_FIX_ONE) radius_q12 = 120L * G3D_FIX_ONE;
    sky->radius_q12 = radius_q12;
}

int blank3d_skybox89_set_face_request(Blank3DSkybox89 *sky,
                                      int face,
                                      const char *request)
{
    if (!sky || face < 0 || face >= B3D_SKYBOX89_FACE_COUNT) return 0;
    b3d_skybox_copy(sky->face_request[face], B3D_SKYBOX89_REQUEST_CAP,
                    request ? request : "");
    sky->face_image_id[face] = 0;
    sky->resolved_face_mask &= ~(1 << face);
    b3d_skybox_sync_layers(sky);
    return 1;
}

int blank3d_skybox89_set_face_uv_q16(Blank3DSkybox89 *sky,
                                     int face,
                                     long u0_q16,
                                     long v0_q16,
                                     long u1_q16,
                                     long v1_q16)
{
    B3DSkyboxUVRect89 *rect;
    if (!sky || face < 0 || face >= B3D_SKYBOX89_FACE_COUNT) return 0;
    if (u0_q16 < 0L) u0_q16 = 0L;
    if (v0_q16 < 0L) v0_q16 = 0L;
    if (u1_q16 > B3D_SKYBOX89_UV_ONE) u1_q16 = B3D_SKYBOX89_UV_ONE;
    if (v1_q16 > B3D_SKYBOX89_UV_ONE) v1_q16 = B3D_SKYBOX89_UV_ONE;
    if (u1_q16 <= u0_q16 || v1_q16 <= v0_q16) return 0;
    rect = &sky->face_uv[face];
    rect->u0_q16 = u0_q16;
    rect->v0_q16 = v0_q16;
    rect->u1_q16 = u1_q16;
    rect->v1_q16 = v1_q16;
    return 1;
}

void blank3d_skybox89_set_uv_inset_pixels(Blank3DSkybox89 *sky, int pixels)
{
    if (!sky) return;
    if (pixels < 0) pixels = 0;
    if (pixels > 8) pixels = 8;
    sky->uv_inset_pixels = pixels;
}

int blank3d_skybox89_set_screen_request(Blank3DSkybox89 *sky,
                                        const char *request)
{
    if (!sky) return 0;
    b3d_skybox_copy(sky->screen_request, B3D_SKYBOX89_REQUEST_CAP,
                    request ? request : "");
    sky->screen_image_id = 0;
    return 1;
}

int blank3d_skybox89_set_dome_request(Blank3DSkybox89 *sky,
                                      const char *request)
{
    if (!sky) return 0;
    b3d_skybox_copy(sky->dome_request, B3D_SKYBOX89_REQUEST_CAP,
                    request ? request : "");
    sky->dome_image_id = 0;
    return 1;
}

int blank3d_skybox89_add_face_path(Blank3DSkybox89 *sky,
                                   const char *path)
{
    int face;
    int convention;
    if (!sky || !path || !path[0]) return 0;
    face = GSKYBOX89_FACE_NONE;
    convention = GSKYBOX89_ASSET_CONV_UNKNOWN;
    if (gskybox89_asset_face_from_name(path, &face, &convention) !=
        GSKYBOX89_ASSET_OK)
        return 0;
    (void)convention;
    return blank3d_skybox89_set_face_request(sky, face, path);
}

int blank3d_skybox89_resolve_assets(Blank3DSkybox89 *sky)
{
    int face;
    int id;
    int mask;
    if (!sky || !sky->images) return 0;
    mask = 0;
    for (face = 0; face < B3D_SKYBOX89_FACE_COUNT; ++face) {
        sky->face_image_id[face] = 0;
        if (!sky->face_request[face][0]) continue;
        id = blank3d_image_assets_resolve_request(sky->images,
                                                   sky->face_request[face]);
        if (id <= 0) continue;
        sky->face_image_id[face] = id;
        mask |= (1 << face);
    }
    sky->screen_image_id = 0;
    if (sky->screen_request[0])
        sky->screen_image_id = blank3d_image_assets_resolve_request(
            sky->images, sky->screen_request);
    sky->dome_image_id = 0;
    if (sky->dome_request[0])
        sky->dome_image_id = blank3d_image_assets_resolve_request(
            sky->images, sky->dome_request);
    sky->resolved_face_mask = mask;
    b3d_skybox_sync_layers(sky);
    return mask;
}

int blank3d_skybox89_resolve_faces(Blank3DSkybox89 *sky)
{
    return blank3d_skybox89_resolve_assets(sky);
}

int blank3d_skybox89_face_image_id(const Blank3DSkybox89 *sky, int face)
{
    if (!sky || face < 0 || face >= B3D_SKYBOX89_FACE_COUNT) return 0;
    return sky->face_image_id[face];
}

int blank3d_skybox89_screen_image_id(const Blank3DSkybox89 *sky)
{
    return sky ? sky->screen_image_id : 0;
}

int blank3d_skybox89_dome_image_id(const Blank3DSkybox89 *sky)
{
    return sky ? sky->dome_image_id : 0;
}

int blank3d_skybox89_resolved_mask(const Blank3DSkybox89 *sky)
{
    return sky ? sky->resolved_face_mask : 0;
}
