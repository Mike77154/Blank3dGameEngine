# Giffany v3.10.0 release notes

The gunner is no longer a special machine-gun case. Armed NPCs now use the same
global INI weapon catalog as the player while retaining actor-private ownership,
clips and ammunition.

Main additions:

- `src/blank3d_npc_inventory.c/.h`
- `config/npc_loadouts/gunner_enemy.ini`
- FPIL actions and conditions for weapon decisions
- RPYL spawn overrides and direct `npc_weapon` commands
- arbitrary INI weapon regression profiles with IDs 42 and 77
- `make test-npc-weapon-inventory`

See `NPC_INVENTORIES_AND_RUNTIME_WEAPON_SELECTION.md` for usage.
