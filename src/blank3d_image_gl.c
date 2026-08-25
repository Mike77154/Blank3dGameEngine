#include "blank3d_image_gl.h"

#include <gl/gl.h>

static void b3d_gl_rgba(unsigned long rgba)
{
    glColor4ub((GLubyte)((rgba >> 24) & 255UL),
               (GLubyte)((rgba >> 16) & 255UL),
               (GLubyte)((rgba >> 8) & 255UL),
               (GLubyte)(rgba & 255UL));
}

static unsigned int b3d_gl_upload_rgba(void *user,
                                        const unsigned char *rgba,
                                        unsigned int width,
                                        unsigned int height)
{
    GLuint texture;
    (void)user;
    if (!rgba || !width || !height) return 0U;
    texture = 0U;
    glGenTextures(1, &texture);
    if (!texture) return 0U;
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 (GLsizei)width, (GLsizei)height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0U);
    return (unsigned int)texture;
}

static void b3d_gl_destroy(void *user, unsigned int texture_token)
{
    GLuint texture;
    (void)user;
    texture = (GLuint)texture_token;
    if (texture) glDeleteTextures(1, &texture);
}

void blank3d_image_gl_make_backend(Blank3DImageBackend *backend)
{
    if (!backend) return;
    backend->user = 0;
    backend->upload_rgba = b3d_gl_upload_rgba;
    backend->destroy_texture = b3d_gl_destroy;
}


static int b3d_image_overlay_height;

void blank3d_image_gl_begin_overlay(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    b3d_image_overlay_height = height;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)width, 0.0, (GLdouble)height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Overlay sprites are screen-space presentation.  A muzzle light from an
       NPC must never modulate them.  Individual image draws enable texturing
       only for the duration of the textured quad. */
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255U, 255U, 255U, 255U);
}

void blank3d_image_gl_end_overlay(void)
{
    glBindTexture(GL_TEXTURE_2D, 0U);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4ub(255U, 255U, 255U, 255U);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    b3d_image_overlay_height = 0;
}

int blank3d_image_gl_draw_subrect(Blank3DImageAssets *assets, int image_id,
                                  int sx, int sy, int sw, int sh,
                                  int dx, int dy, int flip_x, int flip_y)
{
    const Blank3DImageAsset *slot;
    GLfloat u0;
    GLfloat v0;
    GLfloat u1;
    GLfloat v1;
    GLfloat t;
    int y0;
    int y1;
    if (!assets || image_id <= 0 || b3d_image_overlay_height <= 0) return 0;
    if (!blank3d_image_assets_load(assets, image_id)) return 0;
    slot = blank3d_image_assets_get(assets, image_id);
    if (!slot || !slot->loaded || !slot->texture_token ||
        slot->width == 0U || slot->height == 0U) return 0;
    if (sw <= 0) { sx = 0; sw = (int)slot->width; }
    if (sh <= 0) { sy = 0; sh = (int)slot->height; }
    if (sw <= 0 || sh <= 0) return 0;
    u0 = (GLfloat)sx / (GLfloat)slot->width;
    v0 = (GLfloat)sy / (GLfloat)slot->height;
    u1 = (GLfloat)(sx + sw) / (GLfloat)slot->width;
    v1 = (GLfloat)(sy + sh) / (GLfloat)slot->height;
    if (flip_x) { t = u0; u0 = u1; u1 = t; }
    if (flip_y) { t = v0; v0 = v1; v1 = t; }
    y0 = b3d_image_overlay_height - dy - sh;
    y1 = y0 + sh;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, (GLuint)slot->texture_token);
    glColor4ub(255U, 255U, 255U, 255U);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex2i(dx,      y0);
    glTexCoord2f(u1, v1); glVertex2i(dx + sw, y0);
    glTexCoord2f(u1, v0); glVertex2i(dx + sw, y1);
    glTexCoord2f(u0, v0); glVertex2i(dx,      y1);
    glEnd();
    return 1;
}


int blank3d_image_gl_world_effect_begin(Blank3DImageAssets *assets,
                                        int image_id, int additive)
{
    const Blank3DImageAsset *asset;
    if (!assets || image_id <= 0) return 0;
    if (!blank3d_image_assets_load(assets, image_id)) return 0;
    asset = blank3d_image_assets_get(assets, image_id);
    if (!asset || !asset->loaded || !asset->texture_token) return 0;
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, (GLuint)asset->texture_token);
    return 1;
}

