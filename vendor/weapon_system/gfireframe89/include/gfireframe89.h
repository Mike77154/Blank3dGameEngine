#ifndef GFIRE_FRAME89_H
#define GFIRE_FRAME89_H
#include "gweapon89.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct GFireFrame89Tag {
    int initialized;
    int view_valid;
    unsigned int frame_id;
    GWP89_Vec3 eye;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    gwp89_fx pending_pitch;
    gwp89_fx pending_shake;
    unsigned int pending_events;
    unsigned long queued_total;
    unsigned long applied_total;
} GFireFrame89;

typedef int (*GFireFrame89ViewProvider)(void *user,
                                        GWP89_CameraState *out_camera);
typedef void (*GFireFrame89RecoilProvider)(void *user,
                                            gwp89_fx pitch_delta,
                                            gwp89_fx shake_amplitude);

void gfireframe89_init(GFireFrame89 *sync);
void gfireframe89_queue_recoil(GFireFrame89 *sync,
                               gwp89_fx pitch_delta,
                               gwp89_fx shake_amplitude);
unsigned int gfireframe89_apply_pending(GFireFrame89 *sync,
                                         GFireFrame89RecoilProvider provider,
                                         void *provider_user);
int gfireframe89_capture(GFireFrame89 *sync,
                          unsigned int frame_id,
                          GFireFrame89ViewProvider provider,
                          void *provider_user);
int gfireframe89_get_view(const GFireFrame89 *sync,
                           unsigned int frame_id,
                           GWP89_Vec3 *eye,
                           GWP89_Vec3 *forward,
                           GWP89_Vec3 *right,
                           GWP89_Vec3 *up);
#ifdef __cplusplus
}
#endif
#endif
