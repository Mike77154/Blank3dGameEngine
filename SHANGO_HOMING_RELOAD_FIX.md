# Shango + Homing reload fix

## Symptom

Weapon 12 (ShangoWeapon) and weapon 13 (Homing Rocket Launcher) could fire their initial chambered/loaded projectile, but pressing reload appeared to do nothing or did not make a second shot possible.

## Root cause A: Shango double one-shot gate

Shango combined two independent one-shot mechanisms:

- `gtrigger89` profile `charge_release`, which converts the release of a completed charge into one synthetic trigger press.
- GWeapon `fire_mode=hold_once`, which latches the trigger until it receives a `GWP89_TRIGGER_RELEASED` update.

The synthetic charge-release press did not have a matching GWeapon release edge. Therefore GWeapon left `trigger_latched=1` after the first Shango shot. The magazine could actually refill, but all later fire attempts remained trigger-locked.

Fix: Shango now uses `fire_mode=semi`. `gtrigger89` remains the authority that turns a charge release into exactly one trigger press, so the second one-shot gate was redundant.

## Root cause B: runtime reserve-ammo configuration gap

The full player loadout already supplied reserve ammo for the new weapons, but the host runtime configuration path in `apply_runtime_config()` only injected ammo IDs 1 through 9 from `config/blank3d.toml`.

The new ammunition IDs are:

- 11: Shango cells
- 12: homing rockets
- 13: flamethrower fuel

That made patch/merged worktrees fragile: a weapon could begin with its one-round clip initialized from the weapon profile while reserve ammo remained zero. In that state `gwp89_begin_reload()` correctly returns `GWP89_NO_AMMO`.

Fix: `Blank3DConfig`, `blank3d.toml`, and `apply_runtime_config()` now explicitly carry ammo 11, 12 and 13.

## Reload feedback

The Win32 runner now surfaces reload state instead of silently ignoring the return value:

- reload begin: weapon, clip and reserve
- reload complete: weapon, clip and reserve
- no reserve ammo
- provider/`weapon.can_reload` cancellation
- unexpected negative result code

This makes future reload failures observable without attaching a debugger.

## Regression

`tests/test_special_reload.c` exercises the same trigger-to-weapon path used by the runner:

1. Shango/Homing begin with clip 1 and reserve 6.
2. Fire once and assert clip 0.
3. Begin reload.
4. Advance the weapon runtime beyond reload duration.
5. Assert clip 1 and reserve 5.
6. Fire a second time and assert clip 0.
7. For Shango, also assert that the GWeapon trigger latch is clear after charge release.

This test uses `gtrigger89` rather than bypassing directly to the weapon manager.

## Notes

The old `test-core` target already fails with error 42 in the input baseline because its weapon-cycle expectations predate the expanded starting weapon list. The reload changes do not introduce that failure. Focused reload, config, weapon INI, Sat/Telesearcher, flamethrower stack, item/contact integration, and full source syntax checks pass.
