# rocketmeshes behavior profiles

This patch turns `rocketmeshes` from a pure mesh table into a mesh + declarative visual behavior table.

The values are gameplay/renderer hints, not real ballistic simulation constants.

## Core distinction

- **40mm grenade launcher:** cartridge/case behavior; spent case appears when the breech opens or reload begins.
- **M202:** four rocket clip/tube behavior; no brass per shot, but an empty clip can appear on reload.
- **Bazooka:** rocket tube burnout behavior; no brass shell, mainly backblast and optional tiny seal/cap debris.
- **RPG-7 family:** booster + fin deployment + optional delayed sustainer; no shell.
- **OG-7V:** boosted/coasting fragmentation visual, minimal trail.

## Runtime calls

```c
const RM_BehaviorProfile *p;
p = rm_get_behavior_profile(RMESH_RPG7_PG7V);
if (p && rm_behavior_is_projectile(RMESH_RPG7_PG7V)) {
    /* p->fire_fx_mesh -> spawn backblast helper */
    /* p->ticks_to_fin_open -> swap folded/open fins */
    /* p->ticks_to_sustainer_visual -> start delayed trail */
}
```

## Profiles

| Mesh | Propulsion | Ejection | Fin rule | Trail | Notes |
|---|---|---|---|---|---|
| `RMESH_GRENADE40_LV` | `cartridge_low_velocity` | `case_on_breech_open` | `spin_stabilized` | `muzzle_puff_only` | spent case on reload/open-breech |
| `RMESH_M74_FLASH` | `rocket_clip_tube` | `empty_clip_on_reload` | `fixed_foldout` | `short_rocket_smoke` | per-tube empty state, empty 4-rocket clip on reload |
| `RMESH_BAZOOKA_M6A3` | `rocket_tube_burnout` | `optional_tail_cap` | `fixed_ring` | `backblast_only` | no brass; short rear puff and optional tiny cap debris |
| `RMESH_BAZOOKA_M28` | `rocket_tube_burnout` | `optional_seal_debris` | `fixed_foldout` | `backblast_only` | no brass; stronger puff and optional seal debris |
| `RMESH_RPG7_PG7V` | `two_stage_rpg` | `smoke_only` | `deploy_after_launch` | `delayed_sustainer` | booster puff, fins open after launch, trail after delay |
| `RMESH_RPG7_PG7VR` | `two_stage_rpg` | `smoke_only` | `deploy_after_launch` | `delayed_sustainer` | same RPG phases but heavier visual feel |
| `RMESH_RPG7_OG7V` | `boosted_grenade_no_sustainer` | `smoke_only` | `deploy_after_launch` | `coast_minimal` | fragmentation visual: no sustained motor trail |
| `RMESH_SURVIVAL_RPG7_CONE` | `two_stage_rpg` | `smoke_only` | `deploy_after_launch` | `delayed_sustainer` | survival-horror RPG style: theatrical two-stage profile |
| `RMESH_GENERIC_ROCKET_STUB` | `generic_rocket` | `smoke_only` | `fixed_foldout` | `short_rocket_smoke` | fallback rocket profile |
