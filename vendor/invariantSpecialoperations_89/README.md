# invariantSpecialoperations_89

Tiny C89/no-heap/provider-driven helper for declaring invariants without tying
those invariants to gameplay, input, flags, a DSL, or an operating system.

Core relation:

    If A X then B Y

`SpecOp=Viceversa` structurally expands it to:

    If A X then B Y
    If B X then A Y

The core treats subjects and values as opaque scalar tokens. It never assumes
that `1` means true, that `0` means false, or that a subject is a game flag.
Adapters provide those meanings.

## DDSL2 adapter

The included adapter recognizes this intentionally distinct declaration shape:

    If walk_forward TRUE then run_forward FALSE
    Apply SpecOp=Viceversa

Those lines are removed before the ordinary DDSL2 compiler sees the source,
while blank lines are retained for stable compiler line numbers.

This lets ordinary DDSL2 remain unchanged:

    If key_hold Up then walk_forward
    If key_hold Shift and key_hold Up then run_forward

If both emit in the same frame, invariant events are resolved in source/runtime
order before the host executes the final action state. Therefore the later
`run_forward=TRUE` assertion turns `walk_forward` off and run wins naturally.

## flags89 adapter

`adapters/flags89` maps invariant subjects to the existing fixed-capacity
FlagStore. Missing values can be treated as zero. No allocation is performed.

## Build

    make test

The library is strict C89 and contains no malloc/realloc/free or float/double.
