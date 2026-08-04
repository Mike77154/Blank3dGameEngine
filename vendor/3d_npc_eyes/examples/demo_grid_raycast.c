#include <stdio.h>
#include "3d_npc_eyes.h"

#define GRID_W 10
#define GRID_H 10
#define CELL_SIZE 100

struct grid_world {
    char cells[GRID_H][GRID_W + 1];
};

static tdne_i32 local_abs(tdne_i32 v)
{
    if (v < 0) {
        return -v;
    }
    return v;
}

static tdne_i32 local_max3(tdne_i32 a, tdne_i32 b, tdne_i32 c)
{
    tdne_i32 m;
    m = a;
    if (b > m) {
        m = b;
    }
    if (c > m) {
        m = c;
    }
    return m;
}

static int grid_raycast(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
)
{
    struct grid_world *world;
    tdne_i32 dx;
    tdne_i32 dy;
    tdne_i32 dz;
    tdne_i32 steps;
    tdne_i32 i;
    tdne_i32 x;
    tdne_i32 z;
    int gx;
    int gz;

    (void)block_mask;

    world = (struct grid_world *)world_user;
    dx = to->x - from->x;
    dy = to->y - from->y;
    dz = to->z - from->z;
    steps = local_max3(local_abs(dx), local_abs(dy), local_abs(dz)) / 10;
    if (steps < 1) {
        steps = 1;
    }

    for (i = 0; i <= steps; ++i) {
        x = from->x + (dx * i) / steps;
        z = from->z + (dz * i) / steps;
        gx = (int)(x / CELL_SIZE);
        gz = (int)(z / CELL_SIZE);

        if (gx >= 0 && gx < GRID_W && gz >= 0 && gz < GRID_H) {
            if (world->cells[gz][gx] == '#') {
                if (out_hit != 0) {
                    out_hit->hit = TDNE_TRUE;
                    out_hit->point = tdne_vec3_make(x, from->y + (dy * i) / steps, z);
                    out_hit->normal = tdne_vec3_zero();
                    out_hit->material_mask = 1UL;
                    out_hit->user = 0;
                }
                return TDNE_TRUE;
            }
        }
    }

    if (out_hit != 0) {
        out_hit->hit = TDNE_FALSE;
    }
    return TDNE_FALSE;
}

int main(void)
{
    struct grid_world world = {
        {
            "..........",
            "..........",
            "....#.....",
            "....#.....",
            "....#.....",
            "..........",
            "..........",
            "..........",
            "..........",
            ".........."
        }
    };
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(
        &eyes,
        tdne_vec3_make(200, 0, 200),
        tdne_vec3_make(1, 0, 1),
        800,
        100
    );

    tdne_target_init(&target, tdne_vec3_make(700, 0, 300), 10, 1UL, 0);
    tdne_target_use_vertical3(&target, 40, TDNE_AXIS_Y);

    tdne_eval_target(&eyes, &target, grid_raycast, &world, &result);

    printf("state=%s visible=%d clear=%d blocked=%d score=%ld\n",
        tdne_visibility_name(result.visibility),
        result.visible,
        result.rays_clear,
        result.rays_blocked,
        result.score
    );

    if (result.hit.hit) {
        printf("first block at x=%ld y=%ld z=%ld\n",
            result.hit.point.x,
            result.hit.point.y,
            result.hit.point.z
        );
    }

    return 0;
}
