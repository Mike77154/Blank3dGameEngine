# PDC3D Architecture

```txt
                 PhysicalDamageCollision3D
                           |
       +-------------------+-------------------+
       |                   |                   |
   melee3d             grab3d              throws3d
       |                   |                   |
 hurtbox3d + hitbox3d  grab shapes        throw bodies
       |
 weakspots3d + material damage table
       |
 PDC3D event queue / bridge callbacks
```

## Why this shape

Melee combat needs three different things to agree:

1. **Timing**: startup, active and recovery frames.
2. **Space**: hit volumes and hurt volumes.
3. **Gameplay meaning**: material, body part, weakspot, stun, hitstop, grab, throw.

The original zip already had all three ideas, but scattered across independent modules. PDC3D adds one facade so an engine can call a single system without linking duplicate `hurtbox3d` sources.

## Deterministic tick model

PDC3D expects to be called from a fixed game tick. Do not call it with variable render delta. The engine can render however it wants, but combat should advance in discrete ticks.

```txt
render loop produces time
fixed simulation consumes ticks
PDC3D advances once per fixed combat tick
```

## Socket sweep model

For melee weapons and fast limbs, the bridge supplies current socket positions. PDC3D updates the same hitbox slot each tick. The vendor hitbox layer stores the previous shape and queries a swept AABB/shape union.

```txt
previous socket A ---- current socket A
previous socket B ---- current socket B
        \              /
         swept capsule
```

This is intentionally conservative and fixed-point friendly. It avoids missing hits when a sword, claw or tentacle crosses a target between frames.

## Damage packet

A PDC3D damage packet contains:

```txt
attacker id
defender id
attack id
hitbox index
hurtbox index
base damage
final damage
stun frames
hitstop frames
attack flags
material/body flags
weakspot id
user tags
```

The engine should map this packet into health, infection, animation, AI reaction, blood/sparks, sound and DSL events.
