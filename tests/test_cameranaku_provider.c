#include <stdio.h>
#include <string.h>

#include "blank3d_cameranaku.h"

static int fail(const char *message)
{
    printf("FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    Blank3DCameraNaku camera;
    Vec3 target;
    Vec3 velocity;
    Vec3 eye;
    Vec3 forward;
    GWP89_ProviderPacket packet;
    g3d_fix old_pitch;
    g3d_fix feeder_step;
    int result;

    target = gamlib_vec3(0, 0, 0);
    velocity = gamlib_vec3(0, 0, 0);
    blank3d_cameranaku_init(&camera,
                             960, 540,
                             G3D_FIX_FROM_INT(70),
                             G3D_FIX_FROM_INT(1),
                             G3D_FIX_FROM_INT(400));
    blank3d_cameranaku_configure(&camera,
                                  G3D_FIX_FROM_INT(2),
                                  G3D_FIX_FROM_INT(4),
                                  G3D_FIX_FROM_INT(8),
                                  G3D_FIX_FROM_INT(1),
                                  G3D_FIX_FROM_INT(-85),
                                  G3D_FIX_FROM_INT(85),
                                  G3D_FIX_FROM_INT(70),
                                  G3D_FIX_FROM_INT(1),
                                  G3D_FIX_FROM_INT(400));
    blank3d_cameranaku_set_target(&camera, &target, &velocity);
    blank3d_cameranaku_set_angles(&camera,
                                  G3D_FIX_FROM_INT(180),
                                  G3D_FIX_FROM_INT(-12));
    blank3d_cameranaku_set_mode(&camera, B3D_CNK_CAMERA_TPS);
    blank3d_cameranaku_update(&camera, 16);
    blank3d_cameranaku_get_view(&camera, &eye, &forward, 0, 0);

    if (!camera.initialized) return fail("camera did not initialize");
    if (camera.provider_move_calls == 0UL) return fail("move provider unused");
    if (camera.provider_scale_calls == 0UL) return fail("scale provider unused");
    if (camera.provider_rotate_calls == 0UL) return fail("rotate provider unused");
    if (gamlib_vec3_length(&forward) < (G3D_FIX_ONE / 2L))
        return fail("camera forward invalid");
    if (forward.z <= 0)
        return fail("Gamlib yaw 180 did not map to CamNaku world +Z");
    if (eye.y <= 0) return fail("TPS camera height was not applied");

    /* Verify all four semantic feeder bridges negate their old signs. */
    feeder_step = G3D_FIX_FROM_INT(5);
    blank3d_cameranaku_set_angles(&camera, 0, 0);
    blank3d_cameranaku_feed_left(&camera, feeder_step);
    if (camera.yaw <= 0) return fail("left feeder was not negated");

    blank3d_cameranaku_set_angles(&camera, 0, 0);
    blank3d_cameranaku_feed_right(&camera, feeder_step);
    if (camera.yaw >= 0) return fail("right feeder was not negated");

    blank3d_cameranaku_set_angles(&camera, 0, 0);
    blank3d_cameranaku_feed_up(&camera, feeder_step);
    if (camera.pitch >= 0) return fail("up feeder was not negated");

    blank3d_cameranaku_set_angles(&camera, 0, 0);
    blank3d_cameranaku_feed_down(&camera, feeder_step);
    if (camera.pitch <= 0) return fail("down feeder was not negated");

    blank3d_cameranaku_set_angles(&camera,
                                  G3D_FIX_FROM_INT(180),
                                  G3D_FIX_FROM_INT(-12));

    memset(&packet, 0, sizeof(packet));
    packet.phase = GWP89_PHASE_PRE;
    packet.service = GWP89_SERVICE_CAMERA;
    packet.operation = GWP89_OP_GET_CAMERA;
    packet.actor_id = 1;
    packet.actor_kind = 1;
    result = blank3d_cameranaku_weapon_provider(&camera, &packet);
    if ((result & GWP89_PROVIDER_HANDLED) == 0)
        return fail("weapon camera provider did not handle request");
    if (!packet.camera.valid || packet.camera.camera_id != B3D_CNK_CAMERA_ID)
        return fail("weapon camera identity invalid");
    if (packet.camera.view_style != GWP89_VIEW_OVER_SHOULDER)
        return fail("TPS view style not exported");
    if (packet.camera.forward.x == 0L &&
        packet.camera.forward.y == 0L &&
        packet.camera.forward.z == 0L)
        return fail("weapon camera forward missing");

    /* NPC actors must keep their AI-authored socket/camera basis. */
    memset(&packet, 0, sizeof(packet));
    packet.phase = GWP89_PHASE_PRE;
    packet.service = GWP89_SERVICE_CAMERA;
    packet.operation = GWP89_OP_GET_CAMERA;
    packet.actor_id = 1000;
    packet.actor_kind = 2;
    result = blank3d_cameranaku_weapon_provider(&camera, &packet);
    if (result != GWP89_PROVIDER_PASS)
        return fail("NPC camera request was not passed through");
    if (packet.camera.valid)
        return fail("NPC camera basis was overwritten by player camera");

    old_pitch = camera.pitch;
    blank3d_cameranaku_add_recoil(&camera,
                                   G3D_FIX_FROM_INT(2),
                                   G3D_FIX_FROM_INT(1));
    if (camera.pitch <= old_pitch) return fail("recoil did not reach camera");

    blank3d_cameranaku_set_mode(&camera, B3D_CNK_CAMERA_FPS);
    blank3d_cameranaku_update(&camera, 16);
    memset(&packet, 0, sizeof(packet));
    packet.phase = GWP89_PHASE_PRE;
    packet.service = GWP89_SERVICE_CAMERA;
    packet.operation = GWP89_OP_GET_CAMERA;
    packet.actor_id = 1;
    packet.actor_kind = 1;
    (void)blank3d_cameranaku_weapon_provider(&camera, &packet);
    if (packet.camera.view_style != GWP89_VIEW_FPS)
        return fail("FPS view style not exported");

    printf("Blank3D Cameranaku89 receive-provider test: OK\n");
    printf("TRS calls move=%lu scale=%lu rotate=%lu weapon=%lu\n",
           camera.provider_move_calls,
           camera.provider_scale_calls,
           camera.provider_rotate_calls,
           camera.weapon_provider_calls);
    return 0;
}
