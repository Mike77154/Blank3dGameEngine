#include "gfireframe89.h"
#include <string.h>

static gwp89_fx gff89_add_sat(gwp89_fx a, gwp89_fx b)
{
    long r;
    if (b > 0L && a > 2147483647L - b) return 2147483647L;
    if (b < 0L && a < (-2147483647L - 1L) - b) return (-2147483647L - 1L);
    r = a + b;
    return (gwp89_fx)r;
}

void gfireframe89_init(GFireFrame89 *sync)
{
    if (!sync) return;
    memset(sync, 0, sizeof(*sync));
    sync->initialized = 1;
}

void gfireframe89_queue_recoil(GFireFrame89 *sync,
                               gwp89_fx pitch_delta,
                               gwp89_fx shake_amplitude)
{
    gwp89_fx magnitude;
    if (!sync) return;
    if (!sync->initialized) gfireframe89_init(sync);
    sync->pending_pitch = gff89_add_sat(sync->pending_pitch, pitch_delta);
    magnitude = shake_amplitude < 0 ? -shake_amplitude : shake_amplitude;
    if (magnitude > sync->pending_shake) sync->pending_shake = magnitude;
    if (sync->pending_events < 65535U) ++sync->pending_events;
    ++sync->queued_total;
}

unsigned int gfireframe89_apply_pending(GFireFrame89 *sync,
                                         GFireFrame89RecoilProvider provider,
                                         void *provider_user)
{
    unsigned int count;
    if (!sync || !provider) return 0U;
    if (!sync->initialized) gfireframe89_init(sync);
    count = sync->pending_events;
    if (!count) return 0U;
    provider(provider_user, sync->pending_pitch, sync->pending_shake);
    sync->pending_pitch = 0;
    sync->pending_shake = 0;
    sync->pending_events = 0U;
    sync->applied_total += (unsigned long)count;
    return count;
}

int gfireframe89_capture(GFireFrame89 *sync,
                          unsigned int frame_id,
                          GFireFrame89ViewProvider provider,
                          void *provider_user)
{
    GWP89_CameraState camera;
    if (!sync || !provider) return 0;
    if (!sync->initialized) gfireframe89_init(sync);
    memset(&camera, 0, sizeof(camera));
    if (!provider(provider_user, &camera) || !camera.valid) return 0;
    sync->eye = camera.origin;
    sync->forward = camera.forward;
    sync->right = camera.right;
    sync->up = camera.up;
    sync->frame_id = frame_id;
    sync->view_valid = 1;
    return 1;
}

int gfireframe89_get_view(const GFireFrame89 *sync,
                           unsigned int frame_id,
                           GWP89_Vec3 *eye,
                           GWP89_Vec3 *forward,
                           GWP89_Vec3 *right,
                           GWP89_Vec3 *up)
{
    if (!sync || !sync->initialized || !sync->view_valid ||
        sync->frame_id != frame_id) return 0;
    if (eye) *eye = sync->eye;
    if (forward) *forward = sync->forward;
    if (right) *right = sync->right;
    if (up) *up = sync->up;
    return 1;
}
