# GLOCO89 player ground-sweep fix

## Symptom

After the GLOCO89 locomotion integration the player accepted input but did not
translate horizontally.

## Root cause

Blank3D's real collision sweep reports the default floor contact at the player
capsule tangent point with fraction `0` and a normal whose Y component can be
negative (`0,-4096,0` in Q12). The original GLOCO host adapter treated only a
positive-Y normal as ground. A negative-Y floor normal therefore looked like a
side wall, set `GLOCO_FLAG_BLOCKED`, zeroed horizontal velocity and kept the
player at the same transform.

A second issue was exposed by the same regression: simply ignoring that floor
hit was not sufficient because the floor remained the first fraction-0 hit and
could hide a later vertical wall in the same horizontal sweep.

## Fix

`src/blank3d_gloco.c` now:

1. starts the horizontal sphere sweep with a small Q12 vertical skin
   (`1/64` world unit) above the exact floor tangent point;
2. classifies floor/ceiling contacts by `abs(normal.y)` rather than only `+Y`;
3. preserves side-wall blocking for normals whose absolute Y component is below
   half of Q12 one.

No GLOCO vendor code and no pre-existing vendor library was changed.

## Regression

`tests/test_gloco_ground_sweep.c` uses the real Blank3D Collision stack and
verifies both sides of the contract:

- a player standing at `y=0` can move horizontally;
- the real vertical backstop still produces `GLOCO_FLAG_BLOCKED`.

Run:

    make test-gloco-ground-sweep

The ordinary GLOCO provider, Collision, VPhysics, MovementBaseVerbs, input,
syntax/Win32 syntax and no-allocation audits are also expected to remain green.

## Clock skew

The release ZIP is written with normalized old timestamps so extraction on
MSYS2/MinGW does not make source files appear newer than the local clock. For an
already extracted older package, remove stale outputs and normalize timestamps
before rebuilding.
