# Blank3D v3.11.1 — player dual fly + jump test

## Bindings

- `J`: start `jump89` from the floor.
- `F`: ascend with `fly89`.
- `G`: descend with `fly89`, clamped by the player floor.
- `H`: active reload (moved from `F` to avoid an input collision).

The player remains `flyingentity=1`: dumb engine gravity does not pull it down
when flight input stops.  A live `jump89` state has priority over `fly89`, so
pressing J together with F or G produces an ordinary jump arc rather than a
manually distorted one.  To jump after flying, descend to the floor with G and
then press J.

## DDSL2

```text
if key-j = 1 then jump = 1
if key-f = 1 then move_up = 1
if key-g = 1 then move_down = 1
```

Compatibility key stores for `SPACE` and `X` remain exposed to old scripts, but
the bundled player script no longer uses them.
