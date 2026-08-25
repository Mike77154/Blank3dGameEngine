#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>

#include "engine_bridge.h"

#define BRIDGE_Q16_MAX ((soq3d_fx)2147483647L)
#define BRIDGE_Q16_MIN ((soq3d_fx)(-2147483647L - 1L))

soq3d_fx bridge_q20_to_q16(g3d_fix value)
{
    if (value > (g3d_fix)134217727L) return BRIDGE_Q16_MAX;
    if (value < (g3d_fix)(-134217728L)) return BRIDGE_Q16_MIN;
    return (soq3d_fx)(value * 16L);
}

g3d_fix bridge_q16_to_q20(soq3d_fx value)
{
    return (g3d_fix)(value / 16L);
}

void bridge_transform_to_pose(const Transform *transform, soq3d_pose *out_pose)
{
    g3d_fix matrix[16];
    soq3d_basis3 basis;

    if (!transform || !out_pose) return;

    transform_to_matrix4(transform, matrix);

    basis.m00 = bridge_q20_to_q16(matrix[0]);
    basis.m01 = bridge_q20_to_q16(matrix[4]);
    basis.m02 = bridge_q20_to_q16(matrix[8]);
    basis.m10 = bridge_q20_to_q16(matrix[1]);
    basis.m11 = bridge_q20_to_q16(matrix[5]);
    basis.m12 = bridge_q20_to_q16(matrix[9]);
    basis.m20 = bridge_q20_to_q16(matrix[2]);
    basis.m21 = bridge_q20_to_q16(matrix[6]);
    basis.m22 = bridge_q20_to_q16(matrix[10]);

    out_pose->position.x = bridge_q20_to_q16(matrix[12]);
    out_pose->position.y = bridge_q20_to_q16(matrix[13]);
    out_pose->position.z = bridge_q20_to_q16(matrix[14]);
    out_pose->basis = basis;
}

Vec3 bridge_pose_position_q20(const soq3d_pose *pose)
{
    Vec3 result;
    if (!pose) return gamlib_vec3(0, 0, 0);
    result.x = bridge_q16_to_q20(pose->position.x);
    result.y = bridge_q16_to_q20(pose->position.y);
    result.z = bridge_q16_to_q20(pose->position.z);
    return result;
}

static GLfloat bridge_q20_to_gl(g3d_fix value)
{
    return (GLfloat)value / (GLfloat)G3D_FIX_ONE;
}

static GLfloat bridge_q16_to_gl(g3d_fx value)
{
    return (GLfloat)value / (GLfloat)G3D_FX_ONE;
}

void bridge_gl_load_q20_matrix(const g3d_fix matrix[16])
{
    GLfloat gl_matrix[16];
    int i;
    if (!matrix) return;
    for (i = 0; i < 16; ++i) gl_matrix[i] = bridge_q20_to_gl(matrix[i]);
    glLoadMatrixf(gl_matrix);
}

void bridge_gl_apply_pose(const soq3d_pose *pose)
{
    GLfloat matrix[16];
    if (!pose) return;

    matrix[0] = bridge_q16_to_gl(pose->basis.m00);
    matrix[1] = bridge_q16_to_gl(pose->basis.m10);
    matrix[2] = bridge_q16_to_gl(pose->basis.m20);
    matrix[3] = 0.0f;

    matrix[4] = bridge_q16_to_gl(pose->basis.m01);
    matrix[5] = bridge_q16_to_gl(pose->basis.m11);
    matrix[6] = bridge_q16_to_gl(pose->basis.m21);
    matrix[7] = 0.0f;

    matrix[8] = bridge_q16_to_gl(pose->basis.m02);
    matrix[9] = bridge_q16_to_gl(pose->basis.m12);
    matrix[10] = bridge_q16_to_gl(pose->basis.m22);
    matrix[11] = 0.0f;

    matrix[12] = bridge_q16_to_gl(pose->position.x);
    matrix[13] = bridge_q16_to_gl(pose->position.y);
    matrix[14] = bridge_q16_to_gl(pose->position.z);
    matrix[15] = 1.0f;

    glMultMatrixf(matrix);
}

void bridge_gl_apply_q16_matrix(const signed int matrix[4][4])
{
    GLfloat gl_matrix[16];
    int row;
    int column;
    if (!matrix) return;
    for (column = 0; column < 4; ++column) {
        for (row = 0; row < 4; ++row) {
            gl_matrix[column * 4 + row] =
                (GLfloat)matrix[row][column] / 65536.0f;
        }
    }
    glMultMatrixf(gl_matrix);
}

void bridge_gl_draw_mesh(const g3d_mesh *mesh)
{
    unsigned short i;
    const g3d_vertex *vertex;

    if (!mesh || !mesh->vertices || !mesh->indices) return;
    if (mesh->primitive != G3D_PRIMITIVE_TRIANGLES) return;

    glBegin(GL_TRIANGLES);
    for (i = 0U; i < mesh->index_count; ++i) {
        if (mesh->indices[i] >= mesh->vertex_count) continue;
        vertex = &mesh->vertices[mesh->indices[i]];
        glColor4ub(vertex->color.r, vertex->color.g,
                   vertex->color.b, vertex->color.a);
        glNormal3f(bridge_q16_to_gl(vertex->normal.x),
                   bridge_q16_to_gl(vertex->normal.y),
                   bridge_q16_to_gl(vertex->normal.z));
        glVertex3f(bridge_q16_to_gl(vertex->position.x),
                   bridge_q16_to_gl(vertex->position.y),
                   bridge_q16_to_gl(vertex->position.z));
    }
    glEnd();
}


