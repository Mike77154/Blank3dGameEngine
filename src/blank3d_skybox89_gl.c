#include "blank3d_skybox89.h"

#include <gl/gl.h>
#include <string.h>

#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif

typedef struct B3DSkyGLStateTag {
    GLboolean depth_mask;
    GLint depth_func;
    GLint texture_binding;
    GLint blend_src;
    GLint blend_dst;
    GLint cull_face_mode;
    int depth_test;
    int texture_2d;
    int blend;
    int cull_face;
    int lighting;
    int screen_matrix_pushed;
} B3DSkyGLState;

static B3DSkyGLState b3d_sky_gl_state;

static void b3d_sky_gl_save(void)
{
    memset(&b3d_sky_gl_state, 0, sizeof(b3d_sky_gl_state));
    b3d_sky_gl_state.depth_test = glIsEnabled(GL_DEPTH_TEST) ? 1 : 0;
    b3d_sky_gl_state.texture_2d = glIsEnabled(GL_TEXTURE_2D) ? 1 : 0;
    b3d_sky_gl_state.blend = glIsEnabled(GL_BLEND) ? 1 : 0;
    b3d_sky_gl_state.cull_face = glIsEnabled(GL_CULL_FACE) ? 1 : 0;
    b3d_sky_gl_state.lighting = glIsEnabled(GL_LIGHTING) ? 1 : 0;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &b3d_sky_gl_state.depth_mask);
    glGetIntegerv(GL_DEPTH_FUNC, &b3d_sky_gl_state.depth_func);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &b3d_sky_gl_state.texture_binding);
    glGetIntegerv(GL_BLEND_SRC, &b3d_sky_gl_state.blend_src);
    glGetIntegerv(GL_BLEND_DST, &b3d_sky_gl_state.blend_dst);
    glGetIntegerv(GL_CULL_FACE_MODE, &b3d_sky_gl_state.cull_face_mode);
}

