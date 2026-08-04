#ifndef B3D_SOLVER_H
#define B3D_SOLVER_H

#include "bolt3d/b3d_transform.h"
#include "bolt3d/b3d_collision.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_MOTION_BALLISTIC 0
#define B3D_MOTION_SOLVER 1
#define B3D_MOTION_EXTERNAL 2

#define B3D_SHAPE_POINT 0
#define B3D_SHAPE_SPHERE 1
#define B3D_SHAPE_CAPSULE 2
#define B3D_SHAPE_AABB 3
#define B3D_SHAPE_OBB 4
#define B3D_SHAPE_CUSTOM 1000

#define B3D_RESPONSE_DEFAULT (-1)
#define B3D_RESPONSE_IGNORE 0
#define B3D_RESPONSE_BLOCK 1
#define B3D_RESPONSE_SLIDE 2
#define B3D_RESPONSE_BOUNCE 3
#define B3D_RESPONSE_STICK 4
#define B3D_RESPONSE_PIERCE 5
#define B3D_RESPONSE_OVERLAP 6
#define B3D_RESPONSE_CUSTOM 1000

#define B3D_SOLVER_READ_TRANSFORM 0x00000001
#define B3D_SOLVER_WRITE_TRANSFORM 0x00000002
#define B3D_SOLVER_ALIGN_TO_VELOCITY 0x00000004
#define B3D_SOLVER_USE_ANGULAR_VELOCITY 0x00000008
#define B3D_SOLVER_EMIT_TRANSFORM_EVENTS 0x00000010
#define B3D_SOLVER_KEEP_ACTIVE_ON_CONTACT 0x00000020
#define B3D_SOLVER_DISABLE_BUILTIN_QUERY 0x00000040
#define B3D_SOLVER_DISABLE_CONTRACT_QUERY 0x00000080

#define B3D_CONTACT_TRIGGER 0x00000001
#define B3D_CONTACT_START_INSIDE 0x00000002
#define B3D_CONTACT_EXTERNAL 0x00000004

#define B3D_SOLVER_DEFAULT_ITERATIONS 4
#define B3D_SOLVER_MAX_ITERATIONS 16

typedef struct B3D_Shape {
    int kind;
    int user_type;
    B3D_Fixed radius;
    B3D_Fixed half_height;
    B3D_Vec3 half_extents;
    void *user_ptr;
} B3D_Shape;

typedef struct B3D_Contact {
    int hit;
    int hit_kind;
    int target_id;
    int target_slot;
    int target_layer;
    int target_user_type;
    int flags;
    int response;
    B3D_Fixed t;
    B3D_Vec3 point;
    B3D_Vec3 normal;
    void *target_user_ptr;
} B3D_Contact;

typedef struct B3D_TransformRequest {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int binding_id;
    int motion_mode;
    int flags;
    void *projectile_user_ptr;
    void *binding_ptr;
} B3D_TransformRequest;

typedef struct B3D_SolverQuery {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int binding_id;
    int hit_mask;
    int projectile_flags;
    int solver_flags;
    int ignore_target_id;
    B3D_Fixed age;
    B3D_Fixed owner_safe_time;
    B3D_Transform from;
    B3D_Transform to;
    B3D_Shape shape;
    void *projectile_user_ptr;
    void *binding_ptr;
} B3D_SolverQuery;

typedef struct B3D_SolverResponseRequest {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int binding_id;
    int iteration;
    int default_response;
    B3D_Contact contact;
    B3D_Vec3 velocity;
    B3D_Vec3 remaining_delta;
    void *projectile_user_ptr;
    void *binding_ptr;
} B3D_SolverResponseRequest;

typedef struct B3D_SolverGravityRequest {
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int binding_id;
    int projectile_flags;
    int solver_flags;
    B3D_Fixed age;
    B3D_Fixed dt;
    B3D_Transform transform;
    B3D_Vec3 velocity;
    B3D_Vec3 acceleration;
    B3D_Vec3 fallback_gravity;
    void *projectile_user_ptr;
    void *binding_ptr;
} B3D_SolverGravityRequest;

typedef int (*B3D_ReadTransformHook)(void *user_data, const B3D_TransformRequest *request, B3D_Transform *out_transform);
typedef int (*B3D_WriteTransformHook)(void *user_data, const B3D_TransformRequest *request, const B3D_Transform *transform_value);
typedef int (*B3D_QueryCollisionHook)(void *user_data, const B3D_SolverQuery *query, B3D_Contact *contacts, int contact_capacity);
typedef int (*B3D_ResolveContactHook)(void *user_data, const B3D_SolverResponseRequest *request);
typedef int (*B3D_QueryGravityHook)(void *user_data, const B3D_SolverGravityRequest *request, B3D_Vec3 *out_gravity);

typedef struct B3D_SolverDef {
    int motion_mode;
    int flags;
    int default_response;
    int max_iterations;
    int binding_id;
    B3D_Shape shape;
    B3D_Fixed skin;
    B3D_Fixed friction;
    B3D_Fixed restitution;
    void *binding_ptr;
} B3D_SolverDef;

typedef struct B3D_SolverState {
    int active;
    int projectile_id;
    int projectile_slot;
    int motion_mode;
    int flags;
    int default_response;
    int max_iterations;
    int binding_id;
    int last_target_id;
    int last_response;
    int iteration_count;
    B3D_Shape shape;
    B3D_Transform transform;
    B3D_Transform previous_transform;
    B3D_Transform desired_transform;
    B3D_Transform solved_transform;
    B3D_Vec3 angular_velocity;
    B3D_Fixed skin;
    B3D_Fixed friction;
    B3D_Fixed restitution;
    void *binding_ptr;
} B3D_SolverState;

struct B3D_World;
struct B3D_Projectile;

B3D_API void b3d_shape_clear(B3D_Shape *shape);
B3D_API B3D_Shape b3d_shape_sphere(B3D_Fixed radius);
B3D_API void b3d_contact_clear(B3D_Contact *contact);
B3D_API void b3d_solver_def_clear(B3D_SolverDef *def_value);
B3D_API void b3d_solver_state_clear(B3D_SolverState *solver);
B3D_API int b3d_solver_response_is_valid(int response);
B3D_API int b3d_solver_update_projectile(struct B3D_World *world, struct B3D_Projectile *projectile, B3D_SolverState *solver, B3D_Fixed dt);

#ifdef __cplusplus
}
#endif

#endif