void blank3d_image_gl_world_billboard_subrect(
    const Blank3DImageAsset *asset,
    int sx, int sy, int sw, int sh,
    const Vec3 *center, const Vec3 *camera_right, const Vec3 *camera_up,
    g3d_fix width_q12, g3d_fix height_q12,
    int flip_x, unsigned long tint_rgba)
{
    Vec3 half_right;
    Vec3 half_up;
    Vec3 top_left;
    Vec3 top_right;
    Vec3 bottom_right;
    Vec3 bottom_left;
    GLfloat u0;
    GLfloat v0;
    GLfloat u1;
    GLfloat v1;
    GLfloat t;
    if (!asset || !center || !camera_right || !camera_up ||
        !asset->width || !asset->height || sw <= 0 || sh <= 0 ||
        width_q12 <= 0 || height_q12 <= 0) return;
    u0 = ((GLfloat)sx + 0.5f) / (GLfloat)asset->width;
    v0 = ((GLfloat)sy + 0.5f) / (GLfloat)asset->height;
    u1 = ((GLfloat)(sx + sw) - 0.5f) / (GLfloat)asset->width;
    v1 = ((GLfloat)(sy + sh) - 0.5f) / (GLfloat)asset->height;
    if (flip_x) {
        t = u0;
        u0 = u1;
        u1 = t;
    }
    gamlib_vec3_scale(&half_right, camera_right, width_q12 / 2);
    gamlib_vec3_scale(&half_up, camera_up, height_q12 / 2);
    gamlib_vec3_sub(&top_left, center, &half_right);
    gamlib_vec3_add(&top_left, &top_left, &half_up);
    gamlib_vec3_add(&top_right, center, &half_right);
    gamlib_vec3_add(&top_right, &top_right, &half_up);
    gamlib_vec3_add(&bottom_right, center, &half_right);
    gamlib_vec3_sub(&bottom_right, &bottom_right, &half_up);
    gamlib_vec3_sub(&bottom_left, center, &half_right);
    gamlib_vec3_sub(&bottom_left, &bottom_left, &half_up);

    b3d_gl_rgba(tint_rgba);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex3f((GLfloat)top_left.x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)top_left.y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)top_left.z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u1, v0);
    glVertex3f((GLfloat)top_right.x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)top_right.y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)top_right.z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u1, v1);
    glVertex3f((GLfloat)bottom_right.x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)bottom_right.y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)bottom_right.z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u0, v1);
    glVertex3f((GLfloat)bottom_left.x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)bottom_left.y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)bottom_left.z / (GLfloat)G3D_FIX_ONE);
    glEnd();
}

void blank3d_image_gl_world_quad_subrect(
    const Blank3DImageAsset *asset,
    int sx, int sy, int sw, int sh,
    const Vec3 corners[4], int flip_x, int flip_y,
    unsigned long tint_rgba)
{
    GLfloat u0;
    GLfloat v0;
    GLfloat u1;
    GLfloat v1;
    GLfloat t;
    if (!asset || !corners || !asset->width || !asset->height ||
        sw <= 0 || sh <= 0) return;
    u0 = ((GLfloat)sx + 0.5f) / (GLfloat)asset->width;
    v0 = ((GLfloat)sy + 0.5f) / (GLfloat)asset->height;
    u1 = ((GLfloat)(sx + sw) - 0.5f) / (GLfloat)asset->width;
    v1 = ((GLfloat)(sy + sh) - 0.5f) / (GLfloat)asset->height;
    if (flip_x) {
        t = u0; u0 = u1; u1 = t;
    }
    if (flip_y) {
        t = v0; v0 = v1; v1 = t;
    }
    b3d_gl_rgba(tint_rgba);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);
    glVertex3f((GLfloat)corners[0].x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[0].y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[0].z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u1, v0);
    glVertex3f((GLfloat)corners[1].x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[1].y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[1].z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u1, v1);
    glVertex3f((GLfloat)corners[2].x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[2].y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[2].z / (GLfloat)G3D_FIX_ONE);
    glTexCoord2f(u0, v1);
    glVertex3f((GLfloat)corners[3].x / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[3].y / (GLfloat)G3D_FIX_ONE,
               (GLfloat)corners[3].z / (GLfloat)G3D_FIX_ONE);
    glEnd();
}

