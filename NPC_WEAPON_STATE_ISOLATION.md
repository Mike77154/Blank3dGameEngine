# NPC weapon-state isolation (v3.9.4)

The stationary gunner and the player share the same provider-driven `gweapon89`
manager, but they must never share mutable actor state.

This revision adds a host-boundary guard around NPC fire/reload operations. It
preserves the player's:

- `GWP89_UserState` (trigger, cooldown, reload timer, equipped slot and clip),
- `Blank3DSystems.current_weapon_id`,
- `Blank3DSystems.last_trigger_down`,
- external per-weapon clip mirror used by GKInventory.

NPC reload and active-reload events also no longer alter player-only status UI.
The NPC still uses the same manager, profiles, events, projectile providers and
audio synthesizer; only accidental cross-actor host state is rejected.
