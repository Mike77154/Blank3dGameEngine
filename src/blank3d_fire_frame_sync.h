#ifndef BLANK3D_FIRE_FRAME_SYNC_H
#define BLANK3D_FIRE_FRAME_SYNC_H

#include "blank3d_cameranaku.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * One immutable player-camera basis per simulation/render frame.
 *
 * Weapon events may request recoil while the event queue is being drained.
 * Applying that recoil immediately would make the HUD/render camera newer
 * than the projectile snapshot created earlier in the same frame.  This
 * state queues recoil and commits it before the next camera update instead.
 */
typedef struct Blank3DFireFrameSyncTag {
    int initialized;
    int view_valid;
    unsigned int frame_id;
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    g3d_fix pending_pitch;
    g3d_fix pending_shake;
    unsigned int pending_events;
    unsigned long queued_total;
    unsigned long applied_total;
} Blank3DFireFrameSync;

void blank3d_fire_frame_sync_init(Blank3DFireFrameSync *sync);

void blank3d_fire_frame_sync_queue_recoil(
    Blank3DFireFrameSync *sync,
    g3d_fix pitch_delta,
    g3d_fix shake_amplitude);

/* Commit every recoil request accumulated by the previous rendered frame. */
unsigned int blank3d_fire_frame_sync_apply_pending(
    Blank3DFireFrameSync *sync,
    Blank3DCameraNaku *camera);

/* Capture the only camera basis that weapons, HUD and rendering may consume. */
void blank3d_fire_frame_sync_capture(
    Blank3DFireFrameSync *sync,
    unsigned int frame_id,
    const Blank3DCameraNaku *camera);

int blank3d_fire_frame_sync_get_view(
    const Blank3DFireFrameSync *sync,
    unsigned int frame_id,
    Vec3 *eye,
    Vec3 *forward,
    Vec3 *right,
    Vec3 *up);

#ifdef __cplusplus
}
#endif

#endif
