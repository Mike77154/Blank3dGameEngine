# invariantSpecialoperations_89 + DDSL2

Blank3D vendors `invariantSpecialoperations_89` as a small agnostic invariant
engine.  It does not know about movement, players, input, flags, DDSL2, Win32,
or any other host concept.

The core understands only opaque subjects, opaque scalar values, directional
rules, and structural special operations.

## DDSL2 surface

The DDSL2 adapter recognizes:

    If <subjectA> TRUE then <subjectB> FALSE
    Apply SpecOp=Viceversa

For the default player:

    If key_hold Up then walk_forward
    If key_hold Shift and key_hold Up then run_forward

    If walk_forward TRUE then run_forward FALSE
    Apply SpecOp=Viceversa

`Viceversa` expands the invariant to the equivalent pair:

    If walk_forward TRUE then run_forward FALSE
    If run_forward TRUE then walk_forward FALSE

The invariant declaration is consumed before ordinary DDSL2 compilation.  Its
lines become blank lines so DDSL2 diagnostic line numbers stay stable.

## Runtime resolution

Invariant-bound DDSL2 outputs are first written to the existing fixed-capacity
`FlagStore` through the flags89 adapter.  Each write is propagated immediately
through `invariantSpecialoperations_89`, but the gameplay action is emitted to
the host only after the DDSL2 VM has completed the frame.

This makes event order useful without hardcoded priority:

1. Up emits `walk_forward = TRUE`.
2. The invariant makes `run_forward = FALSE`.
3. Shift+Up later emits `run_forward = TRUE` in the same DDSL2 frame.
4. Viceversa makes `walk_forward = FALSE`.
5. Only final TRUE invariant subjects are emitted to the host.
6. `3d_movementbaseverbs89` receives `run_forward`.

Therefore the most recent external assertion naturally wins.  The invariant
engine itself does not know that this is walk/run behavior.

## Separation

    DDSL2 syntax adapter
            |
            v
    invariantSpecialoperations_89   (agnostic core)
            |
            v
      flags89 adapter
            |
            v
        FlagStore
            |
            v
       host actions

Other hosts can attach different providers to the same core without using
DDSL2 or FlagStore.
