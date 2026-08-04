#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define TEST_PROJECTILES 8
#define TEST_EVENTS 128
#define TEST_COLLIDERS 8
#define TEST_CONTACTS 8

static B3D_Projectile g_projectiles[TEST_PROJECTILES];
static B3D_Event g_events[TEST_EVENTS];
static B3D_Collider g_colliders[TEST_COLLIDERS];
static B3D_SolverState g_solvers[TEST_PROJECTILES];
static B3D_Contact g_contacts[TEST_CONTACTS];
static int g_source_tag;
static int g_target_tag;

typedef struct TestTransformHost {
    B3D_Transform desired;
    B3D_Transform written;
    int write_count;
} TestTransformHost;

typedef struct TestGravityHost {
    B3D_Vec3 gravity;
    B3D_Vec3 fallback_seen;
    int accept;
    int call_count;
    int binding_id_seen;
    void *binding_ptr_seen;
} TestGravityHost;

static int plane_query(void *user_data, const B3D_SolverQuery *query, B3D_Contact *contacts, int contact_capacity)
{
    B3D_Fixed plane_x;
    B3D_Fixed dx;
    B3D_Fixed t;

    (void)user_data;
    if (contacts == 0 || contact_capacity <= 0) {
        return 0;
    }
    if (query->ignore_target_id == 77) {
        return 0;
    }

    plane_x = b3d_fixed_from_int(5);
    dx = b3d_fixed_sub_sat(query->to.position.x, query->from.position.x);
    if (dx <= 0 || query->from.position.x >= plane_x || query->to.position.x < plane_x) {
        return 0;
    }

    t = b3d_fixed_div(b3d_fixed_sub_sat(plane_x, query->from.position.x), dx);
    b3d_contact_clear(&contacts[0]);
    contacts[0].hit = B3D_TRUE;
    contacts[0].hit_kind = B3D_HIT_WORLD;
    contacts[0].target_id = 77;
    contacts[0].target_layer = B3D_LAYER_WORLD;
    contacts[0].target_user_type = 900;
    contacts[0].t = t;
    contacts[0].point = b3d_vec3_lerp(query->from.position, query->to.position, t);
    contacts[0].normal = b3d_vec3(b3d_fixed_neg(B3D_FIXED_ONE), 0, 0);
    return 1;
}

static int read_transform(void *user_data, const B3D_TransformRequest *request, B3D_Transform *out_transform)
{
    TestTransformHost *host;

    (void)request;
    host = (TestTransformHost *)user_data;
    *out_transform = host->desired;
    return B3D_TRUE;
}

static int write_transform(void *user_data, const B3D_TransformRequest *request, const B3D_Transform *transform_value)
{
    TestTransformHost *host;

    (void)request;
    host = (TestTransformHost *)user_data;
    host->written = *transform_value;
    host->write_count += 1;
    return B3D_TRUE;
}

static int query_gravity(void *user_data, const B3D_SolverGravityRequest *request, B3D_Vec3 *out_gravity)
{
    TestGravityHost *host;

    host = (TestGravityHost *)user_data;
    host->call_count += 1;
    host->fallback_seen = request->fallback_gravity;
    host->binding_id_seen = request->binding_id;
    host->binding_ptr_seen = request->binding_ptr;
    *out_gravity = host->gravity;
    return host->accept;
}

static void init_world(B3D_World *world)
{
    b3d_world_init(world, g_projectiles, TEST_PROJECTILES, g_events, TEST_EVENTS, g_colliders, TEST_COLLIDERS);
    b3d_world_attach_solver_arena(world, g_solvers, TEST_PROJECTILES, g_contacts, TEST_CONTACTS);
}

