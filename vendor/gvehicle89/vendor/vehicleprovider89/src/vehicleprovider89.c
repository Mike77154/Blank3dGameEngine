#include "vehicleprovider89.h"

void vehicleprovider89_movement_clear(vehicleprovider89_movement *provider)
{
    if (!provider) return;
    provider->user = 0;
    provider->module_mask = 0UL;
    provider->step = 0;
}

void vehicleprovider89_physics_clear(vehicleprovider89_physics *provider)
{
    if (!provider) return;
    provider->user = 0;
    provider->phase_mask = 0UL;
    provider->step = 0;
}

gveh_i32 vehicleprovider89_movement_try(
    const vehicleprovider89_movement *provider,
    const vehicleprovider89_movement_request *request)
{
    if (!provider || !request || !provider->step) return VEHICLEPROVIDER89_DECLINED;
    if ((provider->module_mask & request->module) == 0UL)
        return VEHICLEPROVIDER89_DECLINED;
    return provider->step(provider->user, request) == VEHICLEPROVIDER89_HANDLED
         ? VEHICLEPROVIDER89_HANDLED : VEHICLEPROVIDER89_DECLINED;
}

gveh_i32 vehicleprovider89_physics_try(
    const vehicleprovider89_physics *provider,
    const vehicleprovider89_physics_request *request)
{
    if (!provider || !request || !provider->step) return VEHICLEPROVIDER89_DECLINED;
    if ((provider->phase_mask & request->phase) == 0UL)
        return VEHICLEPROVIDER89_DECLINED;
    return provider->step(provider->user, request) == VEHICLEPROVIDER89_HANDLED
         ? VEHICLEPROVIDER89_HANDLED : VEHICLEPROVIDER89_DECLINED;
}
