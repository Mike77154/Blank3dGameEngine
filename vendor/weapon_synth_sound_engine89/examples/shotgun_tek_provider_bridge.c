#include "gshotgunsequence89.h"

/*
 * Generic adapter for an external caller-owned "tek" synthesis library.
 * The host fills these callbacks with the real library functions.
 */
typedef struct shotgun_tek_host_api_s {
    void (*trigger)(void *state, unsigned int seed,
                    unsigned short delay_ms, int model);
    signed short (*process)(void *state);
    int (*active)(const void *state);
    void *state;
} shotgun_tek_host_api;

static void shotgun_tek_trigger_adapter(void *user, gss89_u32 seed,
                                         gss89_u16 delay_ms,
                                         gss89_shotgun_model model)
{
    shotgun_tek_host_api *api;
    api = (shotgun_tek_host_api *)user;
    if (api != 0 && api->trigger != 0) {
        api->trigger(api->state, (unsigned int)seed,
                     (unsigned short)delay_ms, (int)model);
    }
}

static gss89_s16 shotgun_tek_process_adapter(void *user)
{
    shotgun_tek_host_api *api;
    api = (shotgun_tek_host_api *)user;
    if (api == 0 || api->process == 0) return 0;
    return (gss89_s16)api->process(api->state);
}

static int shotgun_tek_active_adapter(const void *user)
{
    const shotgun_tek_host_api *api;
    api = (const shotgun_tek_host_api *)user;
    if (api == 0 || api->active == 0) return 0;
    return api->active(api->state);
}

void shotgun_tek_connect(gss89_context *shotgun,
                          shotgun_tek_host_api *api)
{
    if (shotgun == 0 || api == 0) return;
    gss89_set_tek_provider(shotgun,
                           shotgun_tek_trigger_adapter,
                           shotgun_tek_process_adapter,
                           shotgun_tek_active_adapter,
                           api);
}