static void b3d_sky_gl_restore(void)
{
    glBindTexture(GL_TEXTURE_2D, (GLuint)b3d_sky_gl_state.texture_binding);
    glDepthMask(b3d_sky_gl_state.depth_mask);
    glDepthFunc((GLenum)b3d_sky_gl_state.depth_func);
    glBlendFunc((GLenum)b3d_sky_gl_state.blend_src,
                (GLenum)b3d_sky_gl_state.blend_dst);
    glCullFace((GLenum)b3d_sky_gl_state.cull_face_mode);
    if (b3d_sky_gl_state.depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (b3d_sky_gl_state.texture_2d) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    if (b3d_sky_gl_state.blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (b3d_sky_gl_state.cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (b3d_sky_gl_state.lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    glColor4ub(255U, 255U, 255U, 255U);
}

static void b3d_sky_gl_begin_pass(void *user, int pass)
{
    (void)user;
    if (pass == GSKYBOX89_PASS_SCREEN) {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        b3d_sky_gl_state.screen_matrix_pushed = 1;
    }
}

static void b3d_sky_gl_end_pass(void *user, int pass)
{
    (void)user;
    if (pass == GSKYBOX89_PASS_SCREEN &&
        b3d_sky_gl_state.screen_matrix_pushed) {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        b3d_sky_gl_state.screen_matrix_pushed = 0;
    }
}

static void b3d_sky_gl_set_state(void *user, const Gskybox89_State *state)
{
    (void)user;
    if (!state) return;
    if (state->depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(state->depth_write ? GL_TRUE : GL_FALSE);
    if (state->depth_func == GSKYBOX89_DEPTH_ALWAYS) glDepthFunc(GL_ALWAYS);
    else if (state->depth_func == GSKYBOX89_DEPTH_EQUAL) glDepthFunc(GL_EQUAL);
    else glDepthFunc(GL_LEQUAL);
    if (state->cull_mode == GSKYBOX89_CULL_NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(state->cull_mode == GSKYBOX89_CULL_FRONT
                   ? GL_FRONT : GL_BACK);
    }
    if (state->blend_mode == GSKYBOX89_BLEND_OFF) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,
                    state->blend_mode == GSKYBOX89_BLEND_ADD
                    ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    }
    if (state->lighting_enabled) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
}

static int b3d_sky_gl_bind_image(Blank3DSkybox89 *sky, int id,
                                 int repeat_s)
{
    const Blank3DImageAsset *asset;
    if (!sky || id <= 0 || !blank3d_image_assets_load(sky->images, id))
        return 0;
    asset = blank3d_image_assets_get(sky->images, id);
    if (!asset || !asset->loaded || !asset->texture_token) return 0;
    sky->current_texture_width = asset->width;
    sky->current_texture_height = asset->height;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)asset->texture_token);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    repeat_s ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    return 1;
}

static void b3d_sky_gl_bind_face(void *user, int layer, int face)
{
    Blank3DSkybox89 *sky;
    int id;
    sky = (Blank3DSkybox89 *)user;
    if (!sky) return;
    sky->current_layer = layer;
    sky->current_face = face;
    sky->current_face_ready = 1;
    sky->current_texture_ready = 0;
    sky->current_texture_width = 0U;
    sky->current_texture_height = 0U;
    sky->current_uv.u0_q16 = 0L;
    sky->current_uv.v0_q16 = 0L;
    sky->current_uv.u1_q16 = B3D_SKYBOX89_UV_ONE;
    sky->current_uv.v1_q16 = B3D_SKYBOX89_UV_ONE;
    id = 0;

    if (layer == GSKYBOX89_LAYER_CUBE6 &&
        face >= 0 && face < B3D_SKYBOX89_FACE_COUNT) {
        id = sky->face_image_id[face];
        sky->current_uv = sky->face_uv[face];
        if (id <= 0 || !b3d_sky_gl_bind_image(sky, id, 0)) {
            sky->current_face_ready = 0;
            glBindTexture(GL_TEXTURE_2D, 0U);
            glDisable(GL_TEXTURE_2D);
            return;
        }
        sky->current_texture_ready = 1;
        return;
    }
    if (layer == GSKYBOX89_LAYER_SCREEN) id = sky->screen_image_id;
    else if (layer == GSKYBOX89_LAYER_DOME) id = sky->dome_image_id;

    if (id > 0 && b3d_sky_gl_bind_image(
            sky, id, layer == GSKYBOX89_LAYER_DOME ? 1 : 0)) {
        sky->current_texture_ready = 1;
        return;
    }
    glBindTexture(GL_TEXTURE_2D, 0U);
    glDisable(GL_TEXTURE_2D);
}

static GLfloat b3d_sky_gl_fx(gskybox89_fx value)
{
    return (GLfloat)value / (GLfloat)GSKYBOX89_FX_ONE;
}

static GLfloat b3d_sky_gl_q16(long value)
{
    return (GLfloat)value / 65536.0f;
}

static GLfloat b3d_sky_gl_world(g3d_fix eye, gskybox89_fx local,
                                g3d_fix radius)
{
    long scaled;
    scaled = ((long)radius * (long)local) / GSKYBOX89_FX_ONE;
    return (GLfloat)(eye + (g3d_fix)scaled) / (GLfloat)G3D_FIX_ONE;
}

static void b3d_sky_gl_cube_uv(const Blank3DSkybox89 *sky,
                               const Gskybox89_Vertex *v,
                               GLfloat *out_u, GLfloat *out_v)
{
    long u0;
    long v0;
    long u1;
    long v1;
    long inset_u;
    long inset_v;
    long u_q16;
    long v_q16;
    u0 = sky->current_uv.u0_q16;
    v0 = sky->current_uv.v0_q16;
    u1 = sky->current_uv.u1_q16;
    v1 = sky->current_uv.v1_q16;
    if (sky->uv_inset_pixels > 0 && sky->current_texture_width > 0U &&
        sky->current_texture_height > 0U) {
        inset_u = ((long)sky->uv_inset_pixels * 65536L) /
                  (long)sky->current_texture_width;
        inset_v = ((long)sky->uv_inset_pixels * 65536L) /
                  (long)sky->current_texture_height;
        if (u1 - u0 > inset_u * 2L) {
            u0 += inset_u;
            u1 -= inset_u;
        }
        if (v1 - v0 > inset_v * 2L) {
            v0 += inset_v;
            v1 -= inset_v;
        }
    }
    u_q16 = u0 + ((u1 - u0) * (long)v->u) / GSKYBOX89_FX_ONE;
    v_q16 = v0 + ((v1 - v0) * (long)v->v) / GSKYBOX89_FX_ONE;
    *out_u = b3d_sky_gl_q16(u_q16);
    *out_v = b3d_sky_gl_q16(v_q16);
}

static void b3d_sky_gl_vertex(const Blank3DSkybox89 *sky,
                              const Gskybox89_Vertex *v)
{
    GLfloat u;
    GLfloat vv;
    glColor4ub((GLubyte)v->color.r, (GLubyte)v->color.g,
               (GLubyte)v->color.b, (GLubyte)v->color.a);
    if (sky->current_texture_ready) {
        if (v->layer == GSKYBOX89_LAYER_CUBE6) {
            b3d_sky_gl_cube_uv(sky, v, &u, &vv);
            glTexCoord2f(u, vv);
        } else if (v->layer == GSKYBOX89_LAYER_DOME) {
            glTexCoord2f(b3d_sky_gl_fx(v->u), b3d_sky_gl_fx(v->v));
        } else if (v->layer == GSKYBOX89_LAYER_SCREEN) {
            u = (b3d_sky_gl_fx(v->x) + 1.0f) * 0.5f;
            vv = (1.0f - b3d_sky_gl_fx(v->y)) * 0.5f;
            glTexCoord2f(u, vv);
        }
    }
    if (v->layer == GSKYBOX89_LAYER_SCREEN) {
        glVertex3f(b3d_sky_gl_fx(v->x), b3d_sky_gl_fx(v->y), 0.999f);
    } else {
        glVertex3f(b3d_sky_gl_world(sky->render_eye_q12.x, v->x,
                                    sky->radius_q12),
                   b3d_sky_gl_world(sky->render_eye_q12.y, v->y,
                                    sky->radius_q12),
                   b3d_sky_gl_world(sky->render_eye_q12.z, v->z,
                                    sky->radius_q12));
    }
}

static void b3d_sky_gl_emit_tri(void *user,
                                const Gskybox89_Vertex *a,
                                const Gskybox89_Vertex *b,
                                const Gskybox89_Vertex *c)
{
    Blank3DSkybox89 *sky;
    sky = (Blank3DSkybox89 *)user;
    if (!sky || !a || !b || !c) return;
    if (a->layer == GSKYBOX89_LAYER_CUBE6 && !sky->current_face_ready)
        return;
    glBegin(GL_TRIANGLES);
    b3d_sky_gl_vertex(sky, a);
    b3d_sky_gl_vertex(sky, b);
    b3d_sky_gl_vertex(sky, c);
    glEnd();
}

void blank3d_skybox89_gl_attach(Blank3DSkybox89 *sky)
{
    Gskybox89_Backend backend;
    if (!sky) return;
    memset(&backend, 0, sizeof(backend));
    backend.user = sky;
    backend.begin_pass = b3d_sky_gl_begin_pass;
    backend.end_pass = b3d_sky_gl_end_pass;
    backend.set_state = b3d_sky_gl_set_state;
    backend.bind_face = b3d_sky_gl_bind_face;
    backend.emit_tri = b3d_sky_gl_emit_tri;
    gskybox89_init(&sky->core, &sky->core_config, &backend);
}

int blank3d_skybox89_gl_render(Blank3DSkybox89 *sky, const Vec3 *eye_q12)
{
    int rc;
    if (!sky || !sky->enabled || !eye_q12) return 0;
    if (sky->core.cfg.layer_mask == 0) return 0;
    sky->render_eye_q12 = *eye_q12;
    sky->current_layer = 0;
    sky->current_face = GSKYBOX89_FACE_NONE;
    sky->current_face_ready = 0;
    sky->current_texture_ready = 0;
    b3d_sky_gl_save();
    rc = gskybox89_render(&sky->core, 0);
    b3d_sky_gl_restore();
    return rc == GSKYBOX89_OK;
}
