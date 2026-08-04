# NPC weapon manager isolation (v3.9.5)

The player and NPCs now use separate `GWP89_Manager` instances.

- `g.systems.weapons`: player manager, GKInventory-backed.
- `g.npc_weapons`: NPC manager, internal ammo/clip banks.

The NPC manager clones the same weapon profiles and provider table, then clears all mutable actor/event state. This preserves one weapon implementation while preventing shared reload timers, trigger latches, user slots, ammo banks and event queue saturation.

NPC events are drained after player events and still feed the same projectile/audio/render pipeline.
