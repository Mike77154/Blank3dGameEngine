/*
    demo_camera_zones.c
    Build example:
    Example build from package root: gcc -std=c89 -pedantic -Wall -Wextra -Iinclude src/cameranaku89.c src/cameranaku89_profiles.c src/cameranaku89_adapter.c src/cameranaku89_collision.c src/cameranaku89_spring_arm.c src/cameranaku89_target_group.c src/cameranaku89_composer.c src/cameranaku89_virtual.c src/cameranaku89_manager.c src/cameranaku89_lens_ext.c src/cameranaku89_freelook.c src/cameranaku89_shake_ext.c src/cameranaku89_zones.c demo/demo_camera_zones.c -o demo_camera_zones
*/
#include <stdio.h>
#include "cameranaku89_all.h"

static cnk_target make_target(int x, int y, int z, int yaw)
{
    cnk_target t;
    t.pos = cnk_vec3_make(CNK_FX_FROM_INT(x), CNK_FX_FROM_INT(y), CNK_FX_FROM_INT(z));
    t.velocity = cnk_vec3_make(0, 0, 0);
    t.yaw_deg = CNK_DEG(yaw);
    t.pitch_deg = 0;
    t.roll_deg = 0;
    t.valid = 1;
    return t;
}

int main(void)
{
    cnk_camera_manager mgr;
    cnk_camera_zone_bank zones;
    cnk_virtual_camera *follow;
    cnk_virtual_camera *fixed;
    cnk_profile profile;
    cnk_pose fixed_pose;
    cnk_vec3 fixed_look;
    cnk_target target;
    cnk_camera final_cam;
    int zone_index;
    int frame;

    cnk_camera_manager_init(&mgr);
    cnk_camera_manager_set_switch_policy(&mgr, 8, CNK_BLEND_EASE_IN_OUT, CNK_FX_FROM_INT(120));
    cnk_camera_zone_bank_clear(&zones);

    target = make_target(-6, 0, 0, 0);

    follow = cnk_camera_manager_alloc(&mgr, 100);
    fixed = cnk_camera_manager_alloc(&mgr, 200);

    cnk_profile_default(&profile);
    cnk_profile_style_follow(&profile);
    cnk_virtual_camera_set_profile(follow, &profile);
    cnk_virtual_camera_set_priority(follow, 4);
    follow->min_live_ticks = 4;

    fixed_pose.pos = cnk_vec3_make(CNK_FX_FROM_INT(0), CNK_FX_FROM_INT(4), -CNK_FX_FROM_INT(10));
    fixed_pose.yaw_deg = 0;
    fixed_pose.pitch_deg = 0;
    fixed_pose.roll_deg = 0;
    fixed_look = cnk_vec3_make(0, 0, 0);
    cnk_profile_style_fixed(&profile, fixed_pose, fixed_look);
    profile.lens.viewport_w = 640;
    profile.lens.viewport_h = 360;
    cnk_virtual_camera_set_profile(fixed, &profile);
    fixed->camera.state.pose.pos = cnk_vec3_make(CNK_FX_FROM_INT(0), CNK_FX_FROM_INT(4), -CNK_FX_FROM_INT(10));
    fixed->camera.state.look_at = cnk_vec3_make(0, 0, 0);
    fixed->camera.state.valid = 1;
    cnk_virtual_camera_set_priority(fixed, 2);
    cnk_composer_set_rule(&fixed->composer, CNK_COMPOSER_RULE_OF_THIRDS_RIGHT);

    zone_index = cnk_camera_zone_add_box(&zones, 1, 200,
        cnk_vec3_make(-CNK_FX_FROM_INT(1), -CNK_FX_FROM_INT(2), -CNK_FX_FROM_INT(4)),
        cnk_vec3_make(CNK_FX_FROM_INT(7), CNK_FX_FROM_INT(4), CNK_FX_FROM_INT(4)),
        10,
        CNK_ZONE_FORCE_CAMERA | CNK_ZONE_ENABLE_CAMERA | CNK_ZONE_USE_BLEND | CNK_ZONE_APPLY_CONFINER);
    cnk_camera_zone_set_bias(&zones, zone_index, 8, 6, 6);
    cnk_camera_zone_set_confiner(&zones, zone_index,
        cnk_vec3_make(-CNK_FX_FROM_INT(8), 0, -CNK_FX_FROM_INT(12)),
        cnk_vec3_make(CNK_FX_FROM_INT(8), CNK_FX_FROM_INT(8), CNK_FX_FROM_INT(2)));

    cnk_camera_reset(&final_cam);

    for (frame = 0; frame < 10; ++frame) {
        target.pos.x += CNK_FX_FROM_INT(1);
        cnk_camera_manager_set_target_all(&mgr, target);
        cnk_camera_zone_bank_apply(&zones, &mgr, target.pos);
        cnk_camera_manager_update(&mgr, 1, 0, 0);
        cnk_camera_manager_emit_to_camera(&mgr, &final_cam);
        printf("frame=%d target_x=%d active_id=%d active_zone=%d final_x=%d final_z=%d\n",
               frame,
               CNK_FX_TO_INT(target.pos.x),
               mgr.active_index >= 0 ? mgr.vcams[mgr.active_index].id : -1,
               zones.active_zone_index,
               CNK_FX_TO_INT(final_cam.state.pose.pos.x),
               CNK_FX_TO_INT(final_cam.state.pose.pos.z));
    }
    return 0;
}
