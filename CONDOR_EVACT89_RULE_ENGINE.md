# Condor EvAct89 rule engine integration

## Role

Condor EvAct89 is the runtime rule core for **Event -> Condition -> Action**.
It does not replace the named authoring vocabulary in `gameverbs89`, and it does
not replace any gameplay system. It connects them.

```text
DDSL2 / GFO / FPIL / RPYL / INI authoring
                 |
                 v
            gameverbs89
      named conditions/actions
                 |
        +--------+--------+
        |                 |
        v                 v
 direct verb call      Condor EvAct89
 (no rule needed)     EVENT -> IF -> DO
        |                 |
        +--------+--------+
                 v
    3DKin / MovementBaseVerbs / VarRuntime /
    GLOCO / Faction / Equipment / other providers
```

A direct `walk_forward` stays a direct GameVerb action. Condor is used when the
intent is reactive: **when X happens, if Y is true, perform Z**.

## Runtime ownership

`condor_evact89` v0.2 is context based. There are no singleton listener,
condition, action, or rule tables. Blank3D owns one `Blank3DCondor` per runtime.

The core keeps generational handles for listeners, conditions, actions and rules,
and snapshots dispatch lists so registration/removal during dispatch is safe.
Capacities are fixed and compile-time configurable; no heap is required.

## Event identity

A Condor event carries two deliberately separate identities:

- `owner`: Actor/GameVerb owner used to execute/query gameplay verbs.
- `instance`: Thing/VarRuntime owner used to read/write per-instance variables.

This avoids making Actor the universal identity. Doors, elevators and other
non-Actor Things can participate in rules while actors still receive their normal
gameplay verb identity.

## Built-in Blank3D event types

- `input_press`
- `input_hold`
- `input_release`
- `thing_spawn`
- `thing_destroy`
- `actor_damage`
- `actor_death`
- `gfo_begin`
- `gfo_end`
- `custom`

Input codes accept HID keyboard usage names in the RPYL adapter (for example
`W`) plus `mouse_left`, `mouse_right`, numeric codes, and `any`/`*` where
applicable.

## Named GameVerb rules

Conditions and actions can be resolved through `gameverbs89` without teaching
Condor about 3DKin, GLOCO, MovementBaseVerbs or any DSL.

Conceptual rule:

```text
EVENT input_hold W
IF    can_walk_forward
DO    walk_forward
```

The action path remains:

```text
walk_forward
 -> gameverbs89
 -> MovementBaseVerbs89
 -> 3DKin preflight/context
 -> GLOCO89
 -> Collision/VPhysics
 -> final Transform
 -> World / Scene / Soquete T0
```

## VarRuntime conditions/actions

The Blank3D bridge can register conditions against VarRuntime values and actions
that SET/ADD/SUB values. Since VarRuntime routes to NumSys, Flags or its dynamic
VarStore, Condor does not duplicate state.

Example concept:

```text
EVENT actor_damage
IF    self.health <= 0
DO    self.dead = true
```

`health` can remain NumSys authority and `dead` can remain Flags/VarRuntime
authority. Condor owns only the rule.

## RPYL authoring

Blank3D exposes generic commands rather than hardcoding each possible rule:

```text
condor_rule input_hold W can_walk_forward walk_forward
condor_rule input_press Space always jump
condor_emit custom 7
```

`always` or `-` means there is no condition.

## GFO host calls

GFO can call the same host-facing commands:

```text
condor_rule custom 7 can_walk_forward walk_forward
condor_emit custom 7
```

GFO lifecycle dispatch additionally emits `gfo_begin` and `gfo_end`; runtime
spawn/destruction emits Thing events.

## Input / Thing / Actor bridges

- Input scanner state emits press/hold/release events after input update.
- Successful GFO/runtime spawns emit `thing_spawn`.
- GFO destroy lifecycle emits `thing_destroy`.
- Actor damage/death emits target Actor as `owner`, target Thing as `instance`,
  and source Actor as event `code`.

If no Condor rules are registered these bridges are inert with respect to
gameplay; existing direct providers keep their original behavior.

## Design boundary

Condor is the **rule brain**. GameVerbs is the **named vocabulary**. Specialized
systems remain authorities for their own domains:

- 3DKin: kinematic/spatial questions.
- MovementBaseVerbs: movement verb semantics.
- GLOCO: locomotion controller/policy.
- Collision/VPhysics: final physical resolution.
- VarRuntime/NumSys/Flags: state.
- Thing/ECS/Actor: runtime identity/composition/gameplay capability.
- Soquete: T0 exact pose/socket truth.

No subsystem is swallowed by Condor.
