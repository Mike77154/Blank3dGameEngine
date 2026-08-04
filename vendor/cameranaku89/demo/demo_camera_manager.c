/*
    demo_camera_manager.c
    C89 demo for Cameranaku89 v3.2 manager + virtual cameras.
*/
#include <stdio.h>
#include "cameranaku89.h"
#include "cameranaku89_profiles.h"
#include "cameranaku89_manager.h"
#include "cameranaku89_lens_ext.h"
#include "cameranaku89_shake_ext.h"

static int demo_probe(void *user, cnk_vec3 from, cnk_vec3 to, cnk_fx radius, cnk_vec3 *out_pos)
{
    (void)user;
    (void)from;
    (void)radius;
    if (to.z < -CNK_FX_FROM_INT(6)) {
        *out_pos = to;
        out_pos->z = -CNK_FX_FROM_INT(6);
        return 1;
    }
    return 0;
}

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
    cnk_virtual_camera *follow;
    cnk_virtual_camera *ots;
    cnk_profile p;
    cnk_target target;
    cnk_shake_request shake;
    cnk_camera final_cam;
    int frame;
    int sx;
    int sy;
    cnk_fx depth;

    cnk_camera_manager_init(&mgr);
    target = make_target(0, 0, 8, 0);

    follow = cnk_camera_manager_alloc(&mgr, 10);
    ots = cnk_camera_manager_alloc(&mgr, 20);

    cnk_profile_default(&p);
    cnk_profile_style_follow(&p);
    cnk_virtual_camera_set_profile(follow, &p);
    cnk_virtual_camera_set_target(follow, target);
    cnk_virtual_camera_set_priority(follow, 5);
    cnk_virtual_camera_enable_spring_arm(follow, 1);

    cnk_profile_style_ots(&p);
    cnk_lens_set_frustum_offset(&p.lens, CNK_FX_FRAC(8, 100), 0);
    cnk_lens_set_masks(&p.lens, 0xffffffffUL, 1UL);
    cnk_virtual_camera_set_profile(ots, &p);
    cnk_virtual_camera_set_target(ots, target);
    cnk_virtual_camera_set_priority(ots, 8);
    cnk_virtual_camera_enable_spring_arm(ots, 1);

    cnk_shake_request_default(&shake, CNK_SHAKE_KIND_RECOIL);
    shake.seed = 99UL;
    cnk_camera_add_shake_request(&ots->camera, &shake);

    cnk_camera_reset(&final_cam);

    for (frame = 0; frame < 8; ++frame) {
        target.pos.x += CNK_FX_FRAC(1, 4);
        cnk_camera_manager_set_target_all(&mgr, target);
        cnk_camera_manager_update(&mgr, 1, demo_probe, 0);
        cnk_camera_manager_emit_to_camera(&mgr, &final_cam);
        cnk_camera_project_point(&final_cam, target.pos, &sx, &sy, &depth);
        printf("frame=%d active=%d final=(%d,%d,%d) screen=(%d,%d) q=%ld\n",
               frame,
               mgr.active_index,
               CNK_FX_TO_INT(final_cam.state.pose.pos.x),
               CNK_FX_TO_INT(final_cam.state.pose.pos.y),
               CNK_FX_TO_INT(final_cam.state.pose.pos.z),
               sx,
               sy,
               (long)mgr.vcams[mgr.active_index].shot.quality);
    }
    return 0;
}