static int test_blocking(void)
{
    B3D_World world;
    B3D_ProjectileDef projectile_def;
    B3D_SolverDef solver_def;
    B3D_Transform transform_value;
    B3D_Projectile *projectile;
    B3D_Event event_value;
    int projectile_id;
    int saw_contact;
    int saw_blocked;
    int saw_damage;
    int pointers_ok;

    init_world(&world);
    b3d_world_add_collider(
        &world,
        44,
        B3D_LAYER_ENEMY,
        b3d_vec3(b3d_fixed_from_int(5), 0, 0),
        b3d_fixed_from_int(1),
        321,
        &g_target_tag
    );

    b3d_projectile_def_clear(&projectile_def);
    projectile_def.hit_mask = B3D_LAYER_ENEMY;
    projectile_def.radius = b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(2));
    projectile_def.damage = b3d_fixed_from_int(7);
    projectile_def.lifetime = 0;
    projectile_def.max_hits = 1;
    projectile_def.user_ptr = &g_source_tag;

    b3d_solver_def_clear(&solver_def);
    solver_def.default_response = B3D_RESPONSE_BLOCK;
    transform_value = b3d_transform_identity();
    projectile_id = b3d_spawn_solver(
        &world,
        &projectile_def,
        &solver_def,
        1,
        &transform_value,
        b3d_vec3(b3d_fixed_from_int(10), 0, 0)
    );
    if (projectile_id == B3D_ID_NONE) {
        printf("FAIL solver spawn\n");
        return 0;
    }

    b3d_world_update(&world, B3D_FIXED_ONE);
    projectile = b3d_world_find_projectile(&world, projectile_id);
    if (projectile == 0 || projectile->position.x <= b3d_fixed_from_int(3) || projectile->position.x >= b3d_fixed_from_int(4)) {
        printf("FAIL solver block position\n");
        return 0;
    }
    if (projectile->velocity.x != 0) {
        printf("FAIL solver block velocity\n");
        return 0;
    }

    saw_contact = 0;
    saw_blocked = 0;
    saw_damage = 0;
    pointers_ok = 0;
    while (b3d_world_poll_event(&world, &event_value)) {
        if (event_value.type == B3D_EVENT_CONTACT) {
            saw_contact = 1;
            if (event_value.source_user_ptr == &g_source_tag && event_value.target_user_ptr == &g_target_tag && event_value.target_user_type == 321) {
                pointers_ok = 1;
            }
        }
        if (event_value.type == B3D_EVENT_BLOCKED) {
            saw_blocked = 1;
        }
        if (event_value.type == B3D_EVENT_DAMAGE && event_value.target_id == 44) {
            saw_damage = 1;
        }
    }
    if (!saw_contact || !saw_blocked || !saw_damage || !pointers_ok) {
        printf("FAIL solver block events %d %d %d %d\n", saw_contact, saw_blocked, saw_damage, pointers_ok);
        return 0;
    }
    return 1;
}

static int test_slide_contract(void)
{
    B3D_World world;
    B3D_Callbacks callbacks;
    B3D_ProjectileDef projectile_def;
    B3D_SolverDef solver_def;
    B3D_Transform transform_value;
    B3D_Projectile *projectile;
    B3D_Event event_value;
    int projectile_id;
    int saw_slide;

    init_world(&world);
    b3d_callbacks_clear(&callbacks);
    callbacks.query_collision = plane_query;
    b3d_world_set_callbacks(&world, &callbacks);

    b3d_projectile_def_clear(&projectile_def);
    projectile_def.hit_mask = B3D_LAYER_WORLD;
    projectile_def.lifetime = 0;
    projectile_def.max_hits = 0;

    b3d_solver_def_clear(&solver_def);
    solver_def.default_response = B3D_RESPONSE_SLIDE;
    solver_def.flags |= B3D_SOLVER_DISABLE_BUILTIN_QUERY;
    transform_value = b3d_transform_identity();
    projectile_id = b3d_spawn_solver(
        &world,
        &projectile_def,
        &solver_def,
        1,
        &transform_value,
        b3d_vec3(b3d_fixed_from_int(10), b3d_fixed_from_int(10), 0)
    );

    b3d_world_update(&world, B3D_FIXED_ONE);
    projectile = b3d_world_find_projectile(&world, projectile_id);
    if (projectile == 0 || projectile->position.x < b3d_fixed_from_int(4) || projectile->position.x > b3d_fixed_from_int(5) || projectile->position.y < b3d_fixed_from_int(9)) {
        printf("FAIL solver slide position %ld %ld\n", projectile != 0 ? projectile->position.x : 0, projectile != 0 ? projectile->position.y : 0);
        return 0;
    }
    if (projectile->velocity.x != 0 || projectile->velocity.y <= 0) {
        printf("FAIL solver slide velocity %ld %ld\n", projectile->velocity.x, projectile->velocity.y);
        return 0;
    }

    saw_slide = 0;
    while (b3d_world_poll_event(&world, &event_value)) {
        if (event_value.type == B3D_EVENT_SLIDE && event_value.target_id == 77) {
            saw_slide = 1;
        }
    }
    if (!saw_slide) {
        printf("FAIL solver slide event\n");
        return 0;
    }
    return 1;
}

