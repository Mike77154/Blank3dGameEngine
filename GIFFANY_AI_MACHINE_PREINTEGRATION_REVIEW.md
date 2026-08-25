# Giffany AI Machine — preintegration review

Reviewed artifact: `giffany_ai_machine_c89_v1-1.zip`.

## Result

The library builds successfully as strict C89 on the host compiler and its demo
runs through idle, combat, cover, reload and fire transitions. The default size
report produced:

```text
sizeof(AI_BBEntry)=8
sizeof(AI_Blackboard)=128
sizeof(AI_EventQueue)=132
sizeof(AI_Agent)=356
sizeof(AI_Machine)=2016
```

It follows the engine rules relevant to Blank3D: caller-owned/static storage,
no runtime allocation, no float/double requirement, fixed capacities and an
iterative runtime.

## Best role inside Blank3D

Do not replace perception, movement or weapons. Use Giffany AI Machine as the
NPC decision coordinator:

```text
Socketer + GFaction + NPC Eyes + Enlightener + GEDER
                         |
                         v
              blackboard/events adapter
                         |
                         v
                Giffany AI Machine
       HFSM + Utility + GOAP + BT + task queue
                         |
                         v
                   dispatch bridge
        FPIL / GAutomotion / GWeapon / GAttach
```

## Important integration rule

Blank3D already owns several decision-capable systems. There must be one final
owner of intent per NPC. The safest split is:

- GEDER/Enlightener publish facts and events.
- Giffany AI Machine selects state, goal and queued intent.
- FPIL becomes an authoring/dispatch layer for concrete verbs.
- GAutomotion and weapon providers execute those verbs.

Running FPIL, a Behavior Tree, Utility AI and GOAP as independent masters in the
same frame would create conflicting actions.

## Required adapters

1. Q20/Q12 engine values to the AI Machine Q8.8 blackboard.
2. Stable operation IDs for guards, actions, scores and state callbacks.
3. Per-agent event routing from GEDER truth changes, damage, sound and faction.
4. Round-robin budget tied to the existing NPC update budget.
5. A single intent lock so only one movement/fire owner writes per agent tick.

## Status

Reviewed and build-tested only. It is not vendored or connected in v3.26.1.
