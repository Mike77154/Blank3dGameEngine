#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define DEMO_PROJECTILES 4
#define DEMO_EVENTS 64
#define DEMO_COLLIDERS 4
#define DEMO_CONTACTS 8

static B3D_Projectile projectiles[DEMO_PROJECTILES];
static B3D_Event events[DEMO_EVENTS];
static B3D_Collider colliders[DEMO_COLLIDERS];
static B3D_SolverState solvers[DEMO_PROJECTILES];
static B3D_Contact contacts[DEMO_CONTACTS];

static int custom_gravity(void *user_data, const B3D_SolverGravityRequest *request, B3D_Vec3 *out_gravity)
{
    (void)user_data;
    (void)request;
    *out_gravity = b3d_vec3(0, b3d_fixed_from_int(-3), b3d_fixed_from_int(1));
    return B3D_TRUE;
}

int main(void)
{
    B3D_World world;
    B3D_ProjectileDef projectile_def;
    B3D_SolverDef solver_def;
    B3D_Callbacks callbacks;
    B3D_Transform transform_value;
    B3D_Event event_value;
    int projectile_id;

    b3d_world_init(&world, projectiles, DEMO_PROJECTILES, events, DEMO_EVENTS, colliders, DEMO_COLLIDERS);
    b3d_world_attach_solver_arena(&world, solvers, DEMO_PROJECTILES, contacts, DEMO_CONTACTS);
    b3d_callbacks_clear(&callbacks);
    callbacks.query_gravity = custom_gravity;
    b3d_world_set_callbacks(&world, &callbacks);
    b3d_world_add_collider(
        &world,
        100,
        B3D_LAYER_WORLD,
        b3d_vec3(b3d_fixed_from_int(6), 0, 0),
        b3d_fixed_from_int(1),
        0,
        0
    );

    projectile_def = b3d_projectile_def_solver_object();
    projectile_def.hit_mask = B3D_LAYER_WORLD;
    projectile_def.flags |= B3D_PROJ_USE_GRAVITY;
    projectile_def.gravity = b3d_fixed_from_int(12);

    b3d_solver_def_clear(&solver_def);
    solver_def.default_response = B3D_RESPONSE_SLIDE;
    solver_def.flags |= B3D_SOLVER_ALIGN_TO_VELOCITY;

    transform_value = b3d_transform_identity();
    projectile_id = b3d_spawn_solver(
        &world,
        &projectile_def,
        &solver_def,
        1,
        &transform_value,
        b3d_vec3(b3d_fixed_from_int(10), b3d_fixed_from_int(2), 0)
    );

    b3d_world_update(&world, B3D_FIXED_ONE);
    while (b3d_world_poll_event(&world, &event_value)) {
        if (event_value.type == B3D_EVENT_CONTACT) {
            printf("contact projectile=%d target=%d response=%d\n", projectile_id, event_value.target_id, event_value.response);
        }
        if (event_value.type == B3D_EVENT_TRANSFORM_SOLVED) {
            printf("solved x=%ld y=%ld z=%ld\n", event_value.solved_transform.position.x, event_value.solved_transform.position.y, event_value.solved_transform.position.z);
        }
    }
    return 0;
}
