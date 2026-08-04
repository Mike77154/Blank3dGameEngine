/*
    demo_cameranaku89.c
    Build example:
    gcc -std=c89 -pedantic -Wall -Wextra -I../include ../src/cameranaku89.c ../src/cameranaku89_profiles.c ../src/cameranaku89_adapter.c demo_cameranaku89.c -o demo_cameranaku89
*/
#include <stdio.h>
#include "cameranaku89.h"
#include "cameranaku89_profiles.h"

static void print_vec(const char *name, cnk_vec3 v)
{
    printf("%s=(%ld,%ld,%ld) fx / int=(%d,%d,%d)\n",
        name,
        v.x, v.y, v.z,
        CNK_FX_TO_INT(v.x), CNK_FX_TO_INT(v.y), CNK_FX_TO_INT(v.z));
}

int main(void)
{
    cnk_camera cam;
    cnk_profile profile;
    cnk_vec3 player_pos;
    cnk_vec3 player_vel;
    cnk_vec3 test_point;
    int sx;
    int sy;
    cnk_fx depth;
    int i;

    cnk_camera_reset(&cam);
    cnk_profile_default(&profile);
    cnk_profile_apply_style(&profile, CNK_STYLE_OTS);
    profile.lens.viewport_w = 1280;
    profile.lens.viewport_h = 720;
    cnk_profile_validate(&profile);
    cnk_camera_apply_profile(&cam, &profile);

    player_pos = cnk_vec3_make(CNK_FX_FROM_INT(0), CNK_FX_FROM_INT(0), CNK_FX_FROM_INT(0));
    player_vel = cnk_vec3_make(0, 0, 0);
    cnk_camera_set_target(&cam, player_pos, player_vel, CNK_DEG(0), 0, 0);
    cnk_camera_snap(&cam);

    for (i = 0; i < 6; ++i) {
        player_pos.z += CNK_FX_FROM_INT(1);
        cnk_camera_set_target(&cam, player_pos, player_vel, CNK_DEG(0), 0, 0);
        cnk_camera_add_input(&cam, CNK_DEG(3), 0, 0, 0);
        cnk_camera_update(&cam, 1, 0, 0);
        printf("frame %d style=%d mode=%d\n", i, cam.profile.style, cam.profile.mode);
        print_vec("camera", cam.state.pose.pos);
        test_point = cnk_vec3_make(CNK_FX_FROM_INT(0), CNK_FX_FROM_INT(2), player_pos.z + CNK_FX_FROM_INT(6));
        if (cnk_camera_project_point(&cam, test_point, &sx, &sy, &depth) != 0) {
            printf("projected point x=%d y=%d depth=%d\n", sx, sy, CNK_FX_TO_INT(depth));
        } else {
            printf("projected point clipped\n");
        }
    }

    return 0;
}
