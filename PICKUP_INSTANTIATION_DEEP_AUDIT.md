# Pickup instantiation deep audit

Root cause of the invisible Uzi/ammo test item was before rendering.

RPYL tokenizes an unquoted slash path such as:

    weapon_pickup config/pickups/uzi_weapon.ini pos -6.5 0 24

as command arguments `config`, `pickups`, `uzi_weapon.ini`, `pos`, `-6.5`, `0`, `24`.
The previous Blank3D host expected `args[0]` to be the whole path and `args[1]` to be `pos`, so the pickup spawn branch never ran. No `Blank3DPickupInstance`, PBB item, trigger, GFO, Thing/ECS object, or mesh was created.

Fixes:

- startup.rpy quotes pickup paths, which is the canonical RPYL spelling.
- `blank3d_pickups_spawn_rpyl_args()` reconstructs legacy/bare split paths before spawning, so old scene lines remain accepted.
- The runtime command handler now delegates pickup RPY argument parsing to that adapter.
- Uzi and magazine prototype parts were raised slightly so no box penetrates the y=0 floor plane.
- Tests now assert the actual startup RPY dispatches each pickup with a single quoted path.
- Pickup tests also exercise the legacy split-path form and prove unknown zombie/world subjects cannot consume the pickup.

Collision audit:

- Pickup collection is not driven by Blank3D world collision or VPhysics.
- The CT89 pickup sensor provider only emits `B3D_PLAYER_ACTOR_ID` as a candidate.
- Zombies are not emitted as pickup candidates.
- The ground is not a CT89 subject/candidate.
- Pickup instances are not inserted into the regular enemy/world collision table, so resting on or visually overlapping the floor cannot consume them.
- A zombie can visually overlap a pickup, but cannot collect/disable it through this provider.

Additional RPYL finding and fix:

The vendored RPYL runtime was configured with only eight AST/parser/runtime arguments. The existing `armed_ally` line truncated at the `loadout` keyword. This was separate from pickup instantiation, but was found during the same audit. AST, parser pre-args, and runtime args are now consistently 24, and scene paths are quoted. The language regression now verifies the full long `armed_ally` command survives dispatch.