void bridge_gl_begin_frame(int width, int height)
{
    GLfloat light_position[4];
    glViewport(0, 0, width, height);
    glClearColor(0.04f, 0.05f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Frame boundary is also a render-state boundary.  Raster effects may use
       additive blending, textures and depth-write suppression, but no effect
       is allowed to leak those flags into next-frame world meshes. */
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glColor4ub(255U, 255U, 255U, 255U);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHT2);
    glEnable(GL_COLOR_MATERIAL);
    light_position[0] = 2.0f;
    light_position[1] = 8.0f;
    light_position[2] = -4.0f;
    light_position[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

void bridge_gl_set_muzzle_light(int enabled,
                                g3d_fix x_q12, g3d_fix y_q12, g3d_fix z_q12,
                                long intensity_q16, long radius_q16,
                                unsigned char r, unsigned char g,
                                unsigned char b)
{
    GLfloat position[4];
    GLfloat diffuse[4];
    GLfloat ambient[4];
    GLfloat specular[4];
    GLfloat intensity;
    GLfloat radius;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    if (!enabled || intensity_q16 <= 0L || radius_q16 <= 0L) {
        glDisable(GL_LIGHT1);
        return;
    }
    intensity = (GLfloat)intensity_q16 / 65536.0f;
    radius = (GLfloat)radius_q16 / 65536.0f;
    if (radius < 0.25f) radius = 0.25f;
    if (intensity > 4.0f) intensity = 4.0f;
    red = (GLfloat)r / 255.0f;
    green = (GLfloat)g / 255.0f;
    blue = (GLfloat)b / 255.0f;
    position[0] = bridge_q20_to_gl(x_q12);
    position[1] = bridge_q20_to_gl(y_q12);
    position[2] = bridge_q20_to_gl(z_q12);
    position[3] = 1.0f;
    diffuse[0] = red * intensity;
    diffuse[1] = green * intensity;
    diffuse[2] = blue * intensity;
    diffuse[3] = 1.0f;
    ambient[0] = red * intensity * 0.055f;
    ambient[1] = green * intensity * 0.055f;
    ambient[2] = blue * intensity * 0.055f;
    ambient[3] = 1.0f;
    specular[0] = red * intensity;
    specular[1] = green * intensity;
    specular[2] = blue * intensity;
    specular[3] = 1.0f;
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, position);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_SPECULAR, specular);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 2.0f / radius);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 4.0f / (radius * radius));
}


void bridge_gl_set_flamethrower_light(int enabled,
                                      g3d_fix x_q12,
                                      g3d_fix y_q12,
                                      g3d_fix z_q12,
                                      long intensity_q16,
                                      long radius_q16,
                                      unsigned char r,
                                      unsigned char g,
                                      unsigned char b)
{
    GLfloat position[4];
    GLfloat diffuse[4];
    GLfloat ambient[4];
    GLfloat specular[4];
    GLfloat intensity;
    GLfloat radius;
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    if (!enabled || intensity_q16 <= 0L || radius_q16 <= 0L) {
        glDisable(GL_LIGHT2);
        return;
    }
    intensity = (GLfloat)intensity_q16 / 65536.0f;
    radius = (GLfloat)radius_q16 / 65536.0f;
    if (radius < 0.25f) radius = 0.25f;
    if (intensity > 3.25f) intensity = 3.25f;
    red = (GLfloat)r / 255.0f;
    green = (GLfloat)g / 255.0f;
    blue = (GLfloat)b / 255.0f;
    position[0] = bridge_q20_to_gl(x_q12);
    position[1] = bridge_q20_to_gl(y_q12);
    position[2] = bridge_q20_to_gl(z_q12);
    position[3] = 1.0f;
    diffuse[0] = red * intensity;
    diffuse[1] = green * intensity;
    diffuse[2] = blue * intensity;
    diffuse[3] = 1.0f;
    ambient[0] = red * intensity * 0.045f;
    ambient[1] = green * intensity * 0.045f;
    ambient[2] = blue * intensity * 0.045f;
    ambient[3] = 1.0f;
    specular[0] = red * intensity * 0.65f;
    specular[1] = green * intensity * 0.65f;
    specular[2] = blue * intensity * 0.65f;
    specular[3] = 1.0f;
    glEnable(GL_LIGHT2);
    glLightfv(GL_LIGHT2, GL_POSITION, position);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT2, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT2, GL_SPECULAR, specular);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 1.65f / radius);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 3.25f / (radius * radius));
}

void bridge_gl_draw_grid(int half_extent)
{
    int i;
    glDisable(GL_LIGHTING);
    glColor3ub(64U, 64U, 64U);
    glBegin(GL_LINES);
    for (i = -half_extent; i <= half_extent; ++i) {
        glVertex3i(i, 0, -half_extent);
        glVertex3i(i, 0, half_extent);
        glVertex3i(-half_extent, 0, i);
        glVertex3i(half_extent, 0, i);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}


void bridge_gl_draw_segment(const Vec3 *a, const Vec3 *b,
                            unsigned char r, unsigned char g,
                            unsigned char b_color)
{
    if (!a || !b) return;
    glDisable(GL_LIGHTING);
    glColor3ub(r, g, b_color);
    glBegin(GL_LINES);
    glVertex3f(bridge_q20_to_gl(a->x),
               bridge_q20_to_gl(a->y),
               bridge_q20_to_gl(a->z));
    glVertex3f(bridge_q20_to_gl(b->x),
               bridge_q20_to_gl(b->y),
               bridge_q20_to_gl(b->z));
    glEnd();
    glEnable(GL_LIGHTING);
}
