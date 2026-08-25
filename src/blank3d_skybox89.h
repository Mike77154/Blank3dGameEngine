#ifndef BLANK3D_SKYBOX89_H
#define BLANK3D_SKYBOX89_H

#include "blank3d_image_assets.h"
#include "gskybox89.h"
#include "gskybox89_assets.h"
#include "gamlib3d_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_SKYBOX89_FACE_COUNT GSKYBOX89_CUBE_FACE_COUNT
#define B3D_SKYBOX89_REQUEST_CAP B3D_IMAGE_PATH_MAX
#define B3D_SKYBOX89_UV_ONE 65536L

typedef struct B3DSkyboxUVRect89Tag {
    long u0_q16;
    long v0_q16;
    long u1_q16;
    long v1_q16;
} B3DSkyboxUVRect89;

typedef struct Blank3DSkybox89Tag {
    Gskybox89_Context core;
    Gskybox89_Config core_config;
    Blank3DImageAssets *images;
    int enabled;
    int screen_enabled;
    int cube_enabled;
    int dome_enabled;
    g3d_fix radius_q12;
    char face_request[B3D_SKYBOX89_FACE_COUNT][B3D_SKYBOX89_REQUEST_CAP];
    int face_image_id[B3D_SKYBOX89_FACE_COUNT];
    B3DSkyboxUVRect89 face_uv[B3D_SKYBOX89_FACE_COUNT];
    int uv_inset_pixels;
    char screen_request[B3D_SKYBOX89_REQUEST_CAP];
    char dome_request[B3D_SKYBOX89_REQUEST_CAP];
    int screen_image_id;
    int dome_image_id;
    int resolved_face_mask;
    int current_layer;
    int current_face;
    int current_face_ready;
    int current_texture_ready;
    B3DSkyboxUVRect89 current_uv;
    unsigned int current_texture_width;
    unsigned int current_texture_height;
    Vec3 render_eye_q12;
} Blank3DSkybox89;

void blank3d_skybox89_init(Blank3DSkybox89 *sky,
                           Blank3DImageAssets *images);
void blank3d_skybox89_set_enabled(Blank3DSkybox89 *sky, int enabled);
void blank3d_skybox89_set_layers(Blank3DSkybox89 *sky,
                                 int screen_enabled,
                                 int cube_enabled,
                                 int dome_enabled);
void blank3d_skybox89_set_radius(Blank3DSkybox89 *sky, g3d_fix radius_q12);
int blank3d_skybox89_set_face_request(Blank3DSkybox89 *sky,
                                      int face,
                                      const char *request);
int blank3d_skybox89_set_face_uv_q16(Blank3DSkybox89 *sky,
                                     int face,
                                     long u0_q16,
                                     long v0_q16,
                                     long u1_q16,
                                     long v1_q16);
void blank3d_skybox89_set_uv_inset_pixels(Blank3DSkybox89 *sky, int pixels);
int blank3d_skybox89_set_screen_request(Blank3DSkybox89 *sky,
                                        const char *request);
int blank3d_skybox89_set_dome_request(Blank3DSkybox89 *sky,
                                      const char *request);
int blank3d_skybox89_add_face_path(Blank3DSkybox89 *sky,
                                   const char *path);
int blank3d_skybox89_resolve_faces(Blank3DSkybox89 *sky);
int blank3d_skybox89_resolve_assets(Blank3DSkybox89 *sky);
int blank3d_skybox89_face_image_id(const Blank3DSkybox89 *sky, int face);
int blank3d_skybox89_screen_image_id(const Blank3DSkybox89 *sky);
int blank3d_skybox89_dome_image_id(const Blank3DSkybox89 *sky);
int blank3d_skybox89_resolved_mask(const Blank3DSkybox89 *sky);

/* Win32/OpenGL host adapter.  The core vendor remains renderer-agnostic. */
void blank3d_skybox89_gl_attach(Blank3DSkybox89 *sky);
int blank3d_skybox89_gl_render(Blank3DSkybox89 *sky, const Vec3 *eye_q12);

#ifdef __cplusplus
}
#endif

#endif
