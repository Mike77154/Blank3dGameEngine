#include <stdio.h>
#include "3d_npc_eyes.h"

static int no_world_blocks(
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

int main(void)
{
    tdne_sensor eyes;
    tdne_target targets[3];
    tdne_result results[3];
    int count;
    int i;

    tdne_sensor_init_cone(
        &eyes,
        tdne_vec3_make(0, 0, 0),
        tdne_vec3_make(0, 0, 1),
        500,
        90
    );

    tdne_target_init(&targets[0], tdne_vec3_make(0, 0, 120), 12, 1UL, 0);
    tdne_target_init(&targets[1], tdne_vec3_make(200, 0, 0), 12, 1UL, 0);
    tdne_target_init(&targets[2], tdne_vec3_make(0, 0, 900), 12, 1UL, 0);

    count = tdne_scan_targets(
        &eyes,
        targets,
        3,
        results,
        3,
        no_world_blocks,
        0,
        TDNE_SCAN_KEEP_REJECTED | TDNE_SCAN_SORT_BY_SCORE
    );

    for (i = 0; i < count; ++i) {
        printf(
            "target=%d state=%s visible=%d score=%ld distance=%ld dot=%ld\n",
            results[i].target_index,
            tdne_visibility_name(results[i].visibility),
            results[i].visible,
            results[i].score,
            results[i].distance,
            results[i].dot
        );
    }

    return 0;
}