static int test_external_transform(void)
{
    B3D_World world;
    B3D_Callbacks callbacks;
    B3D_ProjectileDef projectile_def;
    B3D_SolverDef solver_def;
    B3D_Transform transform_value;
    TestTransformHost host;
    B3D_Event event_value;
    int projectile_id;
    int saw_read;
    int saw_write;

    init_world(&world);
    host.desired = b3d_transform_from_position(b3d_vec3(b3d_fixed_from_int(7), b3d_fixed_from_int(3), 0));
    host.written = b3d_transform_identity();
    host.write_count = 0;

    b3d_callbacks_clear(&callbacks);
    callbacks.user_data = &host;
    callbacks.read_transform = read_transform;
    callbacks.write_transform = write_transform;
    b3d_world_set_callbacks(&world, &callbacks);

    b3d_projectile_def_clear(&projectile_def);
    projectile_def.lifetime = 0;
    projectile_def.max_hits = 0;

    b3d_solver_def_clear(&solver_def);
    solver_def.motion_mode = B3D_MOTION_EXTERNAL;
    solver_def.flags |= B3D_SOLVER_READ_TRANSFORM | B3D_SOLVER_WRITE_TRANSFORM | B3D_SOLVER_DISABLE_BUILTIN_QUERY | B3D_SOLVER_DISABLE_CONTRACT_QUERY;
    transform_value = b3d_transform_identity();
    projectile_id = b3d_spawn_solver(&world, &projectile_def, &solver_def, 1, &transform_value, b3d_vec3_zero());
    b3d_world_update(&world, B3D_FIXED_ONE);

    if (host.write_count != 1 || host.written.position.x != b3d_fixed_from_int(7) || host.written.position.y != b3d_fixed_from_int(3)) {
        printf("FAIL external transform write %d %ld %ld\n", host.write_count, host.written.position.x, host.written.position.y);
        return 0;
    }

    saw_read = 0;
    saw_write = 0;
    while (b3d_world_poll_event(&world, &event_value)) {
        if (event_value.type == B3D_EVENT_TRANSFORM_READ) {
            saw_read = 1;
        }
        if (event_value.type == B3D_EVENT_TRANSFORM_WRITE) {
            saw_write = 1;
        }
    }
    if (!saw_read || !saw_write || projectile_id == B3D_ID_NONE) {
        printf("FAIL external transform events %d %d\n", saw_read, saw_write);
        return 0;
    }
    return 1;
}

