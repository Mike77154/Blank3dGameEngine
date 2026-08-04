#ifndef B3D_PROJECTILE_H
#define B3D_PROJECTILE_H

#include "bolt3d/b3d_events.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_PROJECTILE_KIND_GENERIC 0
#define B3D_PROJECTILE_KIND_BULLET 1
#define B3D_PROJECTILE_KIND_ROCKET 2
#define B3D_PROJECTILE_KIND_GRENADE 3
#define B3D_PROJECTILE_KIND_PLASMA 4
#define B3D_PROJECTILE_KIND_MELEE_TRACE 5
#define B3D_PROJECTILE_KIND_CUSTOM 1000

#define B3D_PROJ_USE_GRAVITY 0x00000001
#define B3D_PROJ_BOUNCE 0x00000002
#define B3D_PROJ_EXPLODE_ON_HIT 0x00000004
#define B3D_PROJ_PIERCE 0x00000008
#define B3D_PROJ_STICK_ON_WORLD 0x00000010
#define B3D_PROJ_OWNER_SAFE 0x00000020
#define B3D_PROJ_EMIT_MOVE 0x00000040
#define B3D_PROJ_DISABLE_ENTITY_HITS 0x00000080
#define B3D_PROJ_DISABLE_WORLD_HITS 0x00000100
#define B3D_PROJ_DIR_IS_NORMALIZED 0x00000200

typedef struct B3D_ProjectileDef {
    int kind;
    int flags;
    int hit_mask;
    int damage_type;
    B3D_Fixed radius;
    B3D_Fixed speed;
    B3D_Fixed damage;
    B3D_Fixed stun;
    B3D_Fixed knockback;
    B3D_Fixed lifetime;
    B3D_Fixed gravity;
    B3D_Fixed bounce;
    B3D_Fixed max_distance;
    B3D_Fixed owner_safe_time;
    B3D_Fixed explosion_radius;
    B3D_Fixed explosion_inner_radius;
    B3D_Fixed explosion_force;
    int max_hits;
    int pierce_count;
    void *user_ptr;
} B3D_ProjectileDef;

typedef struct B3D_Projectile {
    int active;
    int id;
    int slot;
    int kind;
    int flags;
    int owner_id;
    int hit_mask;
    int damage_type;
    int hit_count;
    int max_hits;
    int remaining_pierces;
    int last_hit_id;
    B3D_Vec3 position;
    B3D_Vec3 previous_position;
    B3D_Vec3 velocity;
    B3D_Vec3 acceleration;
    B3D_Fixed radius;
    B3D_Fixed damage;
    B3D_Fixed stun;
    B3D_Fixed knockback;
    B3D_Fixed age;
    B3D_Fixed lifetime;
    B3D_Fixed gravity;
    B3D_Fixed bounce;
    B3D_Fixed distance_traveled;
    B3D_Fixed max_distance;
    B3D_Fixed owner_safe_time;
    B3D_Fixed explosion_radius;
    B3D_Fixed explosion_inner_radius;
    B3D_Fixed explosion_force;
    void *user_ptr;
} B3D_Projectile;

B3D_API void b3d_projectile_def_clear(B3D_ProjectileDef *def_value);
B3D_API void b3d_projectile_clear(B3D_Projectile *projectile);
B3D_API B3D_ProjectileDef b3d_projectile_def_bullet(void);
B3D_API B3D_ProjectileDef b3d_projectile_def_rocket(void);
B3D_API B3D_ProjectileDef b3d_projectile_def_grenade(void);
B3D_API B3D_ProjectileDef b3d_projectile_def_plasma(void);
B3D_API B3D_ProjectileDef b3d_projectile_def_melee_trace(void);
B3D_API B3D_ProjectileDef b3d_projectile_def_solver_object(void);

#ifdef __cplusplus
}
#endif

#endif
