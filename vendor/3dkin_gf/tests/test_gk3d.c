#include <stdio.h>
#include "gk3d.h"

static int failures = 0;

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("FAIL line %d: %s\n", __LINE__, #expr); \
        ++failures; \
    } \
} while (0)

static int reject_precise(const gk3d_world *world,
    int moving_id, const gk3d_aabb *moving_box,
    int other_id, void *user)
{
    int *calls;
    (void)world;
    (void)moving_id;
    (void)moving_box;
    (void)other_id;
    calls = (int *)user;
    ++*calls;
    return GK3D_NARROWPHASE_NO;
}

static int unhandled_precise(const gk3d_world *world,
    int moving_id, const gk3d_aabb *moving_box,
    int other_id, void *user)
{
    (void)world;
    (void)moving_id;
    (void)moving_box;
    (void)other_id;
    (void)user;
    return GK3D_NARROWPHASE_UNHANDLED;
}

static void test_conditions(void)
{
    gk3d_world world;
    int player;
    int floor_id;
    int wall_id;
    int trigger_id;

    gk3d_world_init(&world);
    floor_id = gk3d_world_add_box(&world, 2,
        GK3D_FROM_INT(-10), GK3D_FROM_INT(0), GK3D_FROM_INT(-10),
        GK3D_FROM_INT(20), GK3D_FROM_INT(1), GK3D_FROM_INT(20),
        GK3D_FLAG_SOLID);
    wall_id = gk3d_world_add_box(&world, 2,
        GK3D_FROM_INT(2), GK3D_FROM_INT(1), GK3D_FROM_INT(-2),
        GK3D_FROM_INT(1), GK3D_FROM_INT(4), GK3D_FROM_INT(4),
        GK3D_FLAG_SOLID);
    trigger_id = gk3d_world_add_box(&world, 3,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(3),
        GK3D_FROM_INT(1), GK3D_FROM_INT(2), GK3D_FROM_INT(1),
        GK3D_FLAG_TRIGGER);
    player = gk3d_world_add_box(&world, 1,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(0),
        GK3D_FROM_INT(1), GK3D_FROM_INT(2), GK3D_FROM_INT(1),
        0u);

    CHECK(player != GK3D_ID_NONE);
    CHECK(floor_id != GK3D_ID_NONE);
    CHECK(wall_id != GK3D_ID_NONE);
    CHECK(trigger_id != GK3D_ID_NONE);
    CHECK(gk3d_is_on_floor(&world, player));
    CHECK(!gk3d_is_on_wall(&world, player));
    CHECK(gk3d_place_meeting(&world, player,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(3), 3));
    CHECK(gk3d_instance_place(&world, player,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(3), 3) == trigger_id);
    CHECK(gk3d_place_free(&world, player,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(3)));

    gk3d_obj_set_pos(&world, player,
        GK3D_FROM_INT(1), GK3D_FROM_INT(1), GK3D_FROM_INT(0));
    CHECK(gk3d_is_on_wall(&world, player));
}

static void test_jumpthru_floor(void)
{
    gk3d_world world;
    int actor;

    gk3d_world_init(&world);
    gk3d_world_add_box(&world, 8,
        -GK3D_ONE, 0, -GK3D_ONE,
        GK3D_FROM_INT(3), GK3D_ONE, GK3D_FROM_INT(3),
        GK3D_FLAG_JUMPTHRU);
    actor = gk3d_world_add_box(&world, 1,
        0, GK3D_ONE, 0,
        GK3D_ONE, GK3D_FROM_INT(2), GK3D_ONE, 0u);

    CHECK(gk3d_is_on_floor(&world, actor));
    CHECK(!gk3d_is_on_wall(&world, actor));
    CHECK(gk3d_place_free(&world, actor, 0, 0, 0));
}

static void test_narrowphase(void)
{
    gk3d_world world;
    int actor;
    int calls;

    calls = 0;
    gk3d_world_init(&world);
    gk3d_world_set_narrowphase(&world, reject_precise, &calls);
    actor = gk3d_world_add_box(&world, 1,
        0, 0, 0, GK3D_ONE, GK3D_ONE, GK3D_ONE,
        GK3D_FLAG_PRECISE);
    gk3d_world_add_box(&world, 2,
        0, 0, 0, GK3D_ONE, GK3D_ONE, GK3D_ONE,
        GK3D_FLAG_SOLID | GK3D_FLAG_PRECISE);

    CHECK(!gk3d_place_meeting(&world, actor, 0, 0, 0, GK3D_TARGET_SOLID));
    CHECK(calls == 1);
    CHECK(world.stats.narrowphase_calls == 1);
}

static void test_verb(void)
{
    gk3d_world world;
    gk3d_condition_result result;
    int actor;

    gk3d_world_init(&world);
    gk3d_world_add_box(&world, 2,
        -GK3D_ONE, 0, -GK3D_ONE,
        GK3D_FROM_INT(3), GK3D_ONE, GK3D_FROM_INT(3),
        GK3D_FLAG_SOLID);
    actor = gk3d_world_add_box(&world, 1,
        0, GK3D_ONE, 0,
        GK3D_ONE, GK3D_FROM_INT(2), GK3D_ONE, 0u);

    CHECK(gk3d_verb_from_name("isonfloor") == GK3D_VERB_IS_ON_FLOOR);
    CHECK(gk3d_check_verb(&world, "isOnFloor", actor,
        GK3D_TARGET_SOLID, 0, 0, 0, 0, 0, 0, &result));
    CHECK(result.truth);
    CHECK(result.instance_id != GK3D_ID_NONE);
}

