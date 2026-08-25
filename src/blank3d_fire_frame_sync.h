#ifndef BLANK3D_FIRE_FRAME_SYNC_H
#define BLANK3D_FIRE_FRAME_SYNC_H
#include "blank3d_cameranaku.h"
#include "gfireframe89.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef GFireFrame89 Blank3DFireFrameSync;
void blank3d_fire_frame_sync_init(Blank3DFireFrameSync *sync);
void blank3d_fire_frame_sync_queue_recoil(Blank3DFireFrameSync *sync,g3d_fix pitch_delta,g3d_fix shake_amplitude);
unsigned int blank3d_fire_frame_sync_apply_pending(Blank3DFireFrameSync *sync,Blank3DCameraNaku *camera);
void blank3d_fire_frame_sync_capture(Blank3DFireFrameSync *sync,unsigned int frame_id,const Blank3DCameraNaku *camera);
int blank3d_fire_frame_sync_get_view(const Blank3DFireFrameSync *sync,unsigned int frame_id,Vec3 *eye,Vec3 *forward,Vec3 *right,Vec3 *up);
#ifdef __cplusplus
}
#endif
#endif