void blank3d_image_gl_world_effect_end(void)
{
    glBindTexture(GL_TEXTURE_2D, 0U);
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255U, 255U, 255U, 255U);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void b3d_hud_textured_quad(Blank3DImageAssets *assets,
                                  int sprite_id,
                                  int sx, int sy, int sw, int sh,
                                  int dx, int dy, int dw, int dh,
                                  unsigned long tint_rgba,
                                  int use_uv,
                                  int u0, int v0, int u1, int v1)
{
    const Blank3DImageAsset *slot;
    GLfloat fu0;
    GLfloat fv0;
    GLfloat fu1;
    GLfloat fv1;
    if (!assets || dw <= 0 || dh <= 0) return;
    if (!blank3d_image_assets_load(assets, sprite_id)) return;
    slot = blank3d_image_assets_get(assets, sprite_id);
    if (!slot || !slot->loaded || !slot->texture_token) return;
    if (use_uv) {
        fu0 = (GLfloat)u0 / 10000.0f;
        fv0 = (GLfloat)v0 / 10000.0f;
        fu1 = (GLfloat)u1 / 10000.0f;
        fv1 = (GLfloat)v1 / 10000.0f;
    } else if (sw > 0 && sh > 0 && slot->width && slot->height) {
        fu0 = (GLfloat)sx / (GLfloat)slot->width;
        fv0 = (GLfloat)sy / (GLfloat)slot->height;
        fu1 = (GLfloat)(sx + sw) / (GLfloat)slot->width;
        fv1 = (GLfloat)(sy + sh) / (GLfloat)slot->height;
    } else {
        fu0 = 0.0f; fv0 = 0.0f; fu1 = 1.0f; fv1 = 1.0f;
    }
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, (GLuint)slot->texture_token);
    b3d_gl_rgba(tint_rgba);
    glBegin(GL_QUADS);
    glTexCoord2f(fu0, fv0); glVertex2i(dx,      dh + dy);
    glTexCoord2f(fu1, fv0); glVertex2i(dx + dw, dh + dy);
    glTexCoord2f(fu1, fv1); glVertex2i(dx + dw, dy);
    glTexCoord2f(fu0, fv1); glVertex2i(dx,      dy);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0U);
    glDisable(GL_TEXTURE_2D);
}

static void b3d_hud_draw_image(void *user, int sprite_id,
                               int sx, int sy, int sw, int sh,
                               int dx, int dy, int dw, int dh,
                               unsigned long tint_rgba)
{
    b3d_hud_textured_quad((Blank3DImageAssets *)user, sprite_id,
                          sx, sy, sw, sh, dx, dy, dw, dh,
                          tint_rgba, 0, 0, 0, 0, 0);
}

static void b3d_hud_draw_image_uv(void *user, int sprite_id,
                                  int u0, int v0, int u1, int v1,
                                  int dx, int dy, int dw, int dh,
                                  unsigned long tint_rgba)
{
    b3d_hud_textured_quad((Blank3DImageAssets *)user, sprite_id,
                          0, 0, 0, 0, dx, dy, dw, dh,
                          tint_rgba, 1, u0, v0, u1, v1);
}

void blank3d_image_gl_make_hud_provider(Blank3DHudSpriteProvider *provider,
                                        Blank3DImageAssets *assets)
{
    if (!provider) return;
    provider->user = assets;
    provider->draw = b3d_hud_draw_image;
    provider->draw_uv = b3d_hud_draw_image_uv;
}

static void b3d_world_blend(unsigned short blend)
{
    glEnable(GL_BLEND);
    if (blend == SP89_BLEND_ADDITIVE || blend == SP89_BLEND_SOFT_ADD)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else if (blend == SP89_BLEND_MULTIPLY)
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void blank3d_image_gl_draw_spriteplanes(Blank3DImageAssets *assets,
                                        const sprpl89_emit *emit)
{
    int p;
    if (!assets || !emit) return;
    /* Raster SpritePlanes are presentation geometry.  Keep them full-bright
       and isolate their blend/depth state from the lit 3D mesh pass.  The
       old path leaked additive blending into casings and every mesh drawn
       after the muzzle flash. */
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    for (p = 0; p < emit->packet_count; ++p) {
        const sprpl89_packet *packet;
        const Blank3DImageAsset *slot;
        int ti;
        packet = &emit->packets[p];
        if (packet->primitive_family != SP89_MAT_RASTER) continue;
        if (!blank3d_image_assets_load(assets, (int)packet->page_id)) continue;
        slot = blank3d_image_assets_get(assets, (int)packet->page_id);
        if (!slot || !slot->loaded) continue;
        if (packet->flags & SP89_FLAG_DEPTH_TEST) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        b3d_world_blend(packet->blend_mode);
        glBindTexture(GL_TEXTURE_2D, (GLuint)slot->texture_token);
        glBegin(GL_TRIANGLES);
        for (ti = packet->first_tri; ti < packet->first_tri + packet->tri_count; ++ti) {
            const sprpl89_tri *tri;
            unsigned short ids[3];
            int k;
            tri = &emit->tris[ti];
            ids[0] = tri->a; ids[1] = tri->b; ids[2] = tri->c;
            for (k = 0; k < 3; ++k) {
                const sprpl89_vertex *v;
                v = &emit->verts[ids[k]];
                glColor4ub(v->color.r, v->color.g, v->color.b, v->color.a);
                glTexCoord2f((GLfloat)v->u / (GLfloat)SP89_FX_ONE,
                             (GLfloat)v->v / (GLfloat)SP89_FX_ONE);
                glVertex3f((GLfloat)v->pos.x / (GLfloat)SP89_FX_ONE,
                           (GLfloat)v->pos.y / (GLfloat)SP89_FX_ONE,
                           (GLfloat)v->pos.z / (GLfloat)SP89_FX_ONE);
            }
        }
        glEnd();
    }
    glBindTexture(GL_TEXTURE_2D, 0U);
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255U, 255U, 255U, 255U);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
