#include <stdio.h>

#include "blank3d_fire_frame_sync.h"

static int fail(const char *message, int frame)
{
    fprintf(stderr, "FAIL frame %d: %s\n", frame, message);
    return 1;
}

static int same_vec(Vec3 a, Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

int main(void)
{
    Blank3DCameraNaku camera;
    Blank3DFireFrameSync sync;
    Vec3 target;
    Vec3 velocity;
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 frozen_eye;
    Vec3 frozen_forward;
    Vec3 frozen_right;
    Vec3 frozen_up;
    Vec3 previous_forward;
    Vec3 muzzle;
    Vec3 aim_target;
    Vec3 offset;
    Vec3 launch;
    g3d_fix pitch_before_queue;
    g3d_fix recoil_pitch;
    g3d_fix recoil_shake;
    unsigned int applied;
    int frame;

    blank3d_fire_frame_sync_init(&sync);
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
    blank3d_cameranaku_set_mode(&camera, B3D_CNK_CAMERA_TPS);
    blank3d_cameranaku_set_angles(&camera, 0, G3D_FIX_FROM_INT(-8));

    target = gamlib_vec3(0, 0, 0);
    velocity = gamlib_vec3(0, 0, 0);
    previous_forward = gamlib_vec3(0, 0, 0);
    recoil_pitch = G3D_FIX_ONE / 32L;
    recoil_shake = G3D_FIX_ONE / 64L;

    /* More than two complete yaw revolutions while the player advances and
       alternates machine-gun/Gatling-sized recoil requests every frame. */
    for (frame = 1; frame <= 900; ++frame) {
        applied = blank3d_fire_frame_sync_apply_pending(&sync, &camera);
        if (frame == 1) {
            if (applied != 0U) return fail("first frame applied recoil", frame);
        } else if (applied != 1U) {
            return fail("previous automatic shot was not applied once", frame);
        }

        target.x = g3d_fix_add_sat(target.x, G3D_FIX_ONE / 20L);
        target.z = g3d_fix_add_sat(target.z, G3D_FIX_ONE / 12L);
        velocity.x = G3D_FIX_ONE / 20L;
        velocity.z = G3D_FIX_ONE / 12L;
        blank3d_cameranaku_add_look(&camera, G3D_FIX_FROM_INT(1), 0);
        blank3d_cameranaku_set_target(&camera, &target, &velocity);
        blank3d_cameranaku_update(&camera, 16);
        blank3d_fire_frame_sync_capture(&sync, (unsigned int)frame, &camera);

        if (!blank3d_fire_frame_sync_get_view(
                &sync, (unsigned int)frame,
                &eye, &forward, &right, &up))
            return fail("captured fire frame unavailable", frame);
        if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON)
            return fail("captured forward collapsed", frame);
        if (frame > 1 &&
            gamlib_vec3_dot(&previous_forward, &forward) <
            (G3D_FIX_ONE - G3D_FIX_ONE / 20L))
            return fail("yaw wrap produced a basis discontinuity", frame);
        previous_forward = forward;

        frozen_eye = eye;
        frozen_forward = forward;
        frozen_right = right;
        frozen_up = up;
        pitch_before_queue = camera.pitch;

        /* Alternate two automatic-weapon recoil magnitudes. Queuing must not
           mutate the camera or the frame used by the current HUD/projectile. */
        blank3d_fire_frame_sync_queue_recoil(
            &sync,
            (frame & 1) ? recoil_pitch : g3d_fix_add_sat(recoil_pitch,
                                                         recoil_pitch),
            recoil_shake);
        if (camera.pitch != pitch_before_queue)
            return fail("recoil changed camera inside the firing frame", frame);
        if (!blank3d_fire_frame_sync_get_view(
                &sync, (unsigned int)frame,
                &eye, &forward, &right, &up))
            return fail("queued recoil invalidated current frame", frame);
        if (!same_vec(eye, frozen_eye) ||
            !same_vec(forward, frozen_forward) ||
            !same_vec(right, frozen_right) ||
            !same_vec(up, frozen_up))
            return fail("HUD basis changed after automatic shot", frame);

        /* A muzzle moving with the actor still converges on the exact frozen
           center-HUD ray for this frame. */
        muzzle = target;
        muzzle.y = G3D_FIX_FROM_INT(1);
        gamlib_vec3_scale(&offset, &frozen_forward,
                          G3D_FIX_FROM_INT(32));
        gamlib_vec3_add(&aim_target, &frozen_eye, &offset);
        gamlib_vec3_sub(&launch, &aim_target, &muzzle);
        gamlib_vec3_normalize(&launch, &launch);
        if (gamlib_vec3_dot(&launch, &frozen_forward) <= 0)
            return fail("automatic projectile entered rear hemisphere", frame);
    }

    if (sync.queued_total != 900UL)
        return fail("queued recoil accounting mismatch", 900);
    if (sync.applied_total != 899UL)
        return fail("deferred recoil accounting mismatch", 900);
    if (sync.pending_events != 1U)
        return fail("last shot was not deferred to next frame", 900);

    puts("PASS: 900-frame moving/rotating automatic fire keeps one immutable HUD basis");
    return 0;
}
