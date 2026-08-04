#include "grecoil89_bridge.h"

void grec_bridge_init(GRecBridge *bridge, void *user,
                      GRecApplyCameraFn camera_fn,
                      GRecApplyAimFn aim_fn,
                      GRecApplyWeaponFn weapon_fn,
                      GRecEventFn fire_fn,
                      GRecEventFn update_fn)
{
    if (bridge == 0) return;
    bridge->callbacks.user = user;
    bridge->callbacks.apply_camera = camera_fn;
    bridge->callbacks.apply_aim = aim_fn;
    bridge->callbacks.apply_weapon = weapon_fn;
    bridge->callbacks.on_fire = fire_fn;
    bridge->callbacks.on_update = update_fn;
    bridge->has_output = 0;
    bridge->last_output.aim_angles.pitch = 0;
    bridge->last_output.aim_angles.yaw = 0;
    bridge->last_output.aim_angles.roll = 0;
    bridge->last_output.camera_angles.pitch = 0;
    bridge->last_output.camera_angles.yaw = 0;
    bridge->last_output.camera_angles.roll = 0;
    bridge->last_output.weapon_angles.pitch = 0;
    bridge->last_output.weapon_angles.yaw = 0;
    bridge->last_output.weapon_angles.roll = 0;
    bridge->last_output.weapon_offset.side = 0;
    bridge->last_output.weapon_offset.up = 0;
    bridge->last_output.weapon_offset.back = 0;
    bridge->last_output.spread = 0;
    bridge->last_output.burst_count = 0;
    bridge->last_output.shot_index = 0;
    bridge->last_output.active = 0;
}

void grec_bridge_after_fire(GRecBridge *bridge,
                            const GRecState *state,
                            const GRecProfile *profile,
                            const GRecContext *ctx)
{
    if (bridge == 0) return;
    grec_sample(state, profile, ctx, &bridge->last_output);
    bridge->has_output = 1;
    grec_apply_callbacks(&bridge->callbacks, &bridge->last_output, 1);
}

void grec_bridge_after_update(GRecBridge *bridge,
                              const GRecState *state,
                              const GRecProfile *profile,
                              const GRecContext *ctx)
{
    if (bridge == 0) return;
    grec_sample(state, profile, ctx, &bridge->last_output);
    bridge->has_output = 1;
    grec_apply_callbacks(&bridge->callbacks, &bridge->last_output, 0);
}
