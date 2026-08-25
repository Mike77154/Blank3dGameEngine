# Player fire feedback and skybox background isolation

Blank3D v3.27.3 regression fix.

## 1. Crosshair fire ownership

The local HUD crosshair receives its `firing` input from `g.muzzle_flash_ms > 0`.
`GWP89_EVENT_FIRE_ACCEPTED` already updated that timer only when
`event.actor_id == B3D_PLAYER_ACTOR_ID`, but the later
`GWP89_EVENT_MUZZLE_REQUEST` branch wrote `g.muzzle_flash_ms = 70` for every
actor. Therefore an ally or enemy muzzle request looked like local-player fire
to the HUD and triggered the crosshair animation.

`GWP89_EVENT_MUZZLE_REQUEST` is now world presentation only. It may emit muzzle
sprites and dynamic light for any actor, but it cannot mutate local-player HUD
state. `GWP89_EVENT_FIRE_ACCEPTED` remains the sole weapon-event owner of
`g.muzzle_flash_ms` and keeps its player actor gate.

Result:

```
NPC / ally FIRE_ACCEPTED + MUZZLE_REQUEST
    -> world muzzle sprite/light
    -> no player crosshair firing edge

PLAYER FIRE_ACCEPTED + MUZZLE_REQUEST
    -> player muzzle timer
    -> crosshair firing edge
    -> world muzzle sprite/light
```

## 2. Skybox draw authority

The skybox was rendered after opaque world geometry (floor, player, NPCs,
vehicles and objects). Screen and dome layers are intentionally capable of
depth/blend presentation, so drawing them after opaque geometry allowed the sky
to behave like a post-process and visually tint world pixels.

The host now renders the skybox immediately after installing the camera and
before any opaque/world geometry. Skybox depth writes remain disabled by the
skybox state contract. World rendering therefore paints over the background
instead of the sky painting over the world.

New order:

```
begin frame / clear
set camera
skybox background
world objects
floor/grid
vehicles
pickups
player/NPCs/weapons
projectiles/trails/sprites
HUD
present
```

This keeps gskybox89 renderer-agnostic; the ordering policy belongs to the
Blank3D host bridge.

## Regression gate

`make test-visual-feedback-authority89`

The test verifies:

- local-player actor gating exists in the FIRE_ACCEPTED block;
- FIRE_ACCEPTED owns the player muzzle timer;
- MUZZLE_REQUEST cannot write the player muzzle timer;
- the skybox render call occurs before `blank3d_objects_render()`;
- only one host skybox render call exists.
