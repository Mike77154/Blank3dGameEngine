#include <stdio.h>
#include "gk3d.h"

int main(void)
{
    gk3d_world world;
    gk3d_condition_result result;
    int actor;

    gk3d_world_init(&world);
    gk3d_world_add_box(&world, 20,
        GK3D_FROM_INT(-8), GK3D_FROM_INT(0), GK3D_FROM_INT(-8),
        GK3D_FROM_INT(16), GK3D_FROM_INT(1), GK3D_FROM_INT(16),
        GK3D_FLAG_SOLID);
    actor = gk3d_world_add_box(&world, 10,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(0),
        GK3D_FROM_INT(1), GK3D_FROM_INT(2), GK3D_FROM_INT(1),
        0u);

    gk3d_check_verb(&world, "isOnFloor", actor, GK3D_TARGET_SOLID,
        0, 0, 0, 0, 0, 0, &result);

    printf("verb truth=%d instance=%d\n",
        result.truth, result.instance_id);
    return 0;
}
