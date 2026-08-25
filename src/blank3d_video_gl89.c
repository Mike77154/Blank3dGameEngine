#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <string.h>

#include "blank3d_video_gl89.h"

#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT 0x8D40
#endif
#ifndef GL_RENDERBUFFER_EXT
#define GL_RENDERBUFFER_EXT 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0_EXT
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT_EXT
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE_EXT
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

typedef void (APIENTRY *B3DGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint *);
typedef void (APIENTRY *B3DGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *B3DGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (APIENTRY *B3DGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void (APIENTRY *B3DGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint *);
typedef void (APIENTRY *B3DGLGENRENDERBUFFERSPROC)(GLsizei, GLuint *);
typedef void (APIENTRY *B3DGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *B3DGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *B3DGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRY *B3DGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint *);
typedef int (APIENTRY *B3DWGLSWAPINTERVALEXTPROC)(int);

static B3DGLGENFRAMEBUFFERSPROC b3d_glGenFramebuffers;
static B3DGLBINDFRAMEBUFFERPROC b3d_glBindFramebuffer;
static B3DGLFRAMEBUFFERTEXTURE2DPROC b3d_glFramebufferTexture2D;
static B3DGLCHECKFRAMEBUFFERSTATUSPROC b3d_glCheckFramebufferStatus;
static B3DGLDELETEFRAMEBUFFERSPROC b3d_glDeleteFramebuffers;
static B3DGLGENRENDERBUFFERSPROC b3d_glGenRenderbuffers;
static B3DGLBINDRENDERBUFFERPROC b3d_glBindRenderbuffer;
static B3DGLRENDERBUFFERSTORAGEPROC b3d_glRenderbufferStorage;
static B3DGLFRAMEBUFFERRENDERBUFFERPROC b3d_glFramebufferRenderbuffer;
static B3DGLDELETERENDERBUFFERSPROC b3d_glDeleteRenderbuffers;
static B3DWGLSWAPINTERVALEXTPROC b3d_wglSwapIntervalEXT;

static void b3d_vgl_copy(char *dst, unsigned int cap, const char *src)
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

static void b3d_vgl_status(Blank3DVideoGL89 *video, const char *text)
{
    if (!video) return;
    b3d_vgl_copy(video->status, sizeof(video->status), text);
}

static PROC b3d_gl_proc(const char *core_name, const char *ext_name)
{
    PROC proc;
    proc = wglGetProcAddress(core_name);
    if (!proc && ext_name) proc = wglGetProcAddress(ext_name);
    return proc;
}

static void b3d_vgl_load_extensions(void)
{
    b3d_glGenFramebuffers = (B3DGLGENFRAMEBUFFERSPROC)
        b3d_gl_proc("glGenFramebuffers", "glGenFramebuffersEXT");
    b3d_glBindFramebuffer = (B3DGLBINDFRAMEBUFFERPROC)
        b3d_gl_proc("glBindFramebuffer", "glBindFramebufferEXT");
    b3d_glFramebufferTexture2D = (B3DGLFRAMEBUFFERTEXTURE2DPROC)
        b3d_gl_proc("glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    b3d_glCheckFramebufferStatus = (B3DGLCHECKFRAMEBUFFERSTATUSPROC)
        b3d_gl_proc("glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    b3d_glDeleteFramebuffers = (B3DGLDELETEFRAMEBUFFERSPROC)
        b3d_gl_proc("glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    b3d_glGenRenderbuffers = (B3DGLGENRENDERBUFFERSPROC)
        b3d_gl_proc("glGenRenderbuffers", "glGenRenderbuffersEXT");
    b3d_glBindRenderbuffer = (B3DGLBINDRENDERBUFFERPROC)
        b3d_gl_proc("glBindRenderbuffer", "glBindRenderbufferEXT");
    b3d_glRenderbufferStorage = (B3DGLRENDERBUFFERSTORAGEPROC)
        b3d_gl_proc("glRenderbufferStorage", "glRenderbufferStorageEXT");
    b3d_glFramebufferRenderbuffer = (B3DGLFRAMEBUFFERRENDERBUFFERPROC)
        b3d_gl_proc("glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");
    b3d_glDeleteRenderbuffers = (B3DGLDELETERENDERBUFFERSPROC)
        b3d_gl_proc("glDeleteRenderbuffers", "glDeleteRenderbuffersEXT");
    b3d_wglSwapIntervalEXT = (B3DWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
}

static int b3d_vgl_has_fbo(void)
{
    return b3d_glGenFramebuffers && b3d_glBindFramebuffer &&
           b3d_glFramebufferTexture2D && b3d_glCheckFramebufferStatus &&
           b3d_glDeleteFramebuffers && b3d_glGenRenderbuffers &&
           b3d_glBindRenderbuffer && b3d_glRenderbufferStorage &&
           b3d_glFramebufferRenderbuffer && b3d_glDeleteRenderbuffers;
}

static void b3d_vgl_release_target(Blank3DVideoGL89 *video)
{
    if (!video || !video->context_ready) return;
    if (video->depth_renderbuffer && b3d_glDeleteRenderbuffers)
        b3d_glDeleteRenderbuffers(1, &video->depth_renderbuffer);
    if (video->framebuffer && b3d_glDeleteFramebuffers)
        b3d_glDeleteFramebuffers(1, &video->framebuffer);
    if (video->color_texture) glDeleteTextures(1, &video->color_texture);
    video->depth_renderbuffer = 0U;
    video->framebuffer = 0U;
    video->color_texture = 0U;
    video->texture_width = 0;
    video->texture_height = 0;
    video->using_fbo = 0;
}

static int b3d_next_pot(int value)
{
    int result;
    result = 1;
    while (result < value && result < 16384) result <<= 1;
    return result;
}

static void b3d_vgl_texture_params(const Blank3DVideoGL89 *video)
{
    GLint filter;
    filter = video->config.filter_mode == GVC89_FILTER_LINEAR
           ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

static int b3d_vgl_create_target(Blank3DVideoGL89 *video)
{
    int rw;
    int rh;
    GLenum status;
    if (!video || !video->context_ready) return 0;
    rw = video->config.resolution_width;
    rh = video->config.resolution_height;
    b3d_vgl_release_target(video);
    if (b3d_vgl_has_fbo()) {
        glGenTextures(1, &video->color_texture);
        glBindTexture(GL_TEXTURE_2D, video->color_texture);
        b3d_vgl_texture_params(video);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rw, rh, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, 0);
        b3d_glGenFramebuffers(1, &video->framebuffer);
        b3d_glBindFramebuffer(GL_FRAMEBUFFER_EXT, video->framebuffer);
        b3d_glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                                   GL_COLOR_ATTACHMENT0_EXT,
                                   GL_TEXTURE_2D,
                                   video->color_texture, 0);
        b3d_glGenRenderbuffers(1, &video->depth_renderbuffer);
        b3d_glBindRenderbuffer(GL_RENDERBUFFER_EXT,
                               video->depth_renderbuffer);
        b3d_glRenderbufferStorage(GL_RENDERBUFFER_EXT,
                                  GL_DEPTH_COMPONENT24, rw, rh);
        b3d_glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT,
                                      GL_DEPTH_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT,
                                      video->depth_renderbuffer);
        status = b3d_glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
        b3d_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0U);
        if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
            video->using_fbo = 1;
            video->texture_width = rw;
            video->texture_height = rh;
            video->target_dirty = 0;
            b3d_vgl_status(video, "FBO render resolution target ready");
            return 1;
        }
        b3d_vgl_release_target(video);
    }
    video->texture_width = b3d_next_pot(rw);
    video->texture_height = b3d_next_pot(rh);
    glGenTextures(1, &video->color_texture);
    glBindTexture(GL_TEXTURE_2D, video->color_texture);
    b3d_vgl_texture_params(video);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 video->texture_width, video->texture_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    video->using_fbo = 0;
    video->target_dirty = 0;
    b3d_vgl_status(video, "OpenGL 1.1 copy-scale fallback ready");
    return video->color_texture != 0U;
}

void blank3d_video_gl89_init(Blank3DVideoGL89 *video)
{
    if (!video) return;
    memset(video, 0, sizeof(*video));
    video->config.resolution_width = 960;
    video->config.resolution_height = 540;
    video->config.filter_mode = GVC89_FILTER_NEAREST;
    video->presentation.width = 960;
    video->presentation.height = 540;
    video->output_width = 960;
    video->output_height = 540;
    video->target_dirty = 1;
    b3d_vgl_status(video, "video GL bridge initialized");
}

int blank3d_video_gl89_configure(Blank3DVideoGL89 *video,
                                    const gvc89_config *config,
                                    const gvc89_rect *presentation,
                                    int output_width, int output_height)
{
    int resolution_changed;
    if (!video || !config || !presentation ||
        output_width <= 0 || output_height <= 0) return 0;
    resolution_changed = video->config.resolution_width != config->resolution_width ||
                         video->config.resolution_height != config->resolution_height;
    video->config = *config;
    video->presentation = *presentation;
    video->output_width = output_width;
    video->output_height = output_height;
    video->configured = 1;
    if (resolution_changed) video->target_dirty = 1;
    if (video->context_ready && b3d_wglSwapIntervalEXT)
        (void)b3d_wglSwapIntervalEXT(config->vsync ? 1 : 0);
    return 1;
}

static int b3d_vgl_apply(void *user, const gvc89_config *config,
                         const gvc89_rect *presentation,
                         int output_width, int output_height)
{
    return blank3d_video_gl89_configure((Blank3DVideoGL89 *)user,
                                         config, presentation,
                                         output_width, output_height);
}

void blank3d_video_gl89_make_provider(Blank3DVideoGL89 *video,
                                      gvc89_provider *out_provider)
{
    if (!out_provider) return;
    memset(out_provider, 0, sizeof(*out_provider));
    out_provider->user = video;
    out_provider->apply_video = b3d_vgl_apply;
}

int blank3d_video_gl89_create_context(Blank3DVideoGL89 *video, HDC device)
{
    PIXELFORMATDESCRIPTOR descriptor;
    int pixel_format;
    if (!video || !device) return 0;
    video->device = device;
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.iLayerType = PFD_MAIN_PLANE;
    pixel_format = ChoosePixelFormat(device, &descriptor);
    if (!pixel_format) return 0;
    if (!SetPixelFormat(device, pixel_format, &descriptor)) return 0;
    video->context = wglCreateContext(device);
    if (!video->context) return 0;
    if (!wglMakeCurrent(device, video->context)) return 0;
    video->context_ready = 1;
    b3d_vgl_load_extensions();
    if (b3d_wglSwapIntervalEXT)
        (void)b3d_wglSwapIntervalEXT(video->config.vsync ? 1 : 0);
    video->target_dirty = 1;
    return b3d_vgl_create_target(video);
}

void blank3d_video_gl89_destroy(Blank3DVideoGL89 *video)
{
    if (!video) return;
    if (video->context_ready) {
        b3d_vgl_release_target(video);
        (void)wglMakeCurrent(0, 0);
        if (video->context) (void)wglDeleteContext(video->context);
    }
    video->context = 0;
    video->device = 0;
    video->context_ready = 0;
    b3d_vgl_status(video, "video GL bridge destroyed");
}

int blank3d_video_gl89_begin_render(Blank3DVideoGL89 *video)
{
    if (!video || !video->context_ready) return 0;
    if (video->target_dirty && !b3d_vgl_create_target(video)) return 0;
    if (video->using_fbo)
        b3d_glBindFramebuffer(GL_FRAMEBUFFER_EXT, video->framebuffer);
    else if (b3d_glBindFramebuffer)
        b3d_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0U);
    return 1;
}

static void b3d_vgl_draw_output(Blank3DVideoGL89 *video)
{
    GLfloat u;
    GLfloat v;
    int x0;
    int y0;
    int x1;
    int y1;
    GLint filter;
    u = (GLfloat)video->config.resolution_width /
        (GLfloat)video->texture_width;
    v = (GLfloat)video->config.resolution_height /
        (GLfloat)video->texture_height;
    x0 = video->presentation.x;
    y0 = video->presentation.y;
    x1 = x0 + video->presentation.width;
    y1 = y0 + video->presentation.height;
    filter = video->config.filter_mode == GVC89_FILTER_LINEAR
           ? GL_LINEAR : GL_NEAREST;
    glViewport(0, 0, video->output_width, video->output_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, video->color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)video->output_width,
            0.0, (GLdouble)video->output_height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4ub(255U, 255U, 255U, 255U);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2i(x0, y0);
    glTexCoord2f(u, 0.0f); glVertex2i(x1, y0);
    glTexCoord2f(u, v); glVertex2i(x1, y1);
    glTexCoord2f(0.0f, v); glVertex2i(x0, y1);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
}

int blank3d_video_gl89_present(Blank3DVideoGL89 *video)
{
    if (!video || !video->context_ready || !video->color_texture) return 0;
    if (video->using_fbo) {
        b3d_glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0U);
    } else {
        if (video->config.resolution_width > video->output_width ||
            video->config.resolution_height > video->output_height) {
            b3d_vgl_status(video, "copy-scale fallback cannot supersample; use FBO-capable OpenGL");
            return 0;
        }
        glBindTexture(GL_TEXTURE_2D, video->color_texture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                            video->config.resolution_width,
                            video->config.resolution_height);
    }
    b3d_vgl_draw_output(video);
    return 1;
}

const char *blank3d_video_gl89_status(const Blank3DVideoGL89 *video)
{
    return video ? video->status : "video GL unavailable";
}
