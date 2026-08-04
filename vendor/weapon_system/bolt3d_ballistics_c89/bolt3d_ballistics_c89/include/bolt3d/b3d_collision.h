#ifndef B3D_COLLISION_H
#define B3D_COLLISION_H

#include "bolt3d/b3d_vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_HIT_NONE 0
#define B3D_HIT_ENTITY 1
#define B3D_HIT_WORLD 2
#define B3D_HIT_CUSTOM 3

#define B3D_LAYER_WORLD 0x00000001
#define B3D_LAYER_PLAYER 0x00000002
#define B3D_LAYER_ENEMY 0x00000004
#define B3D_LAYER_TRIGGER 0x00000008
#define B3D_LAYER_PROJECTILE 0x00000010
#define B3D_LAYER_PROP 0x00000020
#define B3D_LAYER_USER_0 0x00010000
#define B3D_LAYER_USER_1 0x00020000
#define B3D_LAYER_USER_2 0x00040000
#define B3D_LAYER_USER_3 0x00080000
#define B3D_LAYER_ALL 0x7FFFFFFF

typedef struct B3D_Collider {
    int active;
    int id;
    int layer_mask;
    B3D_Vec3 center;
    B3D_Fixed radius;
    int user_type;
    void *user_ptr;
} B3D_Collider;

typedef struct B3D_Hit {
    int hit;
    int hit_kind;
    int target_id;
    int target_slot;
    int target_layer;
    int target_user_type;
    B3D_Fixed t;
    B3D_Vec3 point;
    B3D_Vec3 normal;
    void *user_ptr;
} B3D_Hit;

B3D_API void b3d_hit_clear(B3D_Hit *hit);
B3D_API void b3d_collider_clear(B3D_Collider *collider);
B3D_API int b3d_sphere_overlap(B3D_Vec3 a, B3D_Fixed radius_a, B3D_Vec3 b, B3D_Fixed radius_b);
B3D_API int b3d_segment_sphere(B3D_Vec3 from, B3D_Vec3 to, B3D_Vec3 center, B3D_Fixed radius, B3D_Hit *out_hit);

#ifdef __cplusplus
}
#endif

#endif
