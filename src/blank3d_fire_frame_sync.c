#include "blank3d_fire_frame_sync.h"

#include <string.h>

void blank3d_fire_frame_sync_init(Blank3DFireFrameSync *sync)
{
    if (sync == 0) return;
    memset(sync, 0, sizeof(*sync));
    sync->initialized = 1;
}

void blank3d_fire_frame_sync_queue_recoil(
    Blank3DFireFrameSync *sync,
    g3d_fix pitch_delta,
    g3d_fix shake_amplitude)
{
    g3d_fix magnitude;
    if (sync == 0) return;
    if (!sync->initialized) blank3d_fire_frame_sync_init(sync);

    sync->pending_pitch = g3d_fix_add_sat(sync->pending_pitch,
                                          pitch_delta);
    magnitude = shake_amplitude < 0
              ? g3d_fix_neg_sat(shake_amplitude)
              : shake_amplitude;
    if (magnitude > sync->pending_shake)
        sync->pending_shake = magnitude;
    if (sync->pending_events < 65535U)
        sync->pending_events += 1U;
    sync->queued_total += 1UL;
}

unsigned int blank3d_fire_frame_sync_apply_pending(
    Blank3DFireFrameSync *sync,
    Blank3DCameraNaku *camera)
{
    unsigned int count;
    if (sync == 0 || camera == 0) return 0U;
    if (!sync->initialized) blank3d_fire_frame_sync_init(sync);
    count = sync->pending_events;
    if (count == 0U) return 0U;

    blank3d_cameranaku_add_recoil(camera,
                                  sync->pending_pitch,
                                  sync->pending_shake);
    sync->pending_pitch = 0;
    sync->pending_shake = 0;
    sync->pending_events = 0U;
    sync->applied_total += (unsigned long)count;
    return count;
}

void blank3d_fire_frame_sync_capture(
    Blank3DFireFrameSync *sync,
    unsigned int frame_id,
    const Blank3DCameraNaku *camera)
{
    if (sync == 0 || camera == 0) return;
    if (!sync->initialized) blank3d_fire_frame_sync_init(sync);
    blank3d_cameranaku_get_view(camera,
                                &sync->eye,
                                &sync->forward,
                                &sync->right,
                                &sync->up);
    sync->frame_id = frame_id;
    sync->view_valid = 1;
}

int blank3d_fire_frame_sync_get_view(
    const Blank3DFireFrameSync *sync,
    unsigned int frame_id,
    Vec3 *eye,
    Vec3 *forward,
    Vec3 *right,
    Vec3 *up)
{
    if (sync == 0 || !sync->initialized || !sync->view_valid ||
        sync->frame_id != frame_id)
        return 0;
    if (eye != 0) *eye = sync->eye;
    if (forward != 0) *forward = sync->forward;
    if (right != 0) *right = sync->right;
    if (up != 0) *up = sync->up;
    return 1;
}
