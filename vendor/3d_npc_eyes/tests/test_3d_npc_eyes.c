#include <assert.h>
#include <stdio.h>
#include "3d_npc_eyes.h"

static int clear_world(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
)
{
    (void)world_user;
    (void)from;
    (void)to;
    (void)block_mask;
    if (out_hit != 0) {
        out_hit->hit = TDNE_FALSE;
    }
    return TDNE_FALSE;
}

static int always_blocked_world(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
)
{
    (void)world_user;
    (void)block_mask;
    if (out_hit != 0) {
        out_hit->hit = TDNE_TRUE;
        out_hit->point = tdne_vec3_make(
            (from->x + to->x) / 2,
            (from->y + to->y) / 2,
            (from->z + to->z) / 2
        );
        out_hit->normal = tdne_vec3_zero();
        out_hit->material_mask = 1UL;
        out_hit->user = 0;
    }
    return TDNE_TRUE;
}

static void test_cone_visible(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 500, 90);
    tdne_target_init(&target, tdne_vec3_make(0, 0, 100), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_VISIBLE);
    assert(result.visible == TDNE_TRUE);
    assert(result.rays_clear == 1);
    assert(result.score > 0);
}

static void test_cone_out_of_shape(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 500, 60);
    tdne_target_init(&target, tdne_vec3_make(200, 0, 0), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_OUT_OF_SHAPE);
    assert(result.visible == TDNE_FALSE);
}

static void test_out_of_range(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 100, 90);
    tdne_target_init(&target, tdne_vec3_make(0, 0, 300), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_OUT_OF_RANGE);
    assert(result.visible == TDNE_FALSE);
}

static void test_occluded(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 500, 90);
    tdne_target_init(&target, tdne_vec3_make(0, 0, 100), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, always_blocked_world, 0, &result) == TDNE_VIS_OCCLUDED);
    assert(result.visible == TDNE_FALSE);
    assert(result.hit.hit == TDNE_TRUE);
}

static void test_sphere_can_see_behind(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_sphere(&eyes, tdne_vec3_zero(), 500);
    tdne_target_init(&target, tdne_vec3_make(0, 0, -100), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_VISIBLE);
    assert(result.visible == TDNE_TRUE);
}

static void test_box(void)
{
    tdne_sensor box;
    tdne_target inside;
    tdne_target outside;
    tdne_result result;

    tdne_sensor_init_box(&box, tdne_vec3_zero(), tdne_vec3_make(100, 50, 100));
    tdne_sensor_set_line_of_sight(&box, TDNE_FALSE);
    tdne_target_init(&inside, tdne_vec3_make(10, 0, 10), 1, 1UL, 0);
    tdne_target_init(&outside, tdne_vec3_make(200, 0, 0), 1, 1UL, 0);

    assert(tdne_eval_target(&box, &inside, 0, 0, &result) == TDNE_VIS_VISIBLE);
    assert(tdne_eval_target(&box, &outside, 0, 0, &result) == TDNE_VIS_OUT_OF_SHAPE);
}

static void test_frustum(void)
{
    tdne_sensor frustum;
    tdne_target inside;
    tdne_target outside;
    tdne_result result;

    tdne_sensor_init_frustum(
        &frustum,
        tdne_vec3_zero(),
        tdne_vec3_make(0, 0, 1),
        tdne_vec3_make(1, 0, 0),
        tdne_vec3_make(0, 1, 0),
        10,
        1000,
        90,
        60
    );
    tdne_sensor_set_line_of_sight(&frustum, TDNE_FALSE);

    tdne_target_init(&inside, tdne_vec3_make(100, 10, 300), 1, 1UL, 0);
    tdne_target_init(&outside, tdne_vec3_make(800, 0, 300), 1, 1UL, 0);

    assert(tdne_eval_target(&frustum, &inside, 0, 0, &result) == TDNE_VIS_VISIBLE);
    assert(tdne_eval_target(&frustum, &outside, 0, 0, &result) == TDNE_VIS_OUT_OF_SHAPE);
}

static void test_mask(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_sphere(&eyes, tdne_vec3_zero(), 500);
    tdne_sensor_set_masks(&eyes, 2UL, TDNE_MASK_ALL);
    tdne_target_init(&target, tdne_vec3_make(0, 0, 100), 8, 1UL, 0);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_MASKED);
}

static void test_scan_sort(void)
{
    tdne_sensor eyes;
    tdne_target targets[2];
    tdne_result results[2];
    int count;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 1000, 90);
    tdne_target_init(&targets[0], tdne_vec3_make(0, 0, 800), 8, 1UL, 0);
    tdne_target_init(&targets[1], tdne_vec3_make(0, 0, 100), 8, 1UL, 0);

    count = tdne_scan_targets(&eyes, targets, 2, results, 2, clear_world, 0, TDNE_SCAN_SORT_BY_SCORE);
    assert(count == 2);
    assert(results[0].target_index == 1);
}

static void test_partial_visibility(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(&eyes, tdne_vec3_zero(), tdne_vec3_make(0, 0, 1), 500, 90);
    tdne_target_init(&target, tdne_vec3_make(0, 0, 100), 8, 1UL, 0);
    tdne_target_clear_samples(&target);
    assert(tdne_target_add_sample(&target, tdne_vec3_make(0, 0, 0)) == TDNE_TRUE);
    assert(tdne_target_add_sample(&target, tdne_vec3_make(300, 0, 0)) == TDNE_TRUE);

    assert(tdne_eval_target(&eyes, &target, clear_world, 0, &result) == TDNE_VIS_PARTIAL);
    assert(result.visible == TDNE_TRUE);
    assert(result.rays_clear == 1);
}

int main(void)
{
    test_cone_visible();
    test_cone_out_of_shape();
    test_out_of_range();
    test_occluded();
    test_sphere_can_see_behind();
    test_box();
    test_frustum();
    test_mask();
    test_scan_sort();
    test_partial_visibility();

    printf("3D_NPC_Eyes tests passed.\n");
    return 0;
}
