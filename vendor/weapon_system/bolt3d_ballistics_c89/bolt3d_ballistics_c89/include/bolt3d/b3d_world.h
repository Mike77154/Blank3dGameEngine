#ifndef B3D_WORLD_H
#define B3D_WORLD_H

#include "bolt3d/b3d_projectile.h"
#include "bolt3d/b3d_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct B3D_TraceRequest {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int hit_mask;
    int flags;
    B3D_Fixed radius;
    B3D_Fixed age;
    B3D_Fixed owner_safe_time;
    B3D_Vec3 from;
    B3D_Vec3 to;
    int ignore_target_id;
    void *projectile_user_ptr;
} B3D_TraceRequest;

typedef struct B3D_FilterRequest {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int target_id;
    int target_slot;
    int target_layer;
    int target_user_type;
    int hit_kind;
    int hit_mask;
    void *projectile_user_ptr;
    void *target_user_ptr;
} B3D_FilterRequest;

typedef struct B3D_ExplosionRequest {
    int projectile_id;
    int owner_id;
    int target_id;
    int target_slot;
    int target_layer;
    int target_user_type;
    B3D_Vec3 origin;
    B3D_Vec3 target_center;
    B3D_Fixed radius;
    void *projectile_user_ptr;
    void *target_user_ptr;
} B3D_ExplosionRequest;

typedef int (*B3D_TraceWorldHook)(void *user_data, const B3D_TraceRequest *request, B3D_Hit *out_hit);
typedef int (*B3D_FilterHitHook)(void *user_data, const B3D_FilterRequest *request);
typedef int (*B3D_ExplosionFilterHook)(void *user_data, const B3D_ExplosionRequest *request);
typedef void (*B3D_EventHook)(void *user_data, const B3D_Event *event_value);

typedef struct B3D_Callbacks {
    void *user_data;
    B3D_TraceWorldHook trace_world;
    B3D_FilterHitHook filter_hit;
    B3D_ExplosionFilterHook filter_explosion;
    B3D_EventHook on_event;
    B3D_ReadTransformHook read_transform;
    B3D_WriteTransformHook write_transform;
    B3D_QueryCollisionHook query_collision;
    B3D_ResolveContactHook resolve_contact;
    B3D_QueryGravityHook query_gravity;
} B3D_Callbacks;

typedef struct B3D_World {
    B3D_Projectile *projectiles;
    int projectile_capacity;
    B3D_Event *events;
    int event_capacity;
    int event_head;
    int event_tail;
    int event_count;
    int event_dropped;
    B3D_Collider *colliders;
    int collider_capacity;
    int collider_count;
    B3D_SolverState *solvers;
    int solver_capacity;
    B3D_Contact *contact_arena;
    int contact_capacity;
    B3D_Callbacks callbacks;
    int next_projectile_id;
} B3D_World;

B3D_API void b3d_callbacks_clear(B3D_Callbacks *callbacks);
B3D_API void b3d_world_init(B3D_World *world, B3D_Projectile *projectile_buffer, int projectile_capacity, B3D_Event *event_buffer, int event_capacity, B3D_Collider *collider_buffer, int collider_capacity);
B3D_API void b3d_world_attach_solver_arena(B3D_World *world, B3D_SolverState *solver_buffer, int solver_capacity, B3D_Contact *contact_buffer, int contact_capacity);
B3D_API void b3d_world_set_callbacks(B3D_World *world, const B3D_Callbacks *callbacks);
B3D_API void b3d_world_clear_projectiles(B3D_World *world);
B3D_API void b3d_world_clear_colliders(B3D_World *world);
B3D_API int b3d_world_add_collider(B3D_World *world, int id, int layer_mask, B3D_Vec3 center, B3D_Fixed radius, int user_type, void *user_ptr);
B3D_API int b3d_world_remove_collider(B3D_World *world, int id);
B3D_API int b3d_world_update_collider(B3D_World *world, int id, B3D_Vec3 center, B3D_Fixed radius, int layer_mask);
B3D_API B3D_Collider *b3d_world_find_collider(B3D_World *world, int id);
B3D_API int b3d_world_poll_event(B3D_World *world, B3D_Event *out_event);
B3D_API int b3d_world_push_event(B3D_World *world, const B3D_Event *event_value);
B3D_API int b3d_spawn_projectile(B3D_World *world, const B3D_ProjectileDef *def_value, int owner_id, B3D_Vec3 position, B3D_Vec3 direction);
B3D_API int b3d_spawn_projectile_velocity(B3D_World *world, const B3D_ProjectileDef *def_value, int owner_id, B3D_Vec3 position, B3D_Vec3 velocity);
B3D_API int b3d_spawn_solver(B3D_World *world, const B3D_ProjectileDef *projectile_def, const B3D_SolverDef *solver_def, int owner_id, const B3D_Transform *transform_value, B3D_Vec3 velocity);
B3D_API int b3d_enable_projectile_solver(B3D_World *world, int projectile_id, const B3D_SolverDef *solver_def, const B3D_Transform *transform_value);
B3D_API int b3d_disable_projectile_solver(B3D_World *world, int projectile_id);
B3D_API B3D_SolverState *b3d_world_find_solver(B3D_World *world, int projectile_id);
B3D_API int b3d_solver_set_desired_transform(B3D_World *world, int projectile_id, const B3D_Transform *transform_value);
B3D_API int b3d_solver_get_transform(B3D_World *world, int projectile_id, B3D_Transform *out_transform);
B3D_API int b3d_destroy_projectile(B3D_World *world, int projectile_id);
B3D_API B3D_Projectile *b3d_world_find_projectile(B3D_World *world, int projectile_id);
B3D_API void b3d_world_update(B3D_World *world, B3D_Fixed dt);
B3D_API int b3d_trace_segment(B3D_World *world, const B3D_TraceRequest *request, B3D_Hit *out_hit);
B3D_API void b3d_world_explode_projectile(B3D_World *world, B3D_Projectile *projectile, B3D_Vec3 origin);
B3D_API int b3d_fire_hitscan(B3D_World *world, int owner_id, B3D_Vec3 origin, B3D_Vec3 direction, B3D_Fixed range, B3D_Fixed damage, int damage_type, int hit_mask, B3D_Hit *out_hit);
B3D_API int b3d_sweep_melee(B3D_World *world, int owner_id, B3D_Vec3 from, B3D_Vec3 to, B3D_Fixed radius, B3D_Fixed damage, int damage_type, int hit_mask, B3D_Hit *out_hit);

#ifdef __cplusplus
}
#endif

#endif
