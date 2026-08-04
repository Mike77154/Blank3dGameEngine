#ifndef GRECOIL89_BRIDGE_H
#define GRECOIL89_BRIDGE_H

#include "grecoil89.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   Tiny optional bridge helpers.
   They do not know your camera, weapon, entity, renderer or physics types.
   You pass callbacks and a user pointer.
*/

typedef struct GRecBridge_s {
    GRecCallbacks callbacks;
    GRecOutput last_output;
    int has_output;
} GRecBridge;

void grec_bridge_init(GRecBridge *bridge, void *user,
                      GRecApplyCameraFn camera_fn,
                      GRecApplyAimFn aim_fn,
                      GRecApplyWeaponFn weapon_fn,
                      GRecEventFn fire_fn,
                      GRecEventFn update_fn);

void grec_bridge_after_fire(GRecBridge *bridge,
                            const GRecState *state,
                            const GRecProfile *profile,
                            const GRecContext *ctx);

void grec_bridge_after_update(GRecBridge *bridge,
                              const GRecState *state,
                              const GRecProfile *profile,
                              const GRecContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