static void test_up_axis_and_filters(void)
{
    gk3d_world world;
    gk3d_filter filter;
    int actor;
    int floor_id;
    int other_id;
    int found;

    gk3d_world_init(&world);
    gk3d_world_set_up(&world, GK3D_AXIS_Z, 1);
    floor_id = gk3d_world_add_box(&world, 9,
        -GK3D_ONE, -GK3D_ONE, 0,
        GK3D_FROM_INT(4), GK3D_FROM_INT(4), GK3D_ONE,
        GK3D_FLAG_SOLID);
    other_id = gk3d_world_add_box(&world, 9,
        GK3D_FROM_INT(5), 0, GK3D_ONE,
        GK3D_ONE, GK3D_ONE, GK3D_ONE,
        GK3D_FLAG_SOLID);
    actor = gk3d_world_add_box(&world, 1,
        0, 0, GK3D_ONE,
        GK3D_ONE, GK3D_ONE, GK3D_FROM_INT(2), 0u);

    CHECK(gk3d_floor_instance(&world, actor, GK3D_TARGET_SOLID) == floor_id);
    CHECK(gk3d_is_on_floor(&world, actor));
    CHECK(!gk3d_is_on_wall_axis(&world, actor, GK3D_AXIS_Z, -1,
        GK3D_TARGET_SOLID));

    filter = gk3d_filter_make(GK3D_TARGET_ANY);
    filter.exact_id = other_id;
    found = gk3d_instance_place_box_filter(&world, actor,
        gk3d_aabb_at(gk3d_world_get(&world, actor)->box,
            GK3D_FROM_INT(5), 0, GK3D_ONE), &filter);
    CHECK(found == other_id);
    CHECK(!gk3d_place_meeting(&world, actor, 0, 0, GK3D_ONE, -9999));
}

static void test_unhandled_fallback(void)
{
    gk3d_world world;
    int actor;

    gk3d_world_init(&world);
    gk3d_world_set_narrowphase(&world, unhandled_precise, 0);
    actor = gk3d_world_add_box(&world, 1,
        0, 0, 0, GK3D_ONE, GK3D_ONE, GK3D_ONE,
        GK3D_FLAG_PRECISE);
    gk3d_world_add_box(&world, 2,
        0, 0, 0, GK3D_ONE, GK3D_ONE, GK3D_ONE,
        GK3D_FLAG_SOLID | GK3D_FLAG_PRECISE);

    CHECK(gk3d_place_meeting(&world, actor, 0, 0, 0,
        GK3D_TARGET_SOLID));
    CHECK(world.stats.aabb_fallbacks == 1);
}

static void test_grid_equivalence(void)
{
    gk3d_world world;
    const gk3d_obj *actor_obj;
    const gk3d_obj *solid_obj;
    gk3d_aabb candidate;
    int actor;
    int solid;
    int x;
    int y;
    int z;
    int expected;
    int actual;

    gk3d_world_init(&world);
    actor = gk3d_world_add_box(&world, 1,
        0, 0, 0, GK3D_ONE, GK3D_FROM_INT(2), GK3D_ONE, 0u);
    solid = gk3d_world_add_box(&world, 2,
        GK3D_FROM_INT(2), GK3D_FROM_INT(1), GK3D_FROM_INT(2),
        GK3D_FROM_INT(2), GK3D_FROM_INT(2), GK3D_FROM_INT(2),
        GK3D_FLAG_SOLID);
    actor_obj = gk3d_world_get_const(&world, actor);
    solid_obj = gk3d_world_get_const(&world, solid);

    for (x = -1; x <= 5; ++x) {
        for (y = -1; y <= 4; ++y) {
            for (z = -1; z <= 5; ++z) {
                candidate = gk3d_aabb_at(actor_obj->box,
                    GK3D_FROM_INT(x), GK3D_FROM_INT(y), GK3D_FROM_INT(z));
                expected = gk3d_aabb_overlap(candidate, solid_obj->box);
                actual = gk3d_place_meeting(&world, actor,
                    candidate.x, candidate.y, candidate.z,
                    GK3D_TARGET_SOLID);
                CHECK(actual == expected);
            }
        }
    }
}

static void test_solver(void)
{
    gk3d_world world;
    gk3d_solve_result result;
    int actor;

    gk3d_world_init(&world);
    actor = gk3d_world_add_box(&world, 1,
        0, 0, 0, GK3D_ONE, GK3D_ONE, GK3D_ONE, 0u);
    gk3d_world_add_box(&world, 2,
        GK3D_FROM_INT(3), 0, 0,
        GK3D_ONE, GK3D_ONE, GK3D_ONE, GK3D_FLAG_SOLID);

    CHECK(gk3d_solve_move(&world, actor,
        GK3D_FROM_INT(5), 0, 0, GK3D_TARGET_SOLID, &result));
    CHECK(result.collided);
    CHECK(result.hit_x_id != GK3D_ID_NONE);
    CHECK(result.allowed_dx < GK3D_FROM_INT(3));
    CHECK(result.allowed_dx >= GK3D_FROM_INT(1));
    CHECK(result.nx == -GK3D_ONE);
    CHECK(gk3d_apply_solved_move(&world, actor, &result));
    CHECK(gk3d_world_get(&world, actor)->box.x == result.allowed_dx);
    CHECK(gk3d_place_free(&world, actor,
        gk3d_world_get(&world, actor)->box.x,
        gk3d_world_get(&world, actor)->box.y,
        gk3d_world_get(&world, actor)->box.z));
}

int main(void)
{
    test_conditions();
    test_jumpthru_floor();
    test_narrowphase();
    test_unhandled_fallback();
    test_up_axis_and_filters();
    test_grid_equivalence();
    test_verb();
    test_solver();

    if (failures != 0) {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
