#include "blank3d_trails.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <string.h>

static void blank3d_trail_desc(int profile_id, t3d89_desc *desc)
{
    if (!desc) return;
    if (profile_id == B3D_TRAIL_TRACER) {
        t3d89_profile_tracer(desc);
    } else if (profile_id == B3D_TRAIL_HEAVY) {
        t3d89_profile_tracer(desc);
        desc->width_head = 18;
        desc->width_tail = 2;
        desc->life_ticks = 16;
        desc->color_head = t3d89_color_make(255, 235, 170, 230);
        desc->color_tail = t3d89_color_make(255, 105, 35, 0);
    } else if (profile_id == B3D_TRAIL_ARC) {
        t3d89_profile_bullet(desc);
        desc->life_ticks = 28;
        desc->width_head = 10;
        desc->width_tail = 1;
        desc->color_head = t3d89_color_make(220, 235, 255, 190);
        desc->color_tail = t3d89_color_make(100, 130, 180, 0);
    } else {
        t3d89_profile_bullet(desc);
    }
    desc->max_points = 48;
    desc->min_ticks = 1;
}

void blank3d_trails_init(Blank3DTrails *trails)
{
    int i;
    if (!trails) return;
    memset(trails, 0, sizeof(*trails));
    t3d89_init(&trails->ctx);
    for (i = 0; i < T3D89_MAX_TRAILS; ++i)
        trails->bullet_for_trail[i] = -1;
    for (i = 0; i < 128; ++i)
        trails->trail_for_bullet[i] = -1;
    trails->next_recycle = 0;
    t3d89_mesh_clear(&trails->mesh);
    trails->mesh.vertices = trails->vertices;
    trails->mesh.indices = trails->indices;
    trails->mesh.max_vertices = BLANK3D_TRAIL_VERTEX_CAPACITY;
    trails->mesh.max_indices = BLANK3D_TRAIL_INDEX_CAPACITY;
}

int blank3d_trails_attach(Blank3DTrails *trails,
                          int bullet_index,
                          int profile_id)
{
    t3d89_desc desc;
    int trail_id;
    int oldest;
    if (!trails || bullet_index < 0 || bullet_index >= 128 ||
        profile_id == B3D_TRAIL_NONE) return -1;
    blank3d_trails_release(trails, bullet_index);
    blank3d_trail_desc(profile_id, &desc);
    trail_id = t3d89_create(&trails->ctx, &desc);
    if (trail_id < 0) {
        oldest = trails->next_recycle;
        if (oldest < 0 || oldest >= T3D89_MAX_TRAILS) oldest = 0;
        trails->next_recycle = oldest + 1;
        if (trails->next_recycle >= T3D89_MAX_TRAILS)
            trails->next_recycle = 0;
        if (trails->bullet_for_trail[oldest] >= 0)
            trails->trail_for_bullet[trails->bullet_for_trail[oldest]] = -1;
        (void)t3d89_destroy(&trails->ctx, oldest);
        trails->bullet_for_trail[oldest] = -1;
        trail_id = t3d89_create(&trails->ctx, &desc);
    }
    if (trail_id >= 0 && trail_id < T3D89_MAX_TRAILS) {
        trails->trail_for_bullet[bullet_index] = trail_id;
        trails->bullet_for_trail[trail_id] = bullet_index;
    }
    return trail_id;
}

void blank3d_trails_release(Blank3DTrails *trails, int bullet_index)
{
    int trail_id;
    if (!trails || bullet_index < 0 || bullet_index >= 128) return;
    trail_id = trails->trail_for_bullet[bullet_index];
    if (trail_id >= 0 && trail_id < T3D89_MAX_TRAILS) {
        (void)t3d89_destroy(&trails->ctx, trail_id);
        trails->bullet_for_trail[trail_id] = -1;
    }
    trails->trail_for_bullet[bullet_index] = -1;
}

void blank3d_trails_emit_q12(Blank3DTrails *trails,
                             int bullet_index,
                             long x_q12,
                             long y_q12,
                             long z_q12)
{
    int trail_id;
    if (!trails || bullet_index < 0 || bullet_index >= 128) return;
    trail_id = trails->trail_for_bullet[bullet_index];
    if (trail_id < 0) return;
    (void)t3d89_emit_point(&trails->ctx, trail_id,
                           (int)(x_q12 / 16L),
                           (int)(y_q12 / 16L),
                           (int)(z_q12 / 16L));
}

void blank3d_trails_tick(Blank3DTrails *trails, int dt_ticks)
{
    if (!trails) return;
    if (dt_ticks < 1) dt_ticks = 1;
    (void)t3d89_tick(&trails->ctx, dt_ticks);
}

static GLfloat blank3d_q8_to_gl(int value)
{
    return (GLfloat)value / 256.0f;
}

void blank3d_trails_draw(Blank3DTrails *trails,
                         long camera_x_q12,
                         long camera_y_q12,
                         long camera_z_q12)
{
    t3d89_camera camera;
    int trail_id;
    int index;
    int vertex_index;
    t3d89_vertex *vertex;
    if (!trails) return;
    camera.x = (int)(camera_x_q12 / 16L);
    camera.y = (int)(camera_y_q12 / 16L);
    camera.z = (int)(camera_z_q12 / 16L);
    camera.up_x = 0;
    camera.up_y = T3D89_FP_ONE;
    camera.up_z = 0;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (trail_id = 0; trail_id < T3D89_MAX_TRAILS; ++trail_id) {
        if (!trails->ctx.trails[trail_id].active) continue;
        t3d89_mesh_clear(&trails->mesh);
        trails->mesh.vertices = trails->vertices;
        trails->mesh.indices = trails->indices;
        trails->mesh.max_vertices = BLANK3D_TRAIL_VERTEX_CAPACITY;
        trails->mesh.max_indices = BLANK3D_TRAIL_INDEX_CAPACITY;
        if (t3d89_build_mesh(&trails->ctx, trail_id,
                             &camera, &trails->mesh) != T3D89_OK)
            continue;
        glBegin(GL_TRIANGLES);
        for (index = 0; index < trails->mesh.index_count; ++index) {
            vertex_index = (int)trails->mesh.indices[index];
            if (vertex_index < 0 || vertex_index >= trails->mesh.vertex_count)
                continue;
            vertex = &trails->mesh.vertices[vertex_index];
            glColor4ub(vertex->r, vertex->g, vertex->b, vertex->a);
            glVertex3f(blank3d_q8_to_gl(vertex->x),
                       blank3d_q8_to_gl(vertex->y),
                       blank3d_q8_to_gl(vertex->z));
        }
        glEnd();
    }
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}
