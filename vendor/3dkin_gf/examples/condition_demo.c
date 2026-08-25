#include <stdio.h>
#include "gk3d.h"

#define OBJ_PLAYER 1
#define OBJ_WORLD 2
#define OBJ_TRIGGER 3

int main(void)
{
    gk3d_world world;
    int player;
    int floor_id;
    int wall_id;
    int trigger_id;
    int found;

    gk3d_world_init(&world);

    floor_id = gk3d_world_add_box(&world, OBJ_WORLD,
        GK3D_FROM_INT(-20), GK3D_FROM_INT(0), GK3D_FROM_INT(-20),
        GK3D_FROM_INT(40), GK3D_FROM_INT(1), GK3D_FROM_INT(40),
        GK3D_FLAG_SOLID);

    wall_id = gk3d_world_add_box(&world, OBJ_WORLD,
        GK3D_FROM_INT(3), GK3D_FROM_INT(1), GK3D_FROM_INT(-2),
        GK3D_FROM_INT(1), GK3D_FROM_INT(4), GK3D_FROM_INT(4),
        GK3D_FLAG_SOLID);

    trigger_id = gk3d_world_add_box(&world, OBJ_TRIGGER,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(4),
        GK3D_FROM_INT(2), GK3D_FROM_INT(2), GK3D_FROM_INT(2),
        GK3D_FLAG_TRIGGER);

    player = gk3d_world_add_box(&world, OBJ_PLAYER,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(0),
        GK3D_FROM_INT(1), GK3D_FROM_INT(2), GK3D_FROM_INT(1),
        0u);

    printf("floor=%d expected=%d\n",
        gk3d_is_on_floor(&world, player), floor_id != GK3D_ID_NONE);

    printf("wall_now=%d\n", gk3d_is_on_wall(&world, player));
    gk3d_obj_set_pos(&world, player,
        GK3D_FROM_INT(2), GK3D_FROM_INT(1), GK3D_FROM_INT(0));
    printf("wall_after_move=%d expected=%d\n",
        gk3d_is_on_wall(&world, player), wall_id != GK3D_ID_NONE);

    found = gk3d_instance_place(&world, player,
        GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(4),
        OBJ_TRIGGER);
    printf("instance_place=%d trigger=%d\n", found, trigger_id);

    printf("place_free_at_trigger=%d\n",
        gk3d_place_free(&world, player,
            GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(4)));

    return 0;
}
