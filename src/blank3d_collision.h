#ifndef BLANK3D_COLLISION_H
#define BLANK3D_COLLISION_H

#include "gweapon89.h"
#include "ccs_world.h"
#include "sicol_world.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_COLLISION_MAX_ENEMIES 32
#define B3D_COLLISION_STATIC_COUNT 2

#define B3D_COLLISION_LAYER_WORLD  1U
#define B3D_COLLISION_LAYER_ENEMY  2U
#define B3D_COLLISION_LAYER_PLAYER 4U
#define B3D_COLLISION_LAYER_BULLET 8U

#define B3D_COLLISION_MATERIAL_NONE  0
#define B3D_COLLISION_MATERIAL_FLESH 1
#define B3D_COLLISION_MATERIAL_WORLD 2

typedef struct Blank3DCollisionHitTag {
    int hit;
    int enemy_index;
    int material_id;
    gwp89_fx fraction_fx;
    gwp89_fx distance_fx;
    GWP89_Vec3 point;
    GWP89_Vec3 normal;
} Blank3DCollisionHit;

typedef struct Blank3DCollisionTag {
    int initialized;

    ccs_vec3 ccs_enemy_positions[B3D_COLLISION_MAX_ENEMIES];
    ccs_shape_capsule ccs_enemy_shapes[B3D_COLLISION_MAX_ENEMIES];
    int ccs_enemy_ids[B3D_COLLISION_MAX_ENEMIES];
    ccs_vec3 ccs_static_positions[B3D_COLLISION_STATIC_COUNT];
    ccs_shape_box ccs_static_shapes[B3D_COLLISION_STATIC_COUNT];
    int ccs_static_ids[B3D_COLLISION_STATIC_COUNT];

    sicol_world_t sicol_world;
    int sicol_enemy_ids[B3D_COLLISION_MAX_ENEMIES];
    int sicol_static_ids[B3D_COLLISION_STATIC_COUNT];

    char status[160];
} Blank3DCollision;

void blank3d_collision_init(Blank3DCollision *collision);
void blank3d_collision_set_enemy(Blank3DCollision *collision,
                                 int enemy_index,
                                 int active,
                                 gwp89_fx x,
                                 gwp89_fx y,
                                 gwp89_fx z);
void blank3d_collision_step(Blank3DCollision *collision);

int blank3d_collision_sweep_bullet_mask(Blank3DCollision *collision,
                                        const GWP89_Vec3 *start,
                                        const GWP89_Vec3 *delta,
                                        gwp89_fx radius_fx,
                                        unsigned int target_layers,
                                        Blank3DCollisionHit *out_hit);

int blank3d_collision_sweep_bullet(Blank3DCollision *collision,
                                   const GWP89_Vec3 *start,
                                   const GWP89_Vec3 *delta,
                                   gwp89_fx radius_fx,
                                   Blank3DCollisionHit *out_hit);

int blank3d_collision_raycast(Blank3DCollision *collision,
                              const GWP89_Vec3 *origin,
                              const GWP89_Vec3 *direction,
                              gwp89_fx range_fx,
                              unsigned int layer_mask,
                              Blank3DCollisionHit *out_hit);

int blank3d_collision_weapon_provider(void *context,
                                      GWP89_ProviderPacket *packet);

const char *blank3d_collision_status(const Blank3DCollision *collision);

#ifdef __cplusplus
}
#endif

#endif
