#ifndef BLANK3D_VIDEO_GL89_H
#define BLANK3D_VIDEO_GL89_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include "generalvideoconfigc89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DVideoGL89Tag {
    HDC device;
    HGLRC context;
    gvc89_config config;
    gvc89_rect presentation;
    int output_width;
    int output_height;
    int configured;
    int context_ready;
    int target_dirty;
    int using_fbo;
    GLuint color_texture;
    GLuint framebuffer;
    GLuint depth_renderbuffer;
    int texture_width;
    int texture_height;
    char status[128];
} Blank3DVideoGL89;

void blank3d_video_gl89_init(Blank3DVideoGL89 *video);
int blank3d_video_gl89_configure(Blank3DVideoGL89 *video,
                                    const gvc89_config *config,
                                    const gvc89_rect *presentation,
                                    int output_width, int output_height);
void blank3d_video_gl89_make_provider(Blank3DVideoGL89 *video,
                                      gvc89_provider *out_provider);
int blank3d_video_gl89_create_context(Blank3DVideoGL89 *video, HDC device);
void blank3d_video_gl89_destroy(Blank3DVideoGL89 *video);
int blank3d_video_gl89_begin_render(Blank3DVideoGL89 *video);
int blank3d_video_gl89_present(Blank3DVideoGL89 *video);
const char *blank3d_video_gl89_status(const Blank3DVideoGL89 *video);

#ifdef __cplusplus
}
#endif
#endif
