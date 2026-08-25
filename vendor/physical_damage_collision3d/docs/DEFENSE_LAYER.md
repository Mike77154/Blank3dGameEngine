# PDC3D v2 Defense Layer

The v2 defense layer vendorizes and wraps these C89 modules:

```txt
gblock3d89    instant block resolver
gguard3d89    held guard stance and guard gauge
gparry3d89    timed parry/deflect windows
gshell3d89    armor/shell absorbent layers
gcounter3d89  counter token and counter request system
```

The engine-facing API stays under `pdc3d_...` names. Vendor names remain internal or optional for advanced tuning.

## Public defense flags

```txt
PDC3D_DEF_BLOCK
PDC3D_DEF_GUARD
PDC3D_DEF_PARRY
PDC3D_DEF_SHELL
PDC3D_DEF_COUNTER
PDC3D_DEF_ALL
```

## Public defense result flags

```txt
PDC3D_DEFRES_PARRY_PERFECT
PDC3D_DEFRES_PARRY_NORMAL
PDC3D_DEFRES_PARRY_LATE
PDC3D_DEFRES_GUARDED
PDC3D_DEFRES_GUARD_BROKEN
PDC3D_DEFRES_BLOCKED
PDC3D_DEFRES_BLOCK_BROKEN
PDC3D_DEFRES_SHELL_ABSORB
PDC3D_DEFRES_SHELL_BROKEN
PDC3D_DEFRES_COUNTER_TOKEN
PDC3D_DEFRES_COUNTER_USED
PDC3D_DEFRES_PASSED
PDC3D_DEFRES_ACCEPTED
```

## Damage packet result flags added in v2

```txt
PDC3D_DMG_DEFENDED
PDC3D_DMG_GUARDED
PDC3D_DMG_PARRY
PDC3D_DMG_PARRY_LATE
PDC3D_DMG_GUARD_BROKEN
PDC3D_DMG_BLOCK_BROKEN
PDC3D_DMG_SHELL
PDC3D_DMG_SHELL_BROKEN
```

## Typical use

```c
pdc3d_damage_packet packet;
pdc3d_defense_result result;
pdc3d_v3 source_dir;

source_dir = pdc3d_v3_make(PDC3D_FX_ONE, 0, 0);
pdc3d_resolve_incoming_attack(&world, &packet, source_dir, &result);
```

## Counter use

```c
pdc3d_counter_request req;
pdc3d_defense_result counter_result;

req.actor_id = defender_id;
req.target_id = attacker_id;
req.stamina_available = 1000;
req.posture_available = 1000;
req.target_rel_pos = source_dir;

pdc3d_counter_try(&world, &req, &counter_result);
```

A successful counter request does not spawn a hit by itself. The engine should translate it into one of these actions:

```txt
PDC3D_COUNTER_LIGHT  -> melee profile / quick strike
PDC3D_COUNTER_HEAVY  -> heavy melee profile / stagger
PDC3D_COUNTER_THROW  -> grab/throw resolver
PDC3D_COUNTER_SCRIPT -> gameplay script callback
```
