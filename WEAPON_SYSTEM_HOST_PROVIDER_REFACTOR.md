# Weapon System hosted/provider refactor

## Rule

Blank3D is a host. It does not own reusable weapon policy. The Weapon System
owns weapon semantics and asks the host for external services through provider
contracts/adapters.

## Modules moved into `vendor/weapon_system`

- `gweaponmodules89` — weapon composition/module data.
- `gweaponprofileio89` — parses weapon manifests/profiles through IO provider.
- `gweaponloadout89` — catalog/loadout policy through IO provider.
- `gweaponaim89` — universal weapon aim frame math.
- `gplayerfireray89` — player-only fire-ray core with raycast callback.
- `gplayerprojectileaim89` — player camera-to-physical-muzzle projectile aim.
- `gweaponballistics89` — launch/ballistic reconciliation.
- `gboltweapon89` — Bolt3D projectile weapon behavior; host collision is an adapter callback.
- `gshotgun89` — pellet finalization/spread semantics.
- `gfireframe89` — immutable same-frame fire/camera state and deferred recoil contract.
- `gtrigger89` — standard, spin-up, and charge/release trigger state.
- `gprojectilespawn89` — projectile spawn orchestration through providers.
- `gcasingruntime89` — casing event orchestration through world/physics/render/audio providers.
- `gweaponsnapshot89` — actor/weapon/muzzle/camera same-frame snapshot.
- `gweaponcrosshair89` — weapon-side crosshair runtime; host supplies presentation sinks.
- `gweaponsniper89` — weapon-side sniper/scope runtime; host supplies camera/raycast/presentation hooks.
- `gweaponpresentation89` — presentation profile data, not a renderer/backend.
- `gmechanicalweapon89` — reusable mechanical-weapon runtime.
- `gweaponlaunch89` + `g3dweaponzeroing89` — launch coherence and zeroing invariants.

## Host hooks retained in `src/`

The remaining `blank3d_*` files in these areas are compatibility shims or
adapters. They translate Blank3D types/services into Weapon-System contracts.
They are not the reusable implementation.

Notable host hooks:

- `blank3d_weapon_host_io.*` implements `GWP89_SERVICE_IO` using the host filesystem.
- `blank3d_player_fire_ray.c` converts Blank3D collision hits to `gplayerfireray89` hits.
- `blank3d_player_projectile_aim.c` binds the local/view-controlled player camera and host raycast provider.
- `blank3d_bolt.c` binds host collision sweep to `gboltweapon89`.
- `blank3d_fire_frame_sync.c` binds Cameranaku to the neutral `gfireframe89` provider callbacks.

## Provider ownership

| External concern | Weapon System expects | Blank3D currently supplies |
|---|---|---|
| numeric state | `GWP89_SERVICE_NUMERIC` | `numsys` adapter |
| flags/policy gates | `GWP89_SERVICE_FLAGS` | `flags89` adapter |
| actor weapon inventory/ammo | `GWP89_SERVICE_INVENTORY` | `gkinventory` for player; NPC inventory adapter for NPCs |
| filesystem/text | `GWP89_SERVICE_IO` | `blank3d_weapon_host_io` |
| camera | camera provider / immutable fire-frame snapshot | Cameranaku adapter |
| raycast/collision | raycast/sweep callback/provider | Blank3D collision world |
| projectile world | `gprojectilespawn89` providers | Blank3D fixed projectile pool/world hooks |
| casing physics | `gcasingruntime89` physics provider | VPhysics adapter |
| rendering/HUD/scope | presentation sinks/providers | Blank3D/OpenGL/HUD adapters |
| audio | audio/event provider | weapon synth sound host |

The important direction of dependency is always:

```
Blank3D host/backend
       |
       | providers/adapters
       v
Weapon System contracts
       |
       v
Weapon modules/policy
```

The Weapon System never reaches upward into Blank3D to discover a platform,
key binding, world implementation, inventory implementation, or renderer.

## Player vs non-player aiming

`gplayerfireray89` and `gplayerprojectileaim89` are intentionally player/view
controlled modules. They may use the active presentation camera. NPCs,
turrets, allies and remote actors use their actor/muzzle/aim providers and do
not inherit the player's camera semantics.

## Same-frame invariant

Weapon/muzzle/camera data is captured after locomotion and attachment sync and
before weapon update. A projectile therefore cannot use an actor transform
from frame N with a muzzle transform from frame N-1.

## Input authority

The Weapon System sees the logical `shoot` verb. It does not know about
MouseLeft, Space, Win32 VK codes, or any concrete input device.
