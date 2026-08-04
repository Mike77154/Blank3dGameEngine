#include <stdio.h>
#include "3d_npc_eyes.h"

int main(void)
{
    tdne_sensor camera;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_frustum(
        &camera,
        tdne_vec3_make(0, 0, 0),
        tdne_vec3_make(0, 0, 1),
        tdne_vec3_make(1, 0, 0),
        tdne_vec3_make(0, 1, 0),
        10,
        1000,
        90,
        60
    );
    tdne_sensor_set_line_of_sight(&camera, TDNE_FALSE);

    tdne_target_init(&target, tdne_vec3_make(100, 20, 300), 8, 1UL, 0);
    tdne_eval_target(&camera, &target, 0, 0, &result);

    printf("frustum state=%s visible=%d distance=%ld score=%ld\n",
        tdne_visibility_name(result.visibility),
        result.visible,
        result.distance,
        result.score
    );

    return 0;
}