static int test_solver_gravity_hook(void)
{
    B3D_World world;
    B3D_Callbacks callbacks;
    B3D_ProjectileDef projectile_def;
    B3D_SolverDef solver_def;
    B3D_Transform transform_value;
    B3D_Projectile *projectile;
    TestGravityHost host;
    int projectile_id;

    init_world(&world);
    host.gravity = b3d_vec3(b3d_fixed_from_int(2), b3d_fixed_from_int(3), b3d_fixed_from_int(-4));
    host.fallback_seen = b3d_vec3_zero();
    host.accept = B3D_TRUE;
    host.call_count = 0;
    host.binding_id_seen = 0;
    host.binding_ptr_seen = 0;

    b3d_callbacks_clear(&callbacks);
    callbacks.user_data = &host;
    callbacks.query_gravity = query_gravity;
    b3d_world_set_callbacks(&world, &callbacks);

    b3d_projectile_def_clear(&projectile_def);
    projectile_def.flags = B3D_PROJ_USE_GRAVITY;
    projectile_def.gravity = b3d_fixed_from_int(10);
    projectile_def.lifetime = 0;
    projectile_def.max_hits = 0;

    b3d_solver_def_clear(&solver_def);
    solver_def.binding_id = 123;
    solver_def.binding_ptr = &g_source_tag;
    solver_def.flags |= B3D_SOLVER_DISABLE_BUILTIN_QUERY | B3D_SOLVER_DISABLE_CONTRACT_QUERY;
    transform_value = b3d_transform_identity();

    projectile_id = b3d_spawn_solver(&world, &projectile_def, &solver_def, 1, &transform_value, b3d_vec3_zero());
    b3d_world_update(&world, B3D_FIXED_ONE);
    projectile = b3d_world_find_projectile(&world, projectile_id);
    if (projectile == 0 || host.call_count != 1) {
        printf("FAIL solver gravity hook call %d\n", host.call_count);
        return 0;
    }
    if (projectile->velocity.x != b3d_fixed_from_int(2) || projectile->velocity.y != b3d_fixed_from_int(3) || projectile->velocity.z != b3d_fixed_from_int(-4)) {
        printf("FAIL solver gravity hook velocity %ld %ld %ld\n", projectile->velocity.x, projectile->velocity.y, projectile->velocity.z);
        return 0;
    }
    if (projectile->position.x != b3d_fixed_from_int(2) || projectile->position.y != b3d_fixed_from_int(3) || projectile->position.z != b3d_fixed_from_int(-4)) {
        printf("FAIL solver gravity hook position %ld %ld %ld\n", projectile->position.x, projectile->position.y, projectile->position.z);
        return 0;
    }
    if (host.fallback_seen.x != 0 || host.fallback_seen.y != b3d_fixed_from_int(-10) || host.fallback_seen.z != 0 || host.binding_id_seen != 123 || host.binding_ptr_seen != &g_source_tag) {
        printf("FAIL solver gravity hook request\n");
        return 0;
    }

    init_world(&world);
    host.gravity = b3d_vec3(b3d_fixed_from_int(99), b3d_fixed_from_int(99), b3d_fixed_from_int(99));
    host.fallback_seen = b3d_vec3_zero();
    host.accept = B3D_FALSE;
    host.call_count = 0;
    host.binding_id_seen = 0;
    host.binding_ptr_seen = 0;
    b3d_callbacks_clear(&callbacks);
    callbacks.user_data = &host;
    callbacks.query_gravity = query_gravity;
    b3d_world_set_callbacks(&world, &callbacks);

    projectile_id = b3d_spawn_solver(&world, &projectile_def, &solver_def, 1, &transform_value, b3d_vec3_zero());
    b3d_world_update(&world, B3D_FIXED_ONE);
    projectile = b3d_world_find_projectile(&world, projectile_id);
    if (projectile == 0 || projectile->velocity.x != 0 || projectile->velocity.y != b3d_fixed_from_int(-10) || projectile->velocity.z != 0) {
        printf("FAIL solver gravity fallback %ld %ld %ld\n", projectile != 0 ? projectile->velocity.x : 0, projectile != 0 ? projectile->velocity.y : 0, projectile != 0 ? projectile->velocity.z : 0);
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_blocking()) {
        return 1;
    }
    if (!test_slide_contract()) {
        return 1;
    }
    if (!test_external_transform()) {
        return 1;
    }
    if (!test_solver_gravity_hook()) {
        return 1;
    }
    printf("test_solver OK\n");
    return 0;
}
