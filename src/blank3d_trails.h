#ifndef BLANK3D_TRAILS_H
#define BLANK3D_TRAILS_H

#include "blank3d_weapon_modules.h"
#include "trail3d89.h"
#include "trail3d89_profiles.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLANK3D_TRAIL_VERTEX_CAPACITY 768
#define BLANK3D_TRAIL_INDEX_CAPACITY 2304

typedef struct Blank3DTrailsTag {
    t3d89_ctx ctx;
    int bullet_for_trail[T3D89_MAX_TRAILS];
    int trail_for_bullet[128];
    int next_recycle;
    t3d89_vertex vertices[BLANK3D_TRAIL_VERTEX_CAPACITY];
    unsigned short indices[BLANK3D_TRAIL_INDEX_CAPACITY];
    t3d89_mesh mesh;
} Blank3DTrails;

void blank3d_trails_init(Blank3DTrails *trails);
int blank3d_trails_attach(Blank3DTrails *trails,
                          int bullet_index,
                          int profile_id);
void blank3d_trails_release(Blank3DTrails *trails, int bullet_index);
void blank3d_trails_emit_q12(Blank3DTrails *trails,
                             int bullet_index,
                             long x_q12,
                             long y_q12,
                             long z_q12);
void blank3d_trails_tick(Blank3DTrails *trails, int dt_ticks);
void blank3d_trails_draw(Blank3DTrails *trails,
                         long camera_x_q12,
                         long camera_y_q12,
                         long camera_z_q12);

#ifdef __cplusplus
}
#endif

#endif
