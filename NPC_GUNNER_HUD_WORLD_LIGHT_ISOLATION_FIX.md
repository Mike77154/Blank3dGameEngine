# NPC Gunner -> HUD world-light isolation fix

## Symptom

When allied/enemy gunners fired, player HUD elements such as the magazine
indicator and health bars visibly reacted even though the local player's
ammo/health state had not changed.

## Root cause

The data path was already local-player scoped:

- health is supplied to `blank3d_hud_draw()` from `g.player_hp`;
- clip/reserve are queried from `g.systems` for `B3D_PLAYER_ACTOR_ID`;
- NPC weapon users live in the separate `g.npc_weapons` manager.

The leak was render state, not gameplay state.

`bridge_gl_set_muzzle_light()` enables `GL_LIGHT1` for a muzzle light.  NPC
muzzle events are intentionally allowed to emit those world lights.  The HUD
then entered its orthographic 2D pass without disabling `GL_LIGHTING`, so
GBar/NumBar/crosshair immediate-mode geometry remained subject to the active
world lights.  An NPC shot could therefore brighten/tint player-only UI.

## Fix

`src/blank3d_hud.c` now defines a hard 2D presentation boundary:

- disable `GL_LIGHTING`;
- disable `GL_TEXTURE_2D` at the baseline (sprite providers opt in locally);
- disable depth test;
- use normal alpha blending;
- reset current color to opaque white.

The end of the HUD pass normalizes texture/blend/color and restores the
engine world baseline (`GL_DEPTH_TEST` + `GL_LIGHTING`).

`src/blank3d_image_gl.c` applies the same boundary to the generic overlay
pass, preventing the same class of leak in SpriteAsset/SpriteVerbs overlays.

The active `config/hud/gameplay.ini` health NumBar also no longer binds its
optional overlay channel to `gameplay.threat`.  Threat remains available to
dedicated ECG/threat presentation, but a health bar is now semantically driven
by player health only.

## Authority after the fix

```text
NPC gunner fire
    |
    +--> NPC weapon state
    +--> world muzzle sprite/light (GL_LIGHT1)
    `--> world projectile/casing

                       X no world lighting crosses this boundary
                       |
PLAYER HUD  <----------+
    |
    +--> g.player_hp
    +--> B3D_PLAYER_ACTOR_ID clip/reserve
    `--> local-player crosshair feedback
```

The muzzle light is still visible on world geometry; it simply cannot light
screen-space HUD geometry.

## Regression gate

`make test-hud-world-light-isolation89`

The gate verifies both HUD/overlay light isolation and that the host HUD call
continues to use player-local health/clip/reserve sources.

If a future runtime test shows that the numeric player HP or clip value itself
changes without the player being hit/firing, that is a separate gameplay-data
or collision bug.  This fix addresses the confirmed visual state leak.
